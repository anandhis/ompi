/*
 * Copyright (c) 2015 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "orte_config.h"
#include "orte/constants.h"

#include "opal/mca/base/base.h"
#include "opal/util/output.h"
#include "opal/mca/backtrace/backtrace.h"
#include "opal/mca/event/event.h"

#if OPAL_ENABLE_FT_CR == 1
#include "orte/mca/rml/rml.h"
#include "orte/mca/state/state.h"
#endif
#include "orte/mca/rml/base/base.h"
#include "orte/mca/rml/rml_types.h"
#include "orte/mca/routed/routed.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/util/name_fns.h"
#include "orte/runtime/orte_globals.h"

#include "rml_ofi.h"

static int rml_ofi_component_open(void);
static int rml_ofi_component_close(void);
static orte_rml_base_module_t* open_conduit(opal_list_t *attributes);
static orte_rml_pathway_t* query_transports(void);
static char* ofi_get_contact_info(void);
static void process_uri(char *uri);
static void ofi_set_contact_info (const char *uri);
void convert_to_sockaddr( char *ofiuri, struct sockaddr_in* ep_sockaddr);

/**
 * component definition
 */
orte_rml_component_t mca_rml_ofi_component = {
      /* First, the mca_base_component_t struct containing meta
         information about the component itself */

    .base = {
        ORTE_RML_BASE_VERSION_3_0_0,

        .mca_component_name = "ofi",
        MCA_BASE_MAKE_VERSION(component, ORTE_MAJOR_VERSION, ORTE_MINOR_VERSION,
                              ORTE_RELEASE_VERSION),
        .mca_open_component = rml_ofi_component_open,
        .mca_close_component = rml_ofi_component_close,
     },
    .data = {
        /* The component is checkpoint ready */
        MCA_BASE_METADATA_PARAM_CHECKPOINT
    },
    .priority = 10,
    .open_conduit = open_conduit,
    .query_transports = query_transports,
    .get_contact_info = ofi_get_contact_info,
    .set_contact_info = ofi_set_contact_info,
    .close_conduit = NULL
};

/* Local variables */
orte_rml_ofi_module_t orte_rml_ofi = {
    .api = {
    .component = (struct orte_rml_component_t*)&mca_rml_ofi_component,
    .ping = NULL,
    .send_nb = orte_rml_ofi_send_nb,
    .send_buffer_nb = orte_rml_ofi_send_buffer_nb,
    .purge = NULL
    }
};

/* Local variables */
static bool init_done = false;

static int
rml_ofi_component_open(void)
{
     /* Initialise endpoint and all queues */

    orte_rml_ofi.fi_info_list = NULL;
    orte_rml_ofi.min_ofi_recv_buf_sz = MIN_MULTI_BUF_SIZE;
    orte_rml_ofi.cur_msgid = 1;
    orte_rml_ofi.cur_transport_id = RML_OFI_CONDUIT_ID_INVALID;
    orte_rml_ofi.conduit_open_num = 0;
    OBJ_CONSTRUCT(&orte_rml_ofi.peers, opal_hash_table_t);
    opal_hash_table_init(&orte_rml_ofi.peers, 128);

    for( uint8_t conduit_id=0; conduit_id < MAX_CONDUIT ; conduit_id++) {
        orte_rml_ofi.ofi_conduits[conduit_id].fabric =  NULL;
        orte_rml_ofi.ofi_conduits[conduit_id].domain =  NULL;
        orte_rml_ofi.ofi_conduits[conduit_id].av     =  NULL;
        orte_rml_ofi.ofi_conduits[conduit_id].cq     =  NULL;
        orte_rml_ofi.ofi_conduits[conduit_id].ep     =  NULL;
        orte_rml_ofi.ofi_conduits[conduit_id].ep_name[0] = 0;
        orte_rml_ofi.ofi_conduits[conduit_id].epnamelen = 0;
        orte_rml_ofi.ofi_conduits[conduit_id].mr_multi_recv = NULL;
        orte_rml_ofi.ofi_conduits[conduit_id].rxbuf = NULL;
        orte_rml_ofi.ofi_conduits[conduit_id].rxbuf_size = 0;
        orte_rml_ofi.ofi_conduits[conduit_id].progress_ev_active = false;
        orte_rml_ofi.ofi_conduits[conduit_id].conduit_id = RML_OFI_CONDUIT_ID_INVALID;
    }

    opal_output_verbose(1,orte_rml_base_framework.framework_output," from %s:%d rml_ofi_component_open()",__FILE__,__LINE__);

    return ORTE_SUCCESS;
}


void free_conduit_resources( int conduit_id)
{

    int ret=0;
    opal_output_verbose(10,orte_rml_base_framework.framework_output,
                       " %s - free_conduit_resources() begin. OFI conduit_id- %d",
                       ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),conduit_id);
    if (orte_rml_ofi.ofi_conduits[conduit_id].ep) {
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                       " %s - close ep",ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
         CLOSE_FID(orte_rml_ofi.ofi_conduits[conduit_id].ep);
         if (ret)
         {
            opal_output_verbose(10,orte_rml_base_framework.framework_output,
                                " %s - fi_close(ep) failed with error- %d",
                                ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),ret);
         }
    }
    if (orte_rml_ofi.ofi_conduits[conduit_id].mr_multi_recv) {
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                       " %s - close mr_multi_recv",ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
         CLOSE_FID(orte_rml_ofi.ofi_conduits[conduit_id].mr_multi_recv);
    }
    if (orte_rml_ofi.ofi_conduits[conduit_id].cq) {
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                       " %s - close cq",ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
         CLOSE_FID(orte_rml_ofi.ofi_conduits[conduit_id].cq);
    }
    if (orte_rml_ofi.ofi_conduits[conduit_id].av) {
         CLOSE_FID(orte_rml_ofi.ofi_conduits[conduit_id].av);
    }
    if (orte_rml_ofi.ofi_conduits[conduit_id].domain) {
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                       " %s - close domain",ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
         CLOSE_FID(orte_rml_ofi.ofi_conduits[conduit_id].domain);
    }
    if (orte_rml_ofi.ofi_conduits[conduit_id].fabric) {
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                       " %s - close fabric",ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
         fi_close((fid_t)orte_rml_ofi.ofi_conduits[conduit_id].fabric);
    }
    if (orte_rml_ofi.ofi_conduits[conduit_id].rxbuf) {
         free(orte_rml_ofi.ofi_conduits[conduit_id].rxbuf);
    }

    orte_rml_ofi.ofi_conduits[conduit_id].fabric =  NULL;
    orte_rml_ofi.ofi_conduits[conduit_id].domain =  NULL;
    orte_rml_ofi.ofi_conduits[conduit_id].av     =  NULL;
    orte_rml_ofi.ofi_conduits[conduit_id].cq     =  NULL;
    orte_rml_ofi.ofi_conduits[conduit_id].ep     =  NULL;
    orte_rml_ofi.ofi_conduits[conduit_id].ep_name[0] = 0;
    orte_rml_ofi.ofi_conduits[conduit_id].epnamelen = 0;
    orte_rml_ofi.ofi_conduits[conduit_id].rxbuf = NULL;
    orte_rml_ofi.ofi_conduits[conduit_id].rxbuf_size = 0;
    orte_rml_ofi.ofi_conduits[conduit_id].fabric_info = NULL;
    orte_rml_ofi.ofi_conduits[conduit_id].mr_multi_recv = NULL;
    orte_rml_ofi.ofi_conduits[conduit_id].conduit_id = RML_OFI_CONDUIT_ID_INVALID;
    OPAL_LIST_DESTRUCT(&orte_rml_ofi.recv_msg_queue_list);


    if( orte_rml_ofi.ofi_conduits[conduit_id].progress_ev_active) {
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            " %s - deleting progress event",
                            ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
        opal_event_del( &orte_rml_ofi.ofi_conduits[conduit_id].progress_event);
    }

    return;
}


