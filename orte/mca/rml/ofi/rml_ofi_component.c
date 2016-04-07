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

static int my_priority=3;  // [A] - need to make this a #define or constant and also figure what these numbers mean (allowed range etc)
static orte_rml_base_module_t* rml_ofi_init(int* priority);  // [A] changed to refer to use internal base orte_rml_base_module_t
static int rml_ofi_open(void);
static int rml_ofi_close(void);


/**
 * component definition
 */
orte_rml_component_t mca_rml_ofi_component = {
      /* First, the mca_base_component_t struct containing meta
         information about the component itself */

    .rml_version = {
        ORTE_RML_BASE_VERSION_2_0_0,

        .mca_component_name = "ofi",
        MCA_BASE_MAKE_VERSION(component, ORTE_MAJOR_VERSION, ORTE_MINOR_VERSION,
                              ORTE_RELEASE_VERSION),
        .mca_open_component = rml_ofi_open,
        .mca_close_component = rml_ofi_close,
     },
    .rml_data = {
        /* The component is checkpoint ready */
        MCA_BASE_METADATA_PARAM_CHECKPOINT
    },
    .rml_init = rml_ofi_init,
};

orte_rml_ofi_module_t orte_rml_ofi = {
    {
         .enable_comm = orte_rml_ofi_enable_comm,   //  [A] should we be calling this ?
        .finalize = orte_rml_ofi_fini,
	.query_transports = orte_rml_ofi_query_transports,

    }
};

/* Local variables */
static bool init_done = false;

static int
rml_ofi_open(void)
{
     /* Initialise endpoint and all queues */
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
    }
    opal_output_verbose(1,orte_rml_base_framework.framework_output," from %s:%d rml_ofi_open()",__FILE__,__LINE__);
    return ORTE_SUCCESS;
}


void free_conduit_resources( int conduit_id)
{

	if (orte_rml_ofi.ofi_conduits[conduit_id].mr_multi_recv) {
         CLOSE_FID(orte_rml_ofi.ofi_conduits[conduit_id].mr_multi_recv);
    }
    if (orte_rml_ofi.ofi_conduits[conduit_id].av) {
         CLOSE_FID(orte_rml_ofi.ofi_conduits[conduit_id].av);
    }
    if (orte_rml_ofi.ofi_conduits[conduit_id].cq) {
         CLOSE_FID(orte_rml_ofi.ofi_conduits[conduit_id].cq);
    }
    if (orte_rml_ofi.ofi_conduits[conduit_id].ep) {
         CLOSE_FID(orte_rml_ofi.ofi_conduits[conduit_id].ep);
    }
    if (orte_rml_ofi.ofi_conduits[conduit_id].domain) {
         CLOSE_FID(orte_rml_ofi.ofi_conduits[conduit_id].domain);
    }
    if (orte_rml_ofi.ofi_conduits[conduit_id].fabric) {
         CLOSE_FID(orte_rml_ofi.ofi_conduits[conduit_id].fabric);
    }
    if (orte_rml_ofi.ofi_conduits[conduit_id].rxbuf) {
         free(orte_rml_ofi.ofi_conduits[conduit_id].rxbuf);
    }

	
	orte_rml_ofi.ofi_conduits[conduit_id].ep_name[0] = 0;
	orte_rml_ofi.ofi_conduits[conduit_id].epnamelen = 0;
	orte_rml_ofi.ofi_conduits[conduit_id].rxbuf = NULL;
	orte_rml_ofi.ofi_conduits[conduit_id].rxbuf_size = 0;
	
    return;
}


static int
rml_ofi_close(void)
{
    opal_output_verbose(10,orte_rml_base_framework.framework_output," from %s:%d rml_ofi_close()",__FILE__,__LINE__);

    if(orte_rml_ofi.fi_info_list) {
	(void) fi_freeinfo(orte_rml_ofi.fi_info_list);
    }

    /* Close endpoint and all queues */
    for( uint8_t conduit_id=0;conduit_id<orte_rml_ofi.conduit_open_num;conduit_id++) {
         free_conduit_resources(conduit_id);
    }
    return ORTE_SUCCESS;
}