static int
rml_ofi_component_close(void)
{

    int rc;
    opal_object_t *value;
    uint64_t key;
    void *node;

    opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            " %s - rml_ofi_component_close() -begin, total open OFI conduits = %d",
                            ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),orte_rml_ofi.conduit_open_num);

    if(orte_rml_ofi.fi_info_list) {
    (void) fi_freeinfo(orte_rml_ofi.fi_info_list);
    }

    /* Close endpoint and all queues */
    for( uint8_t conduit_id=0;conduit_id<orte_rml_ofi.conduit_open_num;conduit_id++) {
         free_conduit_resources(conduit_id);
    }

    /* release all peers from the hash table */
    rc = opal_hash_table_get_first_key_uint64 (&orte_rml_ofi.peers, &key,
                                               (void **) &value, &node);
    while (OPAL_SUCCESS == rc) {
        if (NULL != value) {
            OBJ_RELEASE(value);
        }
        rc = opal_hash_table_get_next_key_uint64 (&orte_rml_ofi.peers, &key,
                                                  (void **) &value, node, &node);
    }

    OBJ_DESTRUCT(&orte_rml_ofi.peers);

    opal_output_verbose(10,orte_rml_base_framework.framework_output,
                        " %s - rml_ofi_component_close() end",ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
    return ORTE_SUCCESS;
}


void print_provider_info (struct fi_info *cur_fi )
{
    //Display all the details in the fi_info structure
    opal_output_verbose(1,orte_rml_base_framework.framework_output,
                        " %s - Print_provider_info() ",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));

        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            " fi_info[]->caps                  :  0x%x \n",cur_fi->caps);
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            " fi_info[]->mode                  :  0x%x \n",cur_fi->mode);
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            " fi_info[]->address_format        :  0x%x \n",cur_fi->addr_format);
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            " fi_info[]->fabric_attr->provname :  %s \n",cur_fi->fabric_attr->prov_name);
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            " fi_info[]->src_address           :  0x%x \n",cur_fi->src_addr);
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            " fi_info[]->dest_address          :  0x%x \n",cur_fi->dest_addr);
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            " EndPoint Attributes (ep_attr)    :");
        switch( cur_fi->ep_attr->type)
        {
            case FI_EP_UNSPEC:
                 opal_output_verbose(10,orte_rml_base_framework.framework_output," FI_EP_UNSPEC \n");
                 break;
            case FI_EP_MSG:
                 opal_output_verbose(10,orte_rml_base_framework.framework_output," FI_EP_MSG \n");
                 break;
            case FI_EP_DGRAM:
                 opal_output_verbose(10,orte_rml_base_framework.framework_output," FI_EP_DGRAM \n");
                 break;
            case FI_EP_RDM:
                 opal_output_verbose(10,orte_rml_base_framework.framework_output," FI_EP_RDM \n");
                 break;
            default:
                 opal_output_verbose(10,orte_rml_base_framework.framework_output," %d",cur_fi->ep_attr->type);
    }
    opal_output_verbose(10,orte_rml_base_framework.framework_output,
                        " Protocol            : 0x%x \n", cur_fi->ep_attr->protocol);
 }


void print_provider_list_info (struct fi_info *fi )
{
    struct fi_info *cur_fi = fi;
    int fi_count = 0;
    //Display all the details in the fi_info structure
    opal_output_verbose(1,orte_rml_base_framework.framework_output,
                        " %s - Print_provider_list_info() ",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));

    while(cur_fi != NULL)   {
        fi_count++;
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            " %d.\n",fi_count);
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            " fi_info[]->caps                  :  0x%x \n",cur_fi->caps);
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            " fi_info[]->mode                  :  0x%x \n",cur_fi->mode);
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            " fi_info[]->address_format        :  0x%x \n",cur_fi->addr_format);
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            " fi_info[]->fabric_attr->provname :  %s \n",cur_fi->fabric_attr->prov_name);
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            " fi_info[]->src_address           :  0x%x \n",cur_fi->src_addr);
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            " fi_info[]->dest_address          :  0x%x \n",cur_fi->dest_addr);
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            " EndPoint Attributes (ep_attr)    :");
        switch( cur_fi->ep_attr->type)
        {
            case FI_EP_UNSPEC:
                 opal_output_verbose(10,orte_rml_base_framework.framework_output," FI_EP_UNSPEC \n");
                 break;
            case FI_EP_MSG:
                 opal_output_verbose(10,orte_rml_base_framework.framework_output," FI_EP_MSG \n");
                 break;
            case FI_EP_DGRAM:
                 opal_output_verbose(10,orte_rml_base_framework.framework_output," FI_EP_DGRAM \n");
                 break;
            case FI_EP_RDM:
                 opal_output_verbose(10,orte_rml_base_framework.framework_output," FI_EP_RDM \n");
                 break;
            default:
                 opal_output_verbose(10,orte_rml_base_framework.framework_output," %d",cur_fi->ep_attr->type);
    }
    opal_output_verbose(10,orte_rml_base_framework.framework_output,
                        " Protocol            : 0x%x \n", cur_fi->ep_attr->protocol);
    cur_fi = cur_fi->next;
    }
    opal_output_verbose(10,orte_rml_base_framework.framework_output,
                        "Total # of providers supported is %d\n",fi_count);

}

/*
 * This returns all the supported transports in the system that support endpoint type RDM (reliable datagram)
 * The providers returned is a list of type opal_valut_t holding opal_list_t
 */
/*[Anandhi] old defn static int orte_rml_ofi_query_transports(opal_value_t **providers) */
static orte_rml_pathway_t* query_transports(void)
{
    opal_list_t *ofi_prov = NULL;
    opal_value_t *providers_list, *prev_provider=NULL, *next_provider=NULL;
    struct fi_info *cur_fi = NULL;
    int ret = 0, prov_num = 0;

 opal_output_verbose(10,orte_rml_base_framework.framework_output,
                        "\n%s:%d OFI Query Interface not implemented",__FILE__,__LINE__);
/* [Anandhi] comment fully for now, as this implementation is for old defn needs to be re-written
    opal_output_verbose(10,orte_rml_base_framework.framework_output,
                        " %s -Begin of query_transports()",ORTE_NAME_PRINT(ORTE_PROC_MY_NAME) );
    if ( NULL == *providers)
    {
        *providers = OBJ_NEW(opal_value_t);
    }

    providers_list = *providers;

    //Create the opal_value_t list in which each item is an opal_list_t that holds the provider details
    opal_output_verbose(10,orte_rml_base_framework.framework_output,
      "Starting to add the providers in a loop from orte_rml_ofi.ofi_conduits[] %s:%d",__FILE__,__LINE__);

    for ( prov_num = 0; prov_num < orte_rml_ofi.conduit_open_num ; prov_num++ ) {
        cur_fi = orte_rml_ofi.ofi_conduits[prov_num].fabric_info;
        if( NULL != prev_provider)
        {
           //if there is another provider in the array, then add another item to the providers_list
           next_provider = OBJ_NEW(opal_value_t);
           providers_list->super.opal_list_next = &next_provider->super;
           providers_list->super.opal_list_prev = &prev_provider->super;
           providers_list = (opal_value_t *)providers_list->super.opal_list_next;
        }

        /*  populate the opal_list_t *ofi_prov with provider details from the
         *  orte_rml_ofi.fi_info_list array populated in the rml_ofi_component_init() fn.*/
/*        ofi_prov = OBJ_NEW(opal_list_t);
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            "\n loading the attribute ORTE_CONDUIT_ID");
        if( ORTE_SUCCESS !=
                (ret = orte_set_attribute( ofi_prov, ORTE_CONDUIT_ID, ORTE_ATTR_GLOBAL,
                       (void *)&orte_rml_ofi.ofi_conduits[prov_num].conduit_id ,OPAL_UINT8))) {
            opal_output_verbose(1,orte_rml_base_framework.framework_output,
                                       "%s:%d Not able to add provider conduit_id ",__FILE__,__LINE__);
                   return ORTE_ERROR;
        }
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                     "\n provider conduit_id : %d",orte_rml_ofi.ofi_conduits[prov_num].conduit_id);
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            "\n loading the attribute ORTE_PROV_NAME");
        if( ORTE_SUCCESS ==
            (ret = orte_set_attribute( ofi_prov, ORTE_PROV_NAME, ORTE_ATTR_GLOBAL,cur_fi->fabric_attr->prov_name ,OPAL_STRING))) {
            opal_output_verbose(10,orte_rml_base_framework.framework_output,
                                "\n loading the attribute ORTE_PROTOCOL %s",fi_tostr(&cur_fi->ep_attr->protocol,FI_TYPE_PROTOCOL));
            if( ORTE_SUCCESS ==
                (ret = orte_set_attribute( ofi_prov, ORTE_PROTOCOL, ORTE_ATTR_GLOBAL,(void *)&cur_fi->ep_attr->protocol ,OPAL_UINT32))) {
               // insert the opal_list_t into opal_value_t list
               opal_value_load(providers_list,ofi_prov,OPAL_PTR);
               opal_output_verbose(10,orte_rml_base_framework.framework_output,
                                  "\n loading the provider opal_list_t* prov=%x into opal_value_t list successful",
                                  ofi_prov);
            } else {
                   opal_output_verbose(1,orte_rml_base_framework.framework_output,
                                       "%s:%d Not able to add provider name ",__FILE__,__LINE__);
                   return ORTE_ERROR;
            }
        } else {
               opal_output_verbose(1,orte_rml_base_framework.framework_output,
                                   "%s:%d Not able to add provider name ",__FILE__,__LINE__);
               return ORTE_ERROR;
        }

        prev_provider = providers_list;
        cur_fi = cur_fi->next;
    }

    opal_output_verbose(10,orte_rml_base_framework.framework_output,
                        "\n%s:%d Completed Query Interface",__FILE__,__LINE__);*/
    return ORTE_SUCCESS;
}


/*debug routine to print the opal_value_t returned by query interface */
/* [Anandhi] - this has to be re-written
void print_transports_query()
{
    opal_value_t *providers=NULL;
    char* prov_name = NULL;
    int ret;
    int32_t *protocol_ptr, protocol;
    int8_t* prov_num;

    protocol_ptr = &protocol;

    opal_output_verbose(10,orte_rml_base_framework.framework_output,
                        "\n print_transports_query() Begin- %s:%d",__FILE__,__LINE__);
    opal_output_verbose(10,orte_rml_base_framework.framework_output,
                        "\n calling the orte_rml_ofi_query_transports() ");
    if( ORTE_SUCCESS == orte_rml_ofi_query_transports(&providers))
    {
        opal_output_verbose(20,orte_rml_base_framework.framework_output,
                "\n query_transports() completed, printing details\n");
        while (providers)
        {
            //get the first opal_list_t;
            opal_list_t *prov;
            ret = opal_value_unload(providers,(void **)&prov,OPAL_PTR);
            if (ret == OPAL_SUCCESS) {
                if( orte_get_attribute( prov, ORTE_CONDUIT_ID, (void **)&prov_num,OPAL_UINT8)) {
                    opal_output_verbose(10,orte_rml_base_framework.framework_output,
                                        "\n OFI Provider conduit_id  : %d",*prov_num);
                }
                if( orte_get_attribute( prov, ORTE_PROTOCOL, (void **)&protocol_ptr,OPAL_UINT32)) {
                    opal_output_verbose(10,orte_rml_base_framework.framework_output,
                                        "\n Protocol  : %d", *protocol_ptr);
                }
                if( orte_get_attribute( prov, ORTE_PROV_NAME, (void **)&prov_name ,OPAL_STRING)) {
                    opal_output_verbose(10,orte_rml_base_framework.framework_output,
                                        "\n Provider name : %s",prov_name);
                } else {
                    opal_output_verbose(10,orte_rml_base_framework.framework_output,
                                    "\n Error in getting Provider name");
                }
            } else {
                opal_output_verbose(1,orte_rml_base_framework.framework_output,
                                    "\n %s:%d opal_value_unload() failed, opal_list* prov = %x",
                                   __FILE__,__LINE__,prov);
            }
            providers = (opal_value_t *)providers->super.opal_list_next;
        }
    } else {
        opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            "\n query_transports() returned Error ");
    }
    opal_output_verbose(10,orte_rml_base_framework.framework_output,
                        "\n End of print_transports_query() \n");
}
*/



/**
    conduit [in]: the ofi conduit_id that triggered the progress fn
 **/
__opal_attribute_always_inline__ static inline int
orte_rml_ofi_progress(ofi_transport_conduit_t* conduit)
{
    ssize_t ret;
    int count=0;    /* number of messages read and processed */
    struct fi_cq_data_entry wc = { 0 };
    struct fi_cq_err_entry error = { 0 };
    orte_rml_ofi_request_t *ofi_req;

    opal_output_verbose(1, orte_rml_base_framework.framework_output,
                         "%s orte_rml_ofi_progress called for OFI conduitid %d",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         conduit->conduit_id);
    /**
    * Read the work completions from the CQ.
    * From the completion's op_context, we get the associated OFI request.
    * Call the request's callback.
    */
    while (true) {
        /* Read the cq - that triggered the libevent to call this progress fn. */
        ret = fi_cq_read(conduit->cq, (void *)&wc, 1);
        if (0 < ret) {
            opal_output_verbose(1, orte_rml_base_framework.framework_output,
                         "%s cq read for OFI conduitid %d - wc.flags = %x",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         conduit->conduit_id, wc.flags);
            count++;
            // check the flags to see if this is a send-completion or receive
            if ( wc.flags & FI_SEND )
            {
                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                       "%s Send completion received on OFI conduitid %d",
                       ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                       conduit->conduit_id);
                if (NULL != wc.op_context) {
                    /* get the context from the wc and call the message handler */
                    ofi_req = TO_OFI_REQ(wc.op_context);
                    assert(ofi_req);
                    ret = orte_rml_ofi_send_callback(&wc, ofi_req);
                    if (ORTE_SUCCESS != ret) {
                        opal_output(orte_rml_base_framework.framework_output,
                        "Error returned by OFI send callback handler when a send completion was received on OFI conduit: %zd",
                         ret);
                    }
                }
            } else if ( (wc.flags & FI_RECV) && (wc.flags & FI_MULTI_RECV) ) {
                    opal_output_verbose(1, orte_rml_base_framework.framework_output,
                         "%s Received message on OFI conduitid %d - but buffer is consumed, need to repost",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         conduit->conduit_id);
                         // reposting buffer
                         ret = fi_recv(orte_rml_ofi.ofi_conduits[conduit->conduit_id].ep,
                          orte_rml_ofi.ofi_conduits[conduit->conduit_id].rxbuf,
                          orte_rml_ofi.ofi_conduits[conduit->conduit_id].rxbuf_size,
                          fi_mr_desc(orte_rml_ofi.ofi_conduits[conduit->conduit_id].mr_multi_recv),
                          0,&(conduit->rx_ctx1));
                        // call the receive message handler that will call the rml_base
                        ret = orte_rml_ofi_recv_handler(&wc, conduit->conduit_id);
                        if (ORTE_SUCCESS != ret) {
                            opal_output(orte_rml_base_framework.framework_output,
                            "Error returned by OFI Recv handler when handling the received message on the conduit: %zd",
                             ret);
                        }
            } else if ( wc.flags & FI_RECV )  {
                    opal_output_verbose(1, orte_rml_base_framework.framework_output,
                         "%s Received message on OFI conduitid %d",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         conduit->conduit_id);
                        // call the receive message handler that will call the rml_base
                        ret = orte_rml_ofi_recv_handler(&wc, conduit->conduit_id);
                        if (ORTE_SUCCESS != ret) {
                            opal_output(orte_rml_base_framework.framework_output,
                            "Error returned by OFI Recv handler when handling the received message on the OFI conduit: %zd",
                             ret);
                        }
            } else if ( wc.flags & FI_MULTI_RECV ) {
                    opal_output_verbose(1, orte_rml_base_framework.framework_output,
                         "%s Received buffer overrun message on OFI conduitid %d - need to repost",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         conduit->conduit_id);
                        // reposting buffer
                        ret = fi_recv(orte_rml_ofi.ofi_conduits[conduit->conduit_id].ep,
                          orte_rml_ofi.ofi_conduits[conduit->conduit_id].rxbuf,
                          orte_rml_ofi.ofi_conduits[conduit->conduit_id].rxbuf_size,
                          fi_mr_desc(orte_rml_ofi.ofi_conduits[conduit->conduit_id].mr_multi_recv),
                          0,&(conduit->rx_ctx1));
                        if (ORTE_SUCCESS != ret) {
                            opal_output(orte_rml_base_framework.framework_output,
                            "Error returned by OFI when reposting buffer on the OFI conduit: %zd",
                             ret);
                        }
            }else {
                opal_output_verbose(1,orte_rml_base_framework.framework_output,
                        "CQ has unhandled completion event with FLAG wc.flags = 0x%x",
                         wc.flags);
            }
        } else if (ret == -FI_EAVAIL) {
            /**
            * An error occured and is being reported via the CQ.
            * Read the error and forward it to the upper layer.
            */
            opal_output_verbose(1, orte_rml_base_framework.framework_output,
                         "%s cq_read for OFI conduitid %d  returned error 0x%x <%s>",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         conduit->conduit_id, ret,
                         fi_strerror((int) -ret) );
            ret = fi_cq_readerr(conduit->cq,
                                &error,
                                0);
            if (0 > ret) {
                opal_output_verbose(1,orte_rml_base_framework.framework_output,
                                "Error returned from fi_cq_readerr: %zd", ret);
            }
            assert(error.op_context);
            /* get the context from wc and call the error handler */
            ofi_req = TO_OFI_REQ(error.op_context);
            assert(ofi_req);
            ret = orte_rml_ofi_error_callback(&error, ofi_req);
            if (ORTE_SUCCESS != ret) {
               opal_output_verbose(1,orte_rml_base_framework.framework_output,
               "Error returned by request error callback: %zd",
               ret);
            }
            break;
        } else if (ret == -FI_EAGAIN){
            /**
             * The CQ is empty. Return.
             */
            opal_output_verbose(1, orte_rml_base_framework.framework_output,
                         "%s Empty cq for OFI conduitid %d,exiting from ofi_progress()",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         conduit->conduit_id );
            break;
        } else {
            opal_output_verbose(1, orte_rml_base_framework.framework_output,
                         "%s cq_read for OFI conduitid %d  returned error 0x%x <%s>",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         conduit->conduit_id, ret,
                         fi_strerror((int) -ret) );
            break;
        }
    }
    return count;
}