void print_provider_list_info (struct fi_info *fi )
{
    //Display all the details in the fi_info structure
    struct fi_info *cur_fi = fi;
    int fi_count = 0;

    opal_output_verbose(1,orte_rml_base_framework.framework_output," %s:%d Printing results of fi_getinfo() ",__FILE__,__LINE__);

    while(cur_fi != NULL)   {
        fi_count++;
        opal_output_verbose(10,orte_rml_base_framework.framework_output," %d.\n",fi_count);
        opal_output_verbose(10,orte_rml_base_framework.framework_output," fi_info[]->caps                  :  0x%x \n",cur_fi->caps);
        opal_output_verbose(10,orte_rml_base_framework.framework_output," fi_info[]->mode                  :  0x%x \n",cur_fi->mode);
        opal_output_verbose(10,orte_rml_base_framework.framework_output," fi_info[]->address_format        :  0x%x \n",cur_fi->addr_format);
        opal_output_verbose(10,orte_rml_base_framework.framework_output," fi_info[]->fabric_attr->provname :  %s \n",cur_fi->fabric_attr->prov_name);
        opal_output_verbose(10,orte_rml_base_framework.framework_output," fi_info[]->src_address           :  0x%x \n",cur_fi->src_addr);
        opal_output_verbose(10,orte_rml_base_framework.framework_output," fi_info[]->dest_address          :  0x%x \n",cur_fi->dest_addr);
        opal_output_verbose(10,orte_rml_base_framework.framework_output," EndPoint Attributes (ep_attr)    :");
        switch( cur_fi->ep_attr->type)
        {
            case FI_EP_UNSPEC:      opal_output_verbose(10,orte_rml_base_framework.framework_output," FI_EP_UNSPEC \n");
                                    break;
             case FI_EP_MSG:         opal_output_verbose(10,orte_rml_base_framework.framework_output," FI_EP_MSG \n");
                                     break;
             case FI_EP_DGRAM:       opal_output_verbose(10,orte_rml_base_framework.framework_output," FI_EP_DGRAM \n");
                                     break;
             case FI_EP_RDM:         opal_output_verbose(10,orte_rml_base_framework.framework_output," FI_EP_RDM \n");
                                     break;
             default:                opal_output_verbose(10,orte_rml_base_framework.framework_output," %d",cur_fi->ep_attr->type);
	}
	opal_output_verbose(10,orte_rml_base_framework.framework_output," Protocol			  : 0x%x \n", cur_fi->ep_attr->protocol);
	cur_fi = cur_fi->next;
    }
    opal_output_verbose(10,orte_rml_base_framework.framework_output,"Total # of providers supported is %d\n",fi_count);

}

/*
 * This returns all the supported transports in the system that support endpoint type RDM (reliable datagram)
 * The providers returned is a list of type opal_valut_t holding opal_list_t
 */
int orte_rml_ofi_query_transports(opal_value_t **providers)
{
    opal_list_t *ofi_prov;
    opal_value_t *providers_list, *prev_provider=NULL, *next_provider=NULL;
    struct fi_info *cur_fi;
    int ret;

    opal_output_verbose(10,orte_rml_base_framework.framework_output," \n Begin of query_transports %s:%d",__FILE__,__LINE__);
    if ( NULL == *providers)
    {
        /*opal_output_verbose(10,orte_rml_base_framework.framework_output,
                    " \n allocating providers to new opal_value_t %s:%d",__FILE__,__LINE__);
		*/
    	*providers = OBJ_NEW(opal_value_t);
    }

    providers_list = *providers;
    //opal_output_verbose(10,orte_rml_base_framework.framework_output,"*providers = %x, providers_list= %x",*providers,providers_list);
    cur_fi = orte_rml_ofi.fi_info_list;
    //Create the opal_value_t list in which each item is an opal_list_t that holds the provider details
    opal_output_verbose(10,orte_rml_base_framework.framework_output,"Starting to add the providers in a loop from orte_rml_ofi.fi_info_list %s:%d",__FILE__,__LINE__);
    int prov_num=0;
    while ( NULL != cur_fi ) {
	prov_num++;
	if( NULL != prev_provider)
	{
	   //if there is another provider in the array, then add another item to the providers_list
	   next_provider = OBJ_NEW(opal_value_t);
          // opal_output_verbose(10,orte_rml_base_framework.framework_output," \n allocating the next item on opal_value_t list next_provider = 0x%x  %s:%d",next_provider,__FILE__,__LINE__);
          // opal_output_verbose(10,orte_rml_base_framework.framework_output,"\n &next_provider->super =%x ",&next_provider->super);
	   providers_list->super.opal_list_next = &next_provider->super;   // is this same as next_provider (?)
	   providers_list->super.opal_list_prev = &prev_provider->super;
           providers_list = (opal_value_t *)providers_list->super.opal_list_next;
	}

	opal_output_verbose(10,orte_rml_base_framework.framework_output,"\n prov_num : %d",prov_num);
        //populate the opal_list_t *ofi_prov with provider details from the orte_rml_ofi.fi_info_list array populated in the rml_ofi_init() fn.
	ofi_prov = OBJ_NEW(opal_list_t);
	opal_output_verbose(10,orte_rml_base_framework.framework_output,"\n loading the attribute ORTE_PROV_NAME");
	if( ORTE_SUCCESS == (ret = orte_set_attribute( ofi_prov, ORTE_PROV_NAME, ORTE_ATTR_LOCAL,cur_fi->fabric_attr->prov_name ,OPAL_STRING))) {
	    opal_output_verbose(10,orte_rml_base_framework.framework_output,"\n loading the attribute ORTE_PROTOCOL");
	    if( ORTE_SUCCESS == (ret = orte_set_attribute( ofi_prov, ORTE_PROTOCOL, ORTE_ATTR_GLOBAL,(void *)&cur_fi->ep_attr->protocol ,OPAL_UINT32))) {
	       // insert the opal_list_t into opal_value_t list
 	       opal_value_load(providers_list,ofi_prov,OPAL_PTR);
	       opal_output_verbose(10,orte_rml_base_framework.framework_output,"\n loading the provider opal_list_t* prov=%x into opal_value_t list successful",ofi_prov);
	    } else {
               opal_output_verbose(1,orte_rml_base_framework.framework_output,"%s:%d Not able to add provider name ",__FILE__,__LINE__);
   	       return ORTE_ERROR;
	    }
	} else {
           opal_output_verbose(1,orte_rml_base_framework.framework_output,"%s:%d Not able to add provider name ",__FILE__,__LINE__);
	   return ORTE_ERROR;
	}

	prev_provider = providers_list;
	cur_fi = cur_fi->next;
    }

    opal_output_verbose(10,orte_rml_base_framework.framework_output,"\n%s:%d Completed Query Interface",__FILE__,__LINE__);
   // opal_output_verbose(1,orte_rml_base_framework.framework_output,"\n%s:%d Completed Query Interface",__FILE__,__LINE__);
    return ORTE_SUCCESS;
}