/*
 * call the ofi_progress() fn to read the cq
 *
 */
int cq_progress_handler(int sd, short flags, void *cbdata)
{
    ofi_transport_conduit_t* conduit = (ofi_transport_conduit_t*)cbdata;
    int count;

    opal_output_verbose(1, orte_rml_base_framework.framework_output,
                         "%s cq_progress_handler called for OFI conduitid %d",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         conduit->conduit_id);

      /* call the progress fn to read the cq and process the message
        *    for the conduit */
        count = orte_rml_ofi_progress(conduit);
        return count;
}


/* 
 * Returns the number of ofi-providers available
 */
int rml_ofi_component_init()
{
    int ret, fi_version;
    struct fi_info *hints, *fabric_info;
    struct fi_cq_attr cq_attr = {0};
    struct fi_av_attr av_attr = {0};
    size_t namelen;
    char   ep_name[FI_NAME_MAX]= {0};
    char *pmix_key;
    uint8_t fabric_id = 0, cur_conduit;

    opal_output_verbose(20,orte_rml_base_framework.framework_output,
                        "%s - Entering rml_ofi_component_init()",ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));


    if (init_done) {
         return orte_rml_ofi.conduit_open_num;
    }

 
    /**
     * Hints to filter providers
     * See man fi_getinfo for a list of all filters
     * mode:  Select capabilities MTL is prepared to support.
     *        In this case, MTL will pass in context into communication calls
     * ep_type:  reliable datagram operation
     * caps:     Capabilities required from the provider.
     *           Tag matching is specified to implement MPI semantics.
     * msg_order: Guarantee that messages with same tag are ordered.
     */

    hints = fi_allocinfo();
    if (!hints) {
        opal_output_verbose(1, orte_rml_base_framework.framework_output,
                            "%s:%d: Could not allocate fi_info\n",
                            __FILE__, __LINE__);
        return orte_rml_ofi.conduit_open_num;
    }

    /**
     * Refine filter for additional capabilities
     * endpoint type : Reliable datagram
     * threading:  Disable locking
     * control_progress:  enable async progress
     */
     hints->mode               = FI_CONTEXT;
     hints->ep_attr->type      = FI_EP_RDM;      /* Reliable datagram         */

     hints->domain_attr->threading        = FI_THREAD_UNSPEC;
     hints->domain_attr->control_progress = FI_PROGRESS_AUTO;
     hints->domain_attr->data_progress = FI_PROGRESS_AUTO;
     hints->domain_attr->av_type          = FI_AV_MAP;

    /**
     * FI_VERSION provides binary backward and forward compatibility support
     * Specify the version of OFI is coded to, the provider will select struct
     * layouts that are compatible with this version.
     */
     fi_version = FI_VERSION(1, 3);

    /**
     * fi_getinfo:  returns information about fabric  services for reaching a
     * remote node or service.  this does not necessarily allocate resources.
     * Pass NULL for name/service because we want a list of providers supported.
     */
     ret = fi_getinfo(fi_version,    /* OFI version requested                    */
                       NULL,          /* Optional name or fabric to resolve       */
                       NULL,          /* Optional service name or port to request */
                       0ULL,          /* Optional flag                            */
                       hints,        /* In: Hints to filter providers            */
                       &orte_rml_ofi.fi_info_list);   /* Out: List of matching providers          */
     if (0 != ret) {
        opal_output_verbose(1, orte_rml_base_framework.framework_output,
                            "%s:%d: fi_getinfo failed: %s\n",
                            __FILE__, __LINE__, fi_strerror(-ret));
    } else {

        /* added for debug purpose - Print the provider info
        print_transports_query();
        print_provider_list_info(orte_rml_ofi.fi_info_list);
        */

        /** create the OFI objects for each transport in the system
        *   (fi_info_list) and store it in the ofi_conduits array **/
        orte_rml_ofi.conduit_open_num = 0;   // start the conduit_id from 0
        for( fabric_info = orte_rml_ofi.fi_info_list ;
                 NULL != fabric_info && orte_rml_ofi.conduit_open_num < MAX_CONDUIT ; fabric_info = fabric_info->next)
        {
            opal_output_verbose(100,orte_rml_base_framework.framework_output,
                 "%s:%d beginning to add endpoint for OFIconduit_id=%d ",__FILE__,__LINE__,orte_rml_ofi.conduit_open_num);
            print_provider_info(fabric_info);
            cur_conduit = orte_rml_ofi.conduit_open_num;
            orte_rml_ofi.ofi_conduits[cur_conduit].conduit_id = orte_rml_ofi.conduit_open_num ;
            orte_rml_ofi.ofi_conduits[cur_conduit].fabric_info = fabric_info;

            // set FI_MULTI_RECV flag for all recv operations
            fabric_info->rx_attr->op_flags = FI_MULTI_RECV;
            /**
            * Open fabric
            * The getinfo struct returns a fabric attribute struct that can be used to
            * instantiate the virtual or physical network. This opens a "fabric
            * provider". See man fi_fabric for details.
            */

            ret = fi_fabric(fabric_info->fabric_attr,        /* In:  Fabric attributes             */
                             &orte_rml_ofi.ofi_conduits[cur_conduit].fabric,  /* Out: Fabric handle */
                             NULL);                          /* Optional context for fabric events */
            if (0 != ret) {
                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                "%s:%d: fi_fabric failed: %s\n",
                                __FILE__, __LINE__, fi_strerror(-ret));
                    orte_rml_ofi.ofi_conduits[cur_conduit].fabric = NULL;
                    /* abort this current transport, but check if next transport can be opened */
                    continue;
            }


            /**
            * Create the access domain, which is the physical or virtual network or
            * hardware port/collection of ports.  Returns a domain object that can be
            * used to create endpoints.  See man fi_domain for details.
            */
            ret = fi_domain(orte_rml_ofi.ofi_conduits[cur_conduit].fabric,   /* In:  Fabric object */
                             fabric_info,                                   /* In:  Provider          */
                             &orte_rml_ofi.ofi_conduits[cur_conduit].domain, /* Out: Domain oject  */
                              NULL);                                        /* Optional context for domain events */
            if (0 != ret) {
                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: fi_domain failed: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                orte_rml_ofi.ofi_conduits[cur_conduit].domain = NULL;
                /* abort this current transport, but check if next transport can be opened */
                continue;
            }

            /**
            * Create a transport level communication endpoint.  To use the endpoint,
            * it must be bound to completion counters or event queues and enabled,
            * and the resources consumed by it, such as address vectors, counters,
            * completion queues, etc.
            * see man fi_endpoint for more details.
            */
            ret = fi_endpoint(orte_rml_ofi.ofi_conduits[cur_conduit].domain, /* In:  Domain object   */
                                fabric_info,                                /* In:  Provider        */
                                &orte_rml_ofi.ofi_conduits[cur_conduit].ep,  /* Out: Endpoint object */
                              NULL);                                        /* Optional context     */
            if (0 != ret) {
                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: fi_endpoint failed: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(cur_conduit);
                /* abort this current transport, but check if next transport can be opened */
                continue;
            }

            /**
            * Save the maximum inject size.
            */
             //orte_rml_ofi.max_inject_size = prov->tx_attr->inject_size;

            /**
            * Create the objects that will be bound to the endpoint.
            * The objects include:
            *     - completion queue for events
            *     - address vector of other endpoint addresses
            *     - dynamic memory-spanning memory region
            */
            cq_attr.format = FI_CQ_FORMAT_DATA;
            cq_attr.wait_obj = FI_WAIT_FD;
            cq_attr.wait_cond = FI_CQ_COND_NONE;
            ret = fi_cq_open(orte_rml_ofi.ofi_conduits[cur_conduit].domain,
                             &cq_attr, &orte_rml_ofi.ofi_conduits[cur_conduit].cq, NULL);
            if (ret) {
                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                "%s:%d: fi_cq_open failed: %s\n",
                                __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(cur_conduit);
                /* abort this current transport, but check if next transport can be opened */
                continue;
            }

            /**
            * The remote fi_addr will be stored in the ofi_endpoint struct.
            * So, we use the AV in "map" mode.
            */
            av_attr.type = FI_AV_MAP;
            ret = fi_av_open(orte_rml_ofi.ofi_conduits[cur_conduit].domain,
                             &av_attr, &orte_rml_ofi.ofi_conduits[cur_conduit].av, NULL);
            if (ret) {
                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: fi_av_open failed: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(cur_conduit);
                /* abort this current transport, but check if next transport can be opened */
                continue;
            }

            /**
            * Bind the CQ and AV to the endpoint object.
            */
            ret = fi_ep_bind(orte_rml_ofi.ofi_conduits[cur_conduit].ep,
                            (fid_t)orte_rml_ofi.ofi_conduits[cur_conduit].cq,
                            FI_SEND | FI_RECV);
            if (0 != ret) {
                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: fi_bind CQ-EP failed: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(cur_conduit);
                /* abort this current transport, but check if next transport can be opened */
                continue;
            }

            ret = fi_ep_bind(orte_rml_ofi.ofi_conduits[cur_conduit].ep,
                            (fid_t)orte_rml_ofi.ofi_conduits[cur_conduit].av,
                            0);
            if (0 != ret) {
                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: fi_bind AV-EP failed: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(cur_conduit);
                /* abort this current transport, but check if next transport can be opened */
                continue;
            }

            /**
            * Enable the endpoint for communication
            * This commits the bind operations.
            */
            ret = fi_enable(orte_rml_ofi.ofi_conduits[cur_conduit].ep);
            if (0 != ret) {
                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: fi_enable failed: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(cur_conduit);
                /* abort this current transport, but check if next transport can be opened */
                continue;
            }
            opal_output_verbose(10,orte_rml_base_framework.framework_output,
                            "%s:%d ep enabled for conduit_id - %d ",__FILE__,__LINE__,orte_rml_ofi.ofi_conduits[cur_conduit].conduit_id);


            /**
            * Get our address and publish it with modex.
            **/
            orte_rml_ofi.ofi_conduits[cur_conduit].epnamelen = sizeof (orte_rml_ofi.ofi_conduits[cur_conduit].ep_name);
            ret = fi_getname((fid_t)orte_rml_ofi.ofi_conduits[cur_conduit].ep,
                            &orte_rml_ofi.ofi_conduits[cur_conduit].ep_name[0],
                            &orte_rml_ofi.ofi_conduits[cur_conduit].epnamelen);
            if (ret) {
                        opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: fi_getname failed: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(cur_conduit);
                /* abort this current transport, but check if next transport can be opened */
                  continue;
            }

            /* Register the ofi address of this peer with PMIX server only if it is a user process  /
             * for daemons the set/get_contact_info is used to exchange this information */
            if (ORTE_PROC_IS_APP) {
                switch ( orte_rml_ofi.ofi_conduits[cur_conduit].fabric_info->addr_format)
                {
                    case  FI_SOCKADDR_IN :
                        opal_output_verbose(1,orte_rml_base_framework.framework_output,
                            "%s:%d In FI_SOCKADDR_IN.  ",__FILE__,__LINE__);
                        /*  Address is of type sockaddr_in (IPv4) */
                        opal_output_verbose(1,orte_rml_base_framework.framework_output,
                            "%s sending Opal modex string for conduit_it %d, epnamelen = %d  ",
                            ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),cur_conduit,orte_rml_ofi.ofi_conduits[cur_conduit].epnamelen);
                        /*[debug] - print the sockaddr - port and s_addr */
                        struct sockaddr_in* ep_sockaddr = (struct sockaddr_in*)orte_rml_ofi.ofi_conduits[cur_conduit].ep_name;
                        opal_output_verbose(1,orte_rml_base_framework.framework_output,
                            "%s port = 0x%x, InternetAddr = %s  ",
                            ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),ntohs(ep_sockaddr->sin_port),inet_ntoa(ep_sockaddr->sin_addr));
                        /*[end debug]*/
                        asprintf(&pmix_key,"%s%d",OPAL_RML_OFI_FI_SOCKADDR_IN,cur_conduit);
                        opal_output_verbose(10, orte_rml_base_framework.framework_output,
                         "%s calling OPAL_MODEX_SEND_STRING key - %s ", 
                          ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), pmix_key );
                        OPAL_MODEX_SEND_STRING( ret, OPAL_PMIX_GLOBAL,
                                            pmix_key,
                                            orte_rml_ofi.ofi_conduits[cur_conduit].ep_name,
                                            orte_rml_ofi.ofi_conduits[cur_conduit].epnamelen);
                        free(pmix_key);
                        if (ORTE_SUCCESS != ret) {
                            opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: OPAL_MODEX_SEND failed: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                            free_conduit_resources(cur_conduit);
                            /*abort this current transport, but check if next transport can be opened*/
                            continue;
                        }
                        break;
                    case  FI_ADDR_PSMX :
                        opal_output_verbose(1,orte_rml_base_framework.framework_output,
                            "%s:%d In FI_ADDR_PSMX.  ",__FILE__,__LINE__);
                        /*   Address is of type Intel proprietery PSMX */
                        OPAL_MODEX_SEND_STRING( ret, OPAL_PMIX_GLOBAL,
                                           OPAL_RML_OFI_FI_ADDR_PSMX,orte_rml_ofi.ofi_conduits[cur_conduit].ep_name,
                                                    orte_rml_ofi.ofi_conduits[cur_conduit].epnamelen);
                        opal_output_verbose(1,orte_rml_base_framework.framework_output,
                            "%s:%d Opal modex send completed for FI_ADDR_PSMX.  ",__FILE__,__LINE__);
                        if (ORTE_SUCCESS != ret) {
                            opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                "%s:%d: OPAL_MODEX_SEND failed: %s\n",
                                 __FILE__, __LINE__, fi_strerror(-ret));
                            free_conduit_resources(cur_conduit);
                            /*abort this current transport, but check if next transport can be opened*/
                            continue;
                        }
                        break;
                    default:
                        opal_output_verbose(1,orte_rml_base_framework.framework_output,
                        "%s:%d ERROR: Cannot register address, Unhandled addr_format - %d, ep_name - %s  ",
                        __FILE__,__LINE__,orte_rml_ofi.ofi_conduits[cur_conduit].fabric_info->addr_format,
                        orte_rml_ofi.ofi_conduits[cur_conduit].ep_name);
                        free_conduit_resources(cur_conduit);
                        /*abort this current transport, but check if next transport can be opened*/
                        continue;
                }
            }   

            /**
            * Set the ANY_SRC address.
            */
            orte_rml_ofi.any_addr = FI_ADDR_UNSPEC;

            /**
            *  Allocate tx,rx buffers and Post a multi-RECV buffer for each endpoint
            **/
            //[TODO later]  For now not considering ep_attr prefix_size (add this later)
            orte_rml_ofi.ofi_conduits[cur_conduit].rxbuf_size = MIN_MULTI_BUF_SIZE * MULTI_BUF_SIZE_FACTOR;
            orte_rml_ofi.ofi_conduits[cur_conduit].rxbuf = malloc(orte_rml_ofi.ofi_conduits[cur_conduit].rxbuf_size);

            ret = fi_mr_reg(orte_rml_ofi.ofi_conduits[cur_conduit].domain,
                            orte_rml_ofi.ofi_conduits[cur_conduit].rxbuf,
                            orte_rml_ofi.ofi_conduits[cur_conduit].rxbuf_size,
                            FI_RECV, 0, 0, 0, &orte_rml_ofi.ofi_conduits[cur_conduit].mr_multi_recv,
                            &orte_rml_ofi.ofi_conduits[cur_conduit].rx_ctx1);
            if (ret) {
                        opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: fi_mr_reg failed: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(cur_conduit);
                /* abort this current transport, but check if next transport can be opened */
                  continue;
            }

            ret = fi_setopt(&orte_rml_ofi.ofi_conduits[cur_conduit].ep->fid, FI_OPT_ENDPOINT, FI_OPT_MIN_MULTI_RECV,
                            &orte_rml_ofi.min_ofi_recv_buf_sz, sizeof(orte_rml_ofi.min_ofi_recv_buf_sz) );
            if (ret) {
                        opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: fi_setopt failed: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(cur_conduit);
                /* abort this current transport, but check if next transport can be opened */
                  continue;
            }

            ret = fi_recv(orte_rml_ofi.ofi_conduits[cur_conduit].ep,
                          orte_rml_ofi.ofi_conduits[cur_conduit].rxbuf,
                          orte_rml_ofi.ofi_conduits[cur_conduit].rxbuf_size,
                          fi_mr_desc(orte_rml_ofi.ofi_conduits[cur_conduit].mr_multi_recv),
                          0,&orte_rml_ofi.ofi_conduits[cur_conduit].rx_ctx1);
            if (ret) {
                        opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: fi_recv failed: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(cur_conduit);
                /* abort this current transport, but check if next transport can be opened */
                  continue;
            }
            /**
            *  get the fd  and register the progress fn
            **/
            ret = fi_control(&orte_rml_ofi.ofi_conduits[cur_conduit].cq->fid, FI_GETWAIT,
                            (void *) &orte_rml_ofi.ofi_conduits[cur_conduit].fd);
            if (0 != ret) {
                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: fi_control failed to get fd: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(cur_conduit);
                /* abort this current transport, but check if next transport can be opened */
                  continue;
            }

            /* - create the event  that will wait on the fd*/
            /* use the opal_event_set to do a libevent set on the fd
            *  so when something is available to read, the cq_porgress_handler
            *  will be called */
            opal_event_set(orte_event_base,
                       &orte_rml_ofi.ofi_conduits[cur_conduit].progress_event,
                       orte_rml_ofi.ofi_conduits[cur_conduit].fd,
                       OPAL_EV_READ|OPAL_EV_PERSIST,
                       cq_progress_handler,
                       &orte_rml_ofi.ofi_conduits[cur_conduit]);
            opal_event_add(&orte_rml_ofi.ofi_conduits[cur_conduit].progress_event, 0);
            orte_rml_ofi.ofi_conduits[cur_conduit].progress_ev_active = true;

            /** update the number of conduits in the ofi_conduits[] array **/
            opal_output_verbose(1,orte_rml_base_framework.framework_output,
                 "%s:%d Conduit id - %d created ",__FILE__,__LINE__,orte_rml_ofi.conduit_open_num);
            orte_rml_ofi.conduit_open_num++;
        }
        if (fabric_info != NULL &&  orte_rml_ofi.conduit_open_num >= MAX_CONDUIT ) {
            opal_output_verbose(1,orte_rml_base_framework.framework_output,
                 "%s:%d fi_getinfo list not fully parsed as MAX_CONDUIT - %d reached ",__FILE__,__LINE__,orte_rml_ofi.conduit_open_num);
        }


    }
    /**
     * Free providers info since it's not needed anymore.
    */
    fi_freeinfo(hints);
    hints = NULL;
    /* check if atleast one conduit was successfully opened */
    if (0 < orte_rml_ofi.conduit_open_num ) {
        opal_output_verbose(1,orte_rml_base_framework.framework_output,
                    "%s:%d OFIconduits openened=%d returning orte_rml_ofi.api",
                    __FILE__,__LINE__,orte_rml_ofi.conduit_open_num);

        OBJ_CONSTRUCT(&orte_rml_ofi.recv_msg_queue_list,opal_list_t);
    } else {
        opal_output_verbose(1,orte_rml_base_framework.framework_output,
                    "%s:%d Failed to open any OFIconduits",__FILE__,__LINE__);
    }

    return orte_rml_ofi.conduit_open_num;
}