//debug routine to print the opal_value_t returned by query interface
void print_transports_query()
{
    opal_value_t *providers=NULL;
    char* prov_name = NULL;
    int ret;
    int32_t *protocol_ptr, protocol;
    int prov_num=0;

    protocol_ptr = &protocol;

    opal_output_verbose(10,orte_rml_base_framework.framework_output,"\n print_transports_query() Begin- %s:%d",__FILE__,__LINE__);
    opal_output_verbose(10,orte_rml_base_framework.framework_output,"\n calling the orte_rml_ofi_query_transports() ");
    if( ORTE_SUCCESS == orte_rml_ofi_query_transports(&providers))
    {
	//opal_output_verbose(10,orte_rml_base_framework.framework_output,"\n query_transports() completed, printing details\n");
	while (providers)
	{
                prov_num++;

        	opal_output_verbose(10,orte_rml_base_framework.framework_output,"\n prov_num : %d",prov_num);
		//get the first opal_list_t;
		opal_list_t *prov;
		ret = opal_value_unload(providers,(void **)&prov,OPAL_PTR);
		if (ret == OPAL_SUCCESS) {
    		   // opal_output_verbose(1,orte_rml_base_framework.framework_output,"\n %s:%d opal_value_unload() succeeded, opal_list* prov = %x",
                     //                                                                                                     __FILE__,__LINE__,prov);
		    if( orte_get_attribute( prov, ORTE_PROTOCOL, (void **)&protocol_ptr,OPAL_UINT32)) {
		      	opal_output_verbose(10,orte_rml_base_framework.framework_output,"\n Protocol  : %d",*protocol_ptr);
		    }
		    if( orte_get_attribute( prov, ORTE_PROV_NAME, (void **)&prov_name ,OPAL_STRING)) {
		      	opal_output_verbose(10,orte_rml_base_framework.framework_output,"\n Provider name : %s",prov_name);
		    } else {
			opal_output_verbose(10,orte_rml_base_framework.framework_output,"\n Error in getting Provider name");
		    }
		} else {
    		    opal_output_verbose(1,orte_rml_base_framework.framework_output,"\n %s:%d opal_value_unload() failed, opal_list* prov = %x",
                                                                                                                           __FILE__,__LINE__,prov);
		}
		providers = (opal_value_t *)providers->super.opal_list_next;
    	//	opal_output_verbose(1,orte_rml_base_framework.framework_output,"\n %s:%d - Moving on to next provider provders=%x",__FILE__,__LINE__,providers);
	}
    } else {
        opal_output_verbose(10,orte_rml_base_framework.framework_output,"\n query_transports() returned Error ");
    }
    opal_output_verbose(10,orte_rml_base_framework.framework_output,"\n End of print_transports_query() \n");
}

static orte_rml_base_module_t*
rml_ofi_init(int* priority)
{
    int ret, fi_version;
    struct fi_info *hints, *fabric_info;
    struct fi_cq_attr cq_attr = {0};
    struct fi_av_attr av_attr = {0};
    size_t namelen;
	char   ep_name[FI_NAME_MAX]= {0};
    uint8_t conduit_id = 0; //[A]

    opal_output_verbose(1,orte_rml_base_framework.framework_output,"%s:%d Entering rml_ofi_init()",__FILE__,__LINE__);

    if (init_done) {
        *priority = 2;
        return &orte_rml_ofi.super;
    }

    *priority = 2;

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
        return NULL;
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
     hints->domain_attr->control_progress = FI_PROGRESS_MANUAL; //[A]not clear if this needs to be AUTO or manual ?
     hints->domain_attr->av_type          = FI_AV_MAP;

    /**
     * FI_VERSION provides binary backward and forward compatibility support
     * Specify the version of OFI is coded to, the provider will select struct
     * layouts that are compatible with this version.
     */
     fi_version = FI_VERSION(1, 0);

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

        /*[A] added for debug purpose - removing it - Print the provider info
        print_transports_query();
        print_provider_list_info(orte_rml_ofi.fi_info_list);
        */

        /** [A] create the OFI objects for each transport in the system (fi_info_list) and store it in the ofi_conduits array **/
        for( fabric_info = orte_rml_ofi.fi_info_list, conduit_id = 0; NULL != fabric_info; fabric_info = fabric_info->next, conduit_id++)
        {
            orte_rml_ofi.ofi_conduits[conduit_id].conduit_id = conduit_id;
            orte_rml_ofi.ofi_conduits[conduit_id].fabric_info = fabric_info;

			// set FI_MULTI_RECV flag for all recv operations
			orte_rml_ofi.fi_info_list->rx_attr->op_flags = FI_MULTI_RECV;
            /**
            * Open fabric
            * The getinfo struct returns a fabric attribute struct that can be used to
            * instantiate the virtual or physical network. This opens a "fabric
            * provider". See man fi_fabric for details.
            */

            ret = fi_fabric(fabric_info->fabric_attr,         /* In:  Fabric attributes             */
                            &orte_rml_ofi.ofi_conduits[conduit_id].fabric,  /* Out: Fabric handle */
                            NULL);                           /* Optional context for fabric events */
            if (0 != ret) {
                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                "%s:%d: fi_fabric failed: %s\n",
                                __FILE__, __LINE__, fi_strerror(-ret));
                    free_conduit_resources(conduit_id);
					/* abort this current transport, but check if next transport can be opened */
                    continue;                               
            }


            /**
            * Create the access domain, which is the physical or virtual network or
            * hardware port/collection of ports.  Returns a domain object that can be
            * used to create endpoints.  See man fi_domain for details.
            */
            ret = fi_domain(orte_rml_ofi.ofi_conduits[conduit_id].fabric,  /* In:  Fabric object */
                            fabric_info,                 				/* In:  Provider          */
                            &orte_rml_ofi.ofi_conduits[conduit_id].domain, /* Out: Domain oject  */
                            NULL);                         /* Optional context for domain events */
            if (0 != ret) {
                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: fi_domain failed: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(conduit_id);
                continue;                               // abort this current transport, but check if next transport can be opened
            }

            /**
            * Create a transport level communication endpoint.  To use the endpoint,
            * it must be bound to completion counters or event queues and enabled,
            * and the resources consumed by it, such as address vectors, counters,
            * completion queues, etc.
            * see man fi_endpoint for more details.
            */
            ret = fi_endpoint(orte_rml_ofi.ofi_conduits[conduit_id].domain, /* In:  Domain object   */
                              fabric_info,                      /* In:  Provider        */
                              &orte_rml_ofi.ofi_conduits[conduit_id].ep,    /* Out: Endpoint object */
                            NULL);                                      /* Optional context     */
            if (0 != ret) {
                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: fi_endpoint failed: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(conduit_id);
                continue;                               // abort this current transport, but check if next transport can be opened
            }

            /**
            * Save the maximum inject size.
            */
            // orte_rml_ofi.max_inject_size = prov->tx_attr->inject_size;

            /**
            * Create the objects that will be bound to the endpoint.
            * The objects include:
            *     - completion queue for events
            *     - address vector of other endpoint addresses
            *     - dynamic memory-spanning memory region
            */
            cq_attr.format = FI_CQ_FORMAT_DATA;
            ret = fi_cq_open(orte_rml_ofi.ofi_conduits[conduit_id].domain, &cq_attr, &orte_rml_ofi.ofi_conduits[conduit_id].cq, NULL);
            if (ret) {
                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                "%s:%d: fi_cq_open failed: %s\n",
                                __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(conduit_id);
                continue;                               // abort this current transport, but check if next transport can be opened
            }

            /**
            * The remote fi_addr will be stored in the ofi_endpoint struct.
            * So, we use the AV in "map" mode.
            */
            av_attr.type = FI_AV_MAP;
            ret = fi_av_open(orte_rml_ofi.ofi_conduits[conduit_id].domain, &av_attr, &orte_rml_ofi.ofi_conduits[conduit_id].av, NULL);
            if (ret) {
                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: fi_av_open failed: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(conduit_id);
                continue;                               // abort this current transport, but check if next transport can be opened
            }

            /**
            * Bind the CQ and AV to the endpoint object.
            */
            ret = fi_ep_bind(orte_rml_ofi.ofi_conduits[conduit_id].ep,
                            (fid_t)orte_rml_ofi.ofi_conduits[conduit_id].cq,
                            FI_SEND | FI_RECV);
            if (0 != ret) {
                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: fi_bind CQ-EP failed: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(conduit_id);
                continue;                               // abort this current transport, but check if next transport can be opened
            }

            ret = fi_ep_bind(orte_rml_ofi.ofi_conduits[conduit_id].ep,
                            (fid_t)orte_rml_ofi.ofi_conduits[conduit_id].av,
                            0);
            if (0 != ret) {
                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: fi_bind AV-EP failed: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(conduit_id);
                continue;                               // abort this current transport, but check if next transport can be opened
            }

            /**
            * Enable the endpoint for communication
            * This commits the bind operations.
            */
            ret = fi_enable(orte_rml_ofi.ofi_conduits[conduit_id].ep);
            if (0 != ret) {
                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: fi_enable failed: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(conduit_id);
                continue;                               // abort this current transport, but check if next transport can be opened
            }
            opal_output_verbose(1,orte_rml_base_framework.framework_output,
                            "%s:%d ep enabled for conduit_id - %d ",__FILE__,__LINE__,conduit_id);


            /**
            * Get our address and publish it with modex.
            **/
			orte_rml_ofi.ofi_conduits[conduit_id].epnamelen = sizeof (orte_rml_ofi.ofi_conduits[conduit_id].ep_name);
	        ret = fi_getname((fid_t)orte_rml_ofi.ofi_conduits[conduit_id].ep,
                            &orte_rml_ofi.ofi_conduits[conduit_id].ep_name[0],
                            &orte_rml_ofi.ofi_conduits[conduit_id].epnamelen);
            if (ret) {
		                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: fi_getname failed: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(conduit_id);
                /* abort this current transport, but check if next transport can be opened */
                continue;
            }
			 switch ( orte_rml_ofi.ofi_conduits[conduit_id].fabric_info->addr_format)
            {
                case  FI_SOCKADDR_IN :
					opal_output_verbose(1,orte_rml_base_framework.framework_output,
                            "%s:%d In FI_SOCKADDR_IN.  ",__FILE__,__LINE__);
    				/* [TODO] The PMIX framework is not getting initialised in the 
					*  mpirun process, so this hits the null case, this needs to be fixed
					*/
					if( opal_pmix.put ) {
						/*  Address is of type sockaddr_in (IPv4) */
						opal_output_verbose(1,orte_rml_base_framework.framework_output,
                            "%s:%d sending Opal modex string.  ",__FILE__,__LINE__);
                    	OPAL_MODEX_SEND_STRING( ret, OPAL_PMIX_GLOBAL,
                                           	OPAL_RML_OFI_FI_SOCKADDR_IN,
											orte_rml_ofi.ofi_conduits[conduit_id].ep_name,
											orte_rml_ofi.ofi_conduits[conduit_id].epnamelen);
                        if (ORTE_SUCCESS != ret) {
                			opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: OPAL_MODEX_SEND failed: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                			free_conduit_resources(conduit_id);
							/*abort this current transport, but check if next transport can be opened*/
                			continue;           
            			}
					}		
					else
					{
						opal_output_verbose(1,orte_rml_base_framework.framework_output,"opal_pmix is NULL ");
					}
                    break;
                case  FI_ADDR_PSMX :
					opal_output_verbose(1,orte_rml_base_framework.framework_output,
                            "%s:%d In FI_ADDR_PSMX.  ",__FILE__,__LINE__);
                    /*   Address is of type Intel proprietery PSMX */
                    OPAL_MODEX_SEND_STRING( ret, OPAL_PMIX_GLOBAL,
                                           OPAL_RML_OFI_FI_ADDR_PSMX,orte_rml_ofi.ofi_conduits[conduit_id].ep_name,
													orte_rml_ofi.ofi_conduits[conduit_id].epnamelen);		
					if (ORTE_SUCCESS != ret) {
                		opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                "%s:%d: OPAL_MODEX_SEND failed: %s\n",
                                 __FILE__, __LINE__, fi_strerror(-ret));
                		free_conduit_resources(conduit_id);
						/*abort this current transport, but check if next transport can be opened*/
                		continue;           
            		}			
                    break;
                default:
                     opal_output_verbose(1,orte_rml_base_framework.framework_output,
                     "%s:%d ERROR: Cannot register address, Unhandled addr_format - %d, ep_name - %s  ",
                      __FILE__,__LINE__,orte_rml_ofi.ofi_conduits[conduit_id].fabric_info->addr_format,
                      orte_rml_ofi.ofi_conduits[conduit_id].ep_name);
            } 

            /**
            * Set the ANY_SRC address.
            */
            orte_rml_ofi.any_addr = FI_ADDR_UNSPEC;

			/**
 	        *  Allocate tx,rx buffers and Post a multi-RECV buffer for each endpoint
 	        **/
			//[TODO later]  For now not considering ep_attr prefix_size (add this later)
			orte_rml_ofi.ofi_conduits[conduit_id].rxbuf_size = DEFAULT_MULTI_BUF_SIZE * MULTI_BUF_SIZE_FACTOR;
			orte_rml_ofi.ofi_conduits[conduit_id].rxbuf = malloc(orte_rml_ofi.ofi_conduits[conduit_id].rxbuf_size);     
			
			//[TODO] the context is used as NULL - this code needs to be added to provide the right context
			ret = fi_mr_reg(orte_rml_ofi.ofi_conduits[conduit_id].domain, 
							orte_rml_ofi.ofi_conduits[conduit_id].rxbuf, 
							orte_rml_ofi.ofi_conduits[conduit_id].rxbuf_size,
							FI_RECV, 0, 0, 0, &orte_rml_ofi.ofi_conduits[conduit_id].mr_multi_recv, NULL);
			// [TODO] - do we need to use fi_setopt to set the FI_OPT_MIN_MULTI_RECV ?
			ret = fi_recv(orte_rml_ofi.ofi_conduits[conduit_id].ep,
						  orte_rml_ofi.ofi_conduits[conduit_id].rxbuf,
						  orte_rml_ofi.ofi_conduits[conduit_id].rxbuf_size,
						  fi_mr_desc(orte_rml_ofi.ofi_conduits[conduit_id].mr_multi_recv), 
						  0,0);  //[TODO] context param is 0, need to set it to right context var
            /**
            *  get the fd  and register the progress fn 
            **/
			ret = fi_control(&orte_rml_ofi.ofi_conduits[conduit_id].cq->fid, FI_GETWAIT, (void *) &orte_rml_ofi.ofi_conduits[conduit_id].fd);
			if (0 != ret) {
                opal_output_verbose(1, orte_rml_base_framework.framework_output,
                                    "%s:%d: fi_control failed to get fd: %s\n",
                                    __FILE__, __LINE__, fi_strerror(-ret));
                free_conduit_resources(conduit_id);
                continue;                               // abort this current transport, but check if next transport can be opened
            }
			// [TODO] use the opal_event_add to do a libevent add on the fd
			/* - create the event
			/* just ensure the send_event is not active 
                if (!peer->send_ev_active) {
                    opal_event_add(&peer->send_event, 0);
                    peer->send_ev_active = true;
                }
			something like below has to be done but need to figure where to create and store the event - maybe in the conduit[] and the arg (last
             param)   will be the entire conduit array ?
			opal_event_set(mca_oob_tcp_module.ev_base,
                       &peer->recv_event,
                       peer->sd,
                       OPAL_EV_READ|OPAL_EV_PERSIST,
                       mca_oob_tcp_recv_handler,
                       peer);
			*/

        }
    }

    /** the number of conduits in the ofi_conduits[] array **/
    orte_rml_ofi.conduit_open_num = conduit_id;

    /**
     * Free providers info since it's not needed anymore.
     */
    fi_freeinfo(hints);
    hints = NULL;


    /* only if atleast one conduit was successfully opened then return the module */
    if (0 < conduit_id ) {
        opal_output_verbose(1,orte_rml_base_framework.framework_output,
                    "%s:%d conduits openened=%d returning orte_rml_ofi.super",__FILE__,__LINE__,conduit_id);
        OBJ_CONSTRUCT(&orte_rml_ofi.exceptions, opal_list_t);
        init_done = true;
        return &orte_rml_ofi.super;
    } else {
        opal_output_verbose(1,orte_rml_base_framework.framework_output,
                    "%s:%d Failed to open any conduits",__FILE__,__LINE__,conduit_id);
        return NULL;
    }
}