/* return : the ofi_prov_id that corresponds to the transport requested by the attributes
            if transport is not found RML_OFI_CONDUIT_ID_INVALID is returned.
    @[in]attributes  : the attributes passed in to open_conduit reg the transport requested
*/
int get_ofi_prov_id( opal_list_t *attributes)
{

    int ofi_prov_id = RML_OFI_CONDUIT_ID_INVALID, prov_num=0;
    char *provider = NULL, *transport = NULL;
    char *ethernet="sockets", *fabric="psm2"; 
    struct fi_info *cur_fi;

    /* check the list of attributes to see if we should respond
     * Attribute should have ORTE_RML_TRANSPORT_ATTRIB key
     * with values "ethernet" or "fabric"
     * (or)  ORTE_RML_OFI_PROV_NAME key with values "socket" or "OPA"
     * if both above attributes are missing return failure
     */
    if (orte_get_attribute(attributes, ORTE_RML_TRANSPORT_ATTRIB, (void**)&transport, OPAL_STRING) )    {
        if( 0 == strcmp( transport, "ethernet") ) {
            provider = ethernet;
        } else if ( 0 == strcmp( transport, "fabric") ) {
            provider = fabric;
        }
    }
    /* if from the transport we don't know which provider we want, then check for the ORTE_RML_OFI_PROV_NAME_ATTRIB */
    if ( NULL == provider) {
       orte_get_attribute(attributes, ORTE_RML_PROVIDER_ATTRIB, (void**)&provider, OPAL_STRING);
    }
    if (NULL != provider)
    {
        // loop the orte_rml_ofi.conduits[] and find the provider name that matches
        for ( prov_num = 0; prov_num < orte_rml_ofi.conduit_open_num && ofi_prov_id == RML_OFI_CONDUIT_ID_INVALID ; prov_num++ ) {
            cur_fi = orte_rml_ofi.ofi_conduits[prov_num].fabric_info;
            opal_output_verbose(20,orte_rml_base_framework.framework_output,
               "%s - get_ofi_prov_id() -> comparing %s = %s ",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),provider,cur_fi->fabric_attr->prov_name);
            if ( strcmp(provider,cur_fi->fabric_attr->prov_name) == 0) {
                ofi_prov_id = prov_num;
            }
        }

    }

    opal_output_verbose(20,orte_rml_base_framework.framework_output,
                    "%s - get_ofi_prov_id(), returning ofi_prov_id=%d ",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),ofi_prov_id);
    return ofi_prov_id;
}