int
orte_rml_ofi_enable_comm(void)
{
	int ret;

    /* enable the base receive to get updates on contact info */
    orte_rml_base_comm_start();
    opal_output_verbose(1,orte_rml_base_framework.framework_output," From orte_rml_enable_comm() %s:%d", __FILE__,__LINE__);

            /**
            * fi_getname will provide address based on the fi_info.addr_format,
            * have one MODEX key for each addr_format supported by
            * libfabric - FI_SOCKADDR_IN, FI_ADDR_PSMX for now
            * later this can be increased to include all possible addr_formats.
            **/
	for(int conduit_id = 0 ; conduit_id < orte_rml_ofi.conduit_open_num ; conduit_id++)
	{

           
	}

    return ORTE_SUCCESS;
}


void
orte_rml_ofi_fini(void)
{
    opal_list_item_t *item;
    uint8_t conduit_id;


    opal_output_verbose(1,orte_rml_base_framework.framework_output," From orte_rml_ofi_fini() %s:%d", __FILE__,__LINE__);

    while (NULL !=
           (item = opal_list_remove_first(&orte_rml_ofi.exceptions))) {
        OBJ_RELEASE(item);
    }
    OBJ_DESTRUCT(&orte_rml_ofi.exceptions);

   

    /* clear the base receive */
    //[A]
    /*3/25 -> moving this to orte_rml_stub
    orte_rml_base_comm_stop();
    */

}