/* 
 * Allocate a new module and initialise ofi_prov information
 * for the requested provider and return the module *
 */
static orte_rml_base_module_t* make_module( int ofi_prov_id)
{
    orte_rml_ofi_module_t *mod = NULL;

    opal_output_verbose(20,orte_rml_base_framework.framework_output,
                    "%s - rml_ofi make_module() begin ",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));

    if ( RML_OFI_CONDUIT_ID_INVALID == ofi_prov_id) {
        opal_output_verbose(20,orte_rml_base_framework.framework_output,
                    "%s - open_conduit did not select any ofi provider, returning NULL ",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
        return NULL;
    }


    /* create a new module   */
    mod = (orte_rml_ofi_module_t*)calloc(1,sizeof(orte_rml_ofi_module_t));
    if (NULL == mod) {
        opal_output_verbose(20,orte_rml_base_framework.framework_output,
                    "%s - Module allocation failed, returning NULL ",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
        return NULL;
    }
    /* copy the APIs over to it and the OFI provider information */
    memcpy(mod, &orte_rml_ofi, sizeof(orte_rml_ofi_module_t));
    /*  setup the remaining data locations in mod to reflect the current conduit*/
    mod->cur_transport_id = ofi_prov_id;

    return (orte_rml_base_module_t*)mod;
}


/* Order of attributes honoring          *
* ORTE_RML_INCLUDE_COMP_ATTRIB           *
* ORTE_RML_EXCLUDE_COMP_ATTRIB           *
* ORTE_RML_TRANSPORT_ATTRIB              *
* ORTE_RML_PROVIDER_ATTRIB               */
static orte_rml_base_module_t* open_conduit(opal_list_t *attributes)
{
char *comp_attrib = NULL;
    char **comps;
    int i;
    orte_attribute_t *attr;

    opal_output_verbose(20,orte_rml_base_framework.framework_output,
                    "%s - Entering rml_ofi_open_conduit()",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));

    /*[TODO] Limiting current implementation to APP PROCs.  The PMIX component does not get initialised for
     *        ORTE_DAEMON_PROC so the OPAL_MODEX_SEND_STRING (opal_pmix.put) fails during initialisation,
     *        this needs to be fixed to support DAEMON PROC to use ofi
    */
    /*if (!ORTE_PROC_IS_APP) {
        opal_output_verbose(20,orte_rml_base_framework.framework_output,
                    "%s - Currently not handling Daemons, returning NULL",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
        return NULL;
    }*/

    /* Open all ofi endpoints */
    if (!init_done) {
        rml_ofi_component_init();
        init_done = true;
    }

    /* check if atleast 1 ofi provider is initialised */
    if ( 0 >= orte_rml_ofi.conduit_open_num) {
        opal_output_verbose(20,orte_rml_base_framework.framework_output,
                    "%s - Init did not open any Ofi endpoints, returning NULL",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
        return NULL;
    }    

    /* someone may require this specific component, so look for "ofi" */
    if (orte_get_attribute(attributes, ORTE_RML_INCLUDE_COMP_ATTRIB, (void**)&comp_attrib, OPAL_STRING) &&
        NULL != comp_attrib) {
        /* they specified specific components - could be multiple */
        comps = opal_argv_split(comp_attrib, ',');
        for (i=0; NULL != comps[i]; i++) {
            if (0 == strcmp(comps[i], "ofi")) {
                /* we are a candidate,  */
                opal_argv_free(comps);
                return make_module(get_ofi_prov_id(attributes));
            }
        }
        /* we are not a candidate */
        opal_argv_free(comps);
        return NULL;
    } else if (orte_get_attribute(attributes, ORTE_RML_EXCLUDE_COMP_ATTRIB, (void**)&comp_attrib, OPAL_STRING) &&
               NULL != comp_attrib) {
        /* see if we are on the list */
        comps = opal_argv_split(comp_attrib, ',');
        for (i=0; NULL != comps[i]; i++) {
            if (0 == strcmp(comps[i], "ofi")) {
                /* we cannot be a candidate */
                opal_argv_free(comps);
                return NULL;
            }
        }
    }

    /* Alternatively, check the attributes to see if we qualify - we only handle
     * "pt2pt" */
    OPAL_LIST_FOREACH(attr, attributes, orte_attribute_t) {

    }

    opal_output_verbose(20,orte_rml_base_framework.framework_output,
                    "%s - ofi is not a candidate as per attributes, returning NULL",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
    /* if we get here, we cannot handle it */
    return NULL;
}



static void orte_rml_ofi_fini(void *mod)
{
    opal_list_item_t *item;
    uint8_t conduit_id;
    orte_rml_ofi_module_t* ofi_mod = (orte_rml_ofi_module_t *)mod;


    opal_output_verbose(1,orte_rml_base_framework.framework_output,
                        " %s -  orte_rml_ofi_fini() begin ",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));

    //freeing module* created by open_conduit() is done in rml_ofi_component_close()

    opal_output_verbose(1,orte_rml_base_framework.framework_output,
                        " %s -  orte_rml_ofi_fini() completed ",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
}


static void pr_cons(orte_rml_ofi_peer_t *ptr)
{
    ptr->socket_ep = NULL;
    ptr->socket_ep_len = 0;
    ptr->psmx_ep = NULL;
    ptr->psmx_ep_len = 0;
}
static void pr_des(orte_rml_ofi_peer_t *ptr)
{
    if ( 0 < ptr->socket_ep_len)
        free( ptr->socket_ep);
    if ( NULL != ptr->psmx_ep )
        free( ptr->psmx_ep);
}
OBJ_CLASS_INSTANCE(orte_rml_ofi_peer_t,
                   opal_object_t,
                   pr_cons, pr_des);


/* The returned string will be of format - "<process-name>;ofi-socket:<addr_format,ip,portaddr>;ofi-<provider2>:<prov2epname>" */
static char* ofi_get_contact_info(void)
{
    char *turi, *final=NULL, *tmp, *addrtype;
    size_t len = 0;
    int rc=ORTE_SUCCESS, cur_conduit=0;
    bool one_added = false;
    struct sockaddr_in* ep_sockaddr;
    
    /* start with our process name */
    if (ORTE_SUCCESS != (rc = orte_util_convert_process_name_to_string(&final, ORTE_PROC_MY_NAME))) {
        /* [TODO] ORTE_ERROR_LOG(rc);*/
        return final;
    }
    len = strlen(final);

    /* The returned string will be of format - "<process-name>;ofi-socket:<sin_family,sin_addr,sin_port>;ofi-<provider2>:<prov2epname>" */
    for( cur_conduit=0; cur_conduit < orte_rml_ofi.conduit_open_num ; cur_conduit++ ) {
            switch ( orte_rml_ofi.ofi_conduits[cur_conduit].fabric_info->addr_format) {
                case  FI_SOCKADDR_IN :
                    ep_sockaddr = (struct sockaddr_in*)orte_rml_ofi.ofi_conduits[cur_conduit].ep_name;
                    asprintf(&addrtype, SOCKADDR);
                    asprintf(&turi,"%d,%s,%d",ep_sockaddr->sin_family,inet_ntoa(ep_sockaddr->sin_addr),ntohs(ep_sockaddr->sin_port));
                    
                    break;
                case  FI_ADDR_PSMX :
                    asprintf(&addrtype, PSMXADDR); 
                    /* [TBD]  The ep_name has to be converted to PSMX specific address format struct for printing
                    copy the endpoint name as is to length specified - this fails in sprintf as the ep_name is a
                    binary content
                    turi = calloc(orte_rml_ofi.ofi_conduits[cur_conduit].epnamelen + 1, sizeof(char));
                    memcpy( turi, orte_rml_ofi.ofi_conduits[cur_conduit].ep_name, orte_rml_ofi.ofi_conduits[cur_conduit].epnamelen);
                    turi[ (orte_rml_ofi.ofi_conduits[cur_conduit].epnamelen + 1) ] = '\0';*/
		    asprintf(&turi, "psmx addr");
                    break;
                default:
                    opal_output_verbose(1,orte_rml_base_framework.framework_output,
                    "%s:%d ERROR: Cannot register address, Unhandled addr_format - %d",
                     __FILE__,__LINE__,orte_rml_ofi.ofi_conduits[cur_conduit].fabric_info->addr_format);
                     /*abort this current transport, but check next transport */
                     continue;                    
            }    
            opal_output_verbose(20,orte_rml_base_framework.framework_output,
                        "%s - cur_conduit = %d, addrtype = %s ", ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),cur_conduit,addrtype);
            /* Add to the final string - the ofi addrtype and the epname   */
            asprintf(&tmp, "%s;%s:%s", final,addrtype, turi);

            free(addrtype);
            free(turi);
            free(final);
            final = tmp;
            len = strlen(final); 
            /* [TODO] check string length to not exceed limit   */
    }
    opal_output_verbose(10,orte_rml_base_framework.framework_output,
                        "[%s] get_contact_info returns string - %s ", ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),final);              
    return final;
}