#if OPAL_ENABLE_FT_CR == 1
int
orte_rml_ofi_ft_event(int state) {
    int exit_status = ORTE_SUCCESS;
    int ret;


    opal_output_verbose(1,orte_rml_base_framework.framework_output," From orte_rml_ofi_ft_event() %s:%d", __FILE__,__LINE__);
    if(OPAL_CRS_CHECKPOINT == state) {
        ORTE_ACTIVATE_JOB_STATE(NULL, ORTE_JOB_STATE_FT_CHECKPOINT);
    }
    else if(OPAL_CRS_CONTINUE == state) {
        ORTE_ACTIVATE_JOB_STATE(NULL, ORTE_JOB_STATE_FT_CONTINUE);
    }
    else if(OPAL_CRS_RESTART == state) {
        ORTE_ACTIVATE_JOB_STATE(NULL, ORTE_JOB_STATE_FT_RESTART);
    }
    else if(OPAL_CRS_TERM == state ) {
        ;
    }
    else {
        ;
    }


    if(OPAL_CRS_CHECKPOINT == state) {
        ;
    }
    else if(OPAL_CRS_CONTINUE == state) {
        ;
    }
    else if(OPAL_CRS_RESTART == state) {
        (void) mca_base_framework_close(&orte_ofi_base_framework);

        if (ORTE_SUCCESS != (ret = mca_base_framework_open(&orte_ofi_base_framework, 0))) {
            ORTE_ERROR_LOG(ret);
            exit_status = ret;
            goto cleanup;
        }

        if( ORTE_SUCCESS != (ret = orte_ofi_base_select())) {
            ORTE_ERROR_LOG(ret);
            exit_status = ret;
            goto cleanup;
        }
    }
    else if(OPAL_CRS_TERM == state ) {
        ;
    }
    else {
        ;
    }

 cleanup:
    return exit_status;
}
#else
int
orte_rml_ofi_ft_event(int state) {
    opal_output_verbose(1,orte_rml_base_framework.framework_output," From orte_rml_ofi_fini() %s:%d", __FILE__,__LINE__);
   return ORTE_SUCCESS;
}
#endif


/*This fn is dummy need to fixed */
char* orte_rml_ofi_get_uri(void){

     uint8_t conduit_id = 1;
     opal_output_verbose(1,orte_rml_base_framework.framework_output," From orte_rml_ofi_fini() %s:%d", __FILE__,__LINE__);
	return orte_rml_ofi.ofi_conduits[conduit_id].ep_name;  // returning the first ep name for now
}

void orte_rml_ofi_set_uri(const char*uri){
	OPAL_OUTPUT_VERBOSE((1, orte_rml_base_framework.framework_output,
                             " \n From orte_rml_ofi_set_uri %s:%d",__FILE__,__LINE__));
//return ORTE_ERROR;
}


int orte_rml_ofi_ping(const char* uri,
                      const struct timeval* tv){
	OPAL_OUTPUT_VERBOSE((1, orte_rml_base_framework.framework_output,
                             "\n From orte_rml_ofi_ping "));
	return ORTE_ERROR;
}

int orte_rml_ofi_close_channel (orte_rml_channel_num_t channel,
                                orte_rml_channel_callback_fn_t cbfunc,
                                void* cbdata)
{
	OPAL_OUTPUT_VERBOSE((1, orte_rml_base_framework.framework_output,
                             " from orte_rml_ofi_close_channel "));
	return ORTE_SUCCESS;
}