static void ofi_set_contact_info (const char *uri)
{
    char *uris;

    opal_output_verbose(5, orte_rml_base_framework.framework_output,
                        "%s: OFI set_contact_info to uri %s",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                        (NULL == uri) ? "NULL" : uri);

    /* if the request doesn't contain a URI, then we
     * have an error
     */
    if (NULL == uri) {
        opal_output(0, "%s: NULL URI", ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
        /* [TODO] ORTE_FORCED_TERMINATE(1);*/
        return;
    }

    uris = strdup(uri);
    process_uri(uris);
    free(uris); 
    return;
}

static void process_uri( char *uri)
{
    orte_process_name_t peer;
    char *cptr, *ofiuri;
    char **uris=NULL;
    int rc, i=0, tot_reqd = 1, tot_found = 0;
    uint64_t ui64;
    orte_rml_ofi_peer_t *pr;
    void *ep_name;
    struct sockaddr_in* ep_sockaddr;
    /* find the first semi-colon in the string */
    cptr = strchr(uri, ';');
    if (NULL == cptr) {
        /* got a problem - there must be at least two fields,
         * the first containing the process name of our peer
         * and all others containing the OOB contact info
         */
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return;
    }
    *cptr = '\0';
    cptr++;

    /* the first field is the process name, so convert it */
    orte_util_convert_string_to_process_name(&peer, uri);

    /* if the peer is us, no need to go further as we already
     * know our own contact info
     */
    if (peer.jobid == ORTE_PROC_MY_NAME->jobid &&
        peer.vpid == ORTE_PROC_MY_NAME->vpid) {
        opal_output_verbose(5, orte_rml_base_framework.framework_output,
                            "%s:OFI set_contact_info peer %s is me",
                            ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                            ORTE_NAME_PRINT(&peer));
          //skip adding to hashtable for HNP
        if (!ORTE_PROC_IS_HNP) {
           return;
        } else {
                opal_output_verbose(5, orte_rml_base_framework.framework_output,
                            "%s:OFI set_contact_info - HNP process so proceeding to add to hashtable",
                            ORTE_NAME_PRINT(ORTE_PROC_MY_NAME) );
        }
    }

    /* split the rest of the uri into component parts */
    uris = opal_argv_split(cptr, ';');

    /* get the peer object for this process */
    memcpy(&ui64, (char*)&peer, sizeof(uint64_t));
    if (OPAL_SUCCESS != opal_hash_table_get_value_uint64(&orte_rml_ofi.peers,
                                                         ui64, (void**)&pr) ||
        NULL == pr) {
        pr = OBJ_NEW(orte_rml_ofi_peer_t);
        /* populate the peer object with the ofi addresses *
         * [TBD] tot_reqd is hard-coded to 1 for test purpose, the logic needs to be changed *
         * to send some conduit specific information in string as identifier for the addr foll it *
         * store entire string in hash table and then interpret ofi address based on conduit info *
         * either her or in send logic.  The peer object need to be modified to hold conduit info
         * and ofi address in a list */
        for(i=0; NULL != uris[i] && tot_found < tot_reqd; i++) {
            ofiuri = strdup(uris[i]);
            if (NULL == ofiuri) {
                opal_output_verbose(2, orte_rml_base_framework.framework_output,
                                "%s rml:ofi: out of memory",
                                 ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
                continue;
            } 
            /* Handle the specific OFI address types in the uri - SOCKADDR(ofi-socket), PSMXADDR etc */       
            if (0 == strncmp(ofiuri, SOCKADDR, strlen(SOCKADDR)) ) {
                /* allocate and initialise the peer object to be inserted in hashtable */
                pr->socket_ep_len = sizeof(struct sockaddr_in);
                ep_sockaddr = malloc( sizeof ( struct sockaddr_in) );
                /* ofiuri for socket provider is of format - ofi-socket:<sin_family,sin_addr,sin_port> */
                convert_to_sockaddr(ofiuri, ep_sockaddr);              
                pr->socket_ep = (void *)ep_sockaddr;
                tot_found++;
            } else if ( 0 == strncmp(ofiuri, PSMXADDR, strlen(PSMXADDR)) ){
                pr->psmx_ep_len = strlen(ofiuri) - strlen(PSMXADDR) -1;
                pr->psmx_ep = calloc ( pr->psmx_ep_len , sizeof(char) );
                ep_name = ofiuri + strlen(PSMXADDR) + 1;
                memcpy( pr->psmx_ep, ep_name, pr->psmx_ep_len );
                tot_found++;
            }
            free( ofiuri);
        }
        /* if atleast one OFI address is known for peer insert it */
        if( 1 <= tot_found ) {
            if (OPAL_SUCCESS != 
               (rc = opal_hash_table_set_value_uint64(&orte_rml_ofi.peers, ui64, (void*)pr))) {
                opal_output_verbose(5, orte_rml_base_framework.framework_output,
                    "%s: ofi peer address insertion failed for peer %s ",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                    ORTE_NAME_PRINT(&peer));
                ORTE_ERROR_LOG(rc);
            }
            opal_output_verbose(5, orte_rml_base_framework.framework_output,
                    "%s: ofi peer address inserted for peer %s ",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                    ORTE_NAME_PRINT(&peer));
            opal_output_verbose(5, orte_rml_base_framework.framework_output,
                    "%s: ofi sock address length = %d ",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                    pr->socket_ep_len);
            struct sockaddr_in* ep_sockaddr = (struct sockaddr_in*)pr->socket_ep;
            opal_output_verbose(1,orte_rml_base_framework.framework_output,
                            "%s OFI set_name() port = 0x%x, InternetAddr = %s  ",
                            ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),ntohs(ep_sockaddr->sin_port),inet_ntoa(ep_sockaddr->sin_addr));
        }        
    }
    opal_output_verbose(1,orte_rml_base_framework.framework_output,
                    "%s OFI end of set_contact_info()",
                    ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
    opal_argv_free(uris);
    return;
}


/* converts the socket uri returned by get_contact_info into sockaddr_in */
void convert_to_sockaddr( char *ofiuri, struct sockaddr_in* ep_sockaddr)
{
char *tmp, *sin_fly, *sin_port, *sin_addr;
short port;  
int   res;              
    tmp = strchr(ofiuri,':');
    sin_fly = tmp+1;
    tmp = strchr(sin_fly,',');
    sin_addr = tmp+1;
    *tmp = '\0';
    tmp = strchr(sin_addr,',');
    sin_port = tmp + 1;
    *tmp = '\0';

    opal_output_verbose(1,orte_rml_base_framework.framework_output,
                        "%s OFI convert_to_sockaddr uri strings got -> family = %s, InternetAddr = %s, port = %s  ",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),sin_fly,sin_addr, sin_port);
    ep_sockaddr->sin_family = atoi( sin_fly );
    port = atoi( sin_port);
    ep_sockaddr->sin_port   = htons(port);
    res = inet_aton(sin_addr,(struct in_addr *)&ep_sockaddr->sin_addr);

            opal_output_verbose(1,orte_rml_base_framework.framework_output,
                            "%s OFI convert_to_sockaddr() port = 0x%x, InternetAddr = %s  ",
                            ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),ntohs(ep_sockaddr->sin_port),inet_ntoa(ep_sockaddr->sin_addr));
} 