int orte_rml_ofi_send_channel_nb (orte_rml_channel_num_t channel_num,
                                  struct iovec* msg,
                                  int count,
                                  orte_rml_tag_t tag,
                                  orte_rml_send_channel_callback_fn_t cbfunc,
                                  void* cbdata)
{

	OPAL_OUTPUT_VERBOSE((1, orte_rml_base_framework.framework_output,
                             " from orte_rml_ofi_send_channel_nb "));
	return ORTE_SUCCESS;
}

int orte_rml_ofi_open_channel(orte_process_name_t * peer,
                              opal_list_t *qos_attributes,
                              orte_rml_channel_callback_fn_t cbfunc,
                              void *cbdata)
{
	OPAL_OUTPUT_VERBOSE((1, orte_rml_base_framework.framework_output,
                             " from orte_rml_ofi_open_channel_nb "));
	return ORTE_SUCCESS;
}

int orte_rml_ofi_send_buffer_nb(orte_process_name_t* peer,
                                opal_buffer_t* buffer,
                                orte_rml_tag_t tag,
                                orte_rml_buffer_callback_fn_t cbfunc,
                                void* cbdata)
{

	OPAL_OUTPUT_VERBOSE((1, orte_rml_base_framework.framework_output,
                             " from orte_rml_ofi_send_buffer_nb "));
	return ORTE_SUCCESS;
}

int orte_rml_ofi_send_buffer_channel_nb (orte_rml_channel_num_t channel,
                                         opal_buffer_t *buffer,
                                         orte_rml_tag_t tag,
                                         orte_rml_send_buffer_channel_callback_fn_t cbfunc,
                                         void* cbdata)
{
	OPAL_OUTPUT_VERBOSE((1, orte_rml_base_framework.framework_output,
                             " from orte_rml_ofi_send_buffer_channel_nb "));
	return ORTE_SUCCESS;
}

int orte_rml_ofi_send_nb(orte_process_name_t* peer,
                         struct iovec* iov,
                         int count,
                         orte_rml_tag_t tag,
                         orte_rml_callback_fn_t cbfunc,
                         void* cbdata)
{

	OPAL_OUTPUT_VERBOSE((1, orte_rml_base_framework.framework_output,
                             " from orte_rml_ofi_send_nb "));
	return ORTE_SUCCESS;
}


void orte_rml_ofi_recv_cancel(orte_process_name_t* peer,
                              orte_rml_tag_t tag)
{
}


void orte_rml_ofi_recv_nb(orte_process_name_t* peer,
                         orte_rml_tag_t tag,
			 bool persistent,
                         orte_rml_callback_fn_t cbfunc,
                         void* cbdata)
{

	OPAL_OUTPUT_VERBOSE((1, orte_rml_base_framework.framework_output,
                             " from orte_rml_oob_recv_nb "));
}


void orte_rml_ofi_recv_buffer_nb(orte_process_name_t* peer,
                                orte_rml_tag_t tag,
				bool persistent,
                                orte_rml_buffer_callback_fn_t cbfunc,
                                void* cbdata)
{

	OPAL_OUTPUT_VERBOSE((1, orte_rml_base_framework.framework_output,
                             " from orte_rml_oob_recv_buffer_nb "));
}

int orte_rml_ofi_add_exception(orte_rml_exception_callback_t cbfunc)
{
	return ORTE_SUCCESS;
}
int orte_rml_ofi_del_exception(orte_rml_exception_callback_t cbfunc)
{
	return ORTE_SUCCESS;
}

void orte_rml_ofi_exception_callback(orte_process_name_t *peer,
                                     orte_rml_exception_t exception)
{
}


void orte_rml_ofi_purge(orte_process_name_t *peer)
{
}


