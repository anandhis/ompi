#include "orte_config.h"

#include <stdio.h>
#include <signal.h>
#include <math.h>

#include "opal/runtime/opal_progress.h"

#include "orte/util/proc_info.h"
#include "orte/util/name_fns.h"
#include "orte/runtime/orte_globals.h"
#include "orte/mca/rml/rml.h"
#include "orte/mca/rml/base/base.h"
#include "orte/mca/rml/rml_types.h"
#include "orte/mca/errmgr/errmgr.h"

#include "orte/runtime/runtime.h"
#include "orte/runtime/orte_wait.h"

#define MY_TAG 12345
#define MAX_COUNT 3

static bool msg_recvd;
static volatile bool msg_active;

static void send_callback(int status, orte_process_name_t *peer,
                          opal_buffer_t* buffer, orte_rml_tag_t tag,
                          void* cbdata)

{
    OBJ_RELEASE(buffer);
    if (ORTE_SUCCESS != status) {
        exit(1);
    }
    msg_active = false;
}


/*
void print_provider_list_info (struct fi_info *fi )
{
    //Display all the details in the fi_info structure
    struct fi_info *cur_fi = fi;
    int fi_count = 0;

    while(cur_fi != NULL)   {
        fi_count++;
        opal_output(0," %d.\n",fi_count);
        opal_output(0," fi_info[]->caps                  :  0x%x \n",cur_fi->caps);
        opal_output(0," fi_info[]->mode                  :  0x%x \n",cur_fi->mode);
        opal_output(0," fi_info[]->address_format        :  0x%x \n",cur_fi->addr_format);
        opal_output(0," fi_info[]->fabric_attr->provname :  %s \n",cur_fi->fabric_attr->prov_name);
        opal_output(0," fi_info[]->src_address           :  0x%x \n",cur_fi->src_addr);
        opal_output(0," fi_info[]->dest_address          :  0x%x \n",cur_fi->dest_addr);
        opal_output(0," EndPoint Attributes (ep_attr)    :");
        switch( cur_fi->ep_attr->type)
        {
            case FI_EP_UNSPEC:      opal_output(0," FI_EP_UNSPEC \n");
                                    break;
             case FI_EP_MSG:         opal_output(0," FI_EP_MSG \n");
                                     break;
             case FI_EP_DGRAM:       opal_output(0," FI_EP_DGRAM \n");
                                     break;
             case FI_EP_RDM:         opal_output(0," FI_EP_RDM \n");
                                     break;
             default:                opal_output(0," %d",cur_fi->ep_attr->type);
	}
	opal_output(0," Protocol			  : 0x%x \n", cur_fi->ep_attr->protocol);
	cur_fi = cur_fi->next;
    }
    opal_output(0,"Total # of providers supported is %d\n",fi_count);

}
*/

//debug routine to print the opal_value_t returned by query interface
void print_transports_query()
{
    opal_value_t *providers=NULL;
    char* prov_name = NULL;
    int ret;
    int32_t *protocol_ptr, protocol;
    int prov_num=0;
    
    protocol_ptr = &protocol;
	
    opal_output(0,"\n print_transports_query() Begin- %s:%d",__FILE__,__LINE__);
    opal_output(0,"\n calling the orte_rml_ofi_query_transports() ");
    if( ORTE_SUCCESS == orte_rml.query_transports(&providers))
    {
	//opal_output(0,"\n query_transports() completed, printing details\n");
	while (providers)
	{
                prov_num++;

        	opal_output(0," prov_num : %d",prov_num);
		//get the first opal_list_t;
		opal_list_t *prov;
		ret = opal_value_unload(providers,(void **)&prov,OPAL_PTR);
		if (ret == OPAL_SUCCESS) {
    		   // opal_output_verbose(1,orte_rml_base_framework.framework_output,"\n %s:%d opal_value_unload() succeeded, opal_list* prov = %x",
                     //                                                                                                     __FILE__,__LINE__,prov); 	
		    if( orte_get_attribute( prov, ORTE_PROTOCOL, (void **)&protocol_ptr,OPAL_UINT32)) {
		      	opal_output(0," Protocol  : %d",*protocol_ptr);
		    }
		    if( orte_get_attribute( prov, ORTE_PROV_NAME, (void **)&prov_name ,OPAL_STRING)) {
		      	opal_output(0," Provider name : %s",prov_name);
		    } else {
			opal_output(0," Error in getting Provider name");
		    }
		} else {
    		    opal_output(0," %s:%d opal_value_unload() failed, opal_list* prov = %x",
                                                                                                                           __FILE__,__LINE__,prov); 	
		}
		providers = (opal_value_t *)providers->super.opal_list_next;
    	//	opal_output_verbose(1,orte_rml_base_framework.framework_output,"\n %s:%d - Moving on to next provider provders=%x",__FILE__,__LINE__,providers);
	}
    } else {
        opal_output(0,"\n query_transports() returned Error ");
    }
    opal_output(0,"\n End of print_transports_query() from ofi_query_test.c \n");

  //need to free all the providers here
}


int
main(int argc, char *argv[]){
    int count;
    int msgsize;
    uint8_t *msg;
    int i, j, rc;
    orte_process_name_t peer;
    double maxpower;
    opal_buffer_t *buf;
    orte_rml_recv_cb_t blob;

    
     opal_output(0, "%s pid = %d ", ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), getpid());

     /*
     * Init
     */
    orte_init(&argc, &argv, ORTE_PROC_NON_MPI);

        /*
     * Runtime Messaging Layer
     */

    if (ORTE_SUCCESS == ( mca_base_framework_open(&orte_rml_base_framework, 0))) {
	 opal_output(0, "%s RML framework opened successfully ", ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), getpid());
	if (ORTE_SUCCESS == orte_rml_base_select()) {
	    opal_output(0, "%s RML framework base_select completed successfully ", ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), getpid());
	    print_transports_query();
	}
    }
   // mca_base_framework_close(&orte_rml_base_framework);

/*
    if (argc > 1) {
        count = atoi(argv[1]);
        if (count < 0) {
            count = INT_MAX-1;
        }
    } else {
        count = MAX_COUNT;
    }

    peer.jobid = ORTE_PROC_MY_NAME->jobid;
    peer.vpid = ORTE_PROC_MY_NAME->vpid + 1;
    if (peer.vpid == orte_process_info.num_procs) {
        peer.vpid = 0;
    }

    for (j=1; j < count+1; j++) {
        /* rank0 starts ring */
/*        if (ORTE_PROC_MY_NAME->vpid == 0) {
            /* setup the initiating buffer - put random sized message in it */
/*            buf = OBJ_NEW(opal_buffer_t);

            maxpower = (double)(j%7);
            msgsize = (int)pow(10.0, maxpower);
            opal_output(0, "Ring %d message size %d bytes", j, msgsize);
            msg = (uint8_t*)malloc(msgsize);
            opal_dss.pack(buf, msg, msgsize, OPAL_BYTE);
            free(msg);
            orte_rml.send_buffer_nb(&peer, buf, MY_TAG, orte_rml_send_callback, NULL);

            /* wait for it to come around */
/*            OBJ_CONSTRUCT(&blob, orte_rml_recv_cb_t);
            blob.active = true;
            orte_rml.recv_buffer_nb(ORTE_NAME_WILDCARD, MY_TAG,
                                    ORTE_RML_NON_PERSISTENT,
                                    orte_rml_recv_callback, &blob);
            ORTE_WAIT_FOR_COMPLETION(blob.active);
            OBJ_DESTRUCT(&blob);

            opal_output(0, "%s Ring %d completed", ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), j);
        } else {
            /* wait for msg */
/*            OBJ_CONSTRUCT(&blob, orte_rml_recv_cb_t);
            blob.active = true;
            orte_rml.recv_buffer_nb(ORTE_NAME_WILDCARD, MY_TAG,
                                    ORTE_RML_NON_PERSISTENT,
                                    orte_rml_recv_callback, &blob);
            ORTE_WAIT_FOR_COMPLETION(blob.active);

            opal_output(0, "%s received message %d from %s", ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), j, ORTE_NAME_PRINT(&blob.name));

            /* send it along */
/*            buf = OBJ_NEW(opal_buffer_t);
            opal_dss.copy_payload(buf, &blob.data);
            OBJ_DESTRUCT(&blob);
            msg_active = true;
            orte_rml.send_buffer_nb(&peer, buf, MY_TAG, send_callback, NULL);
            ORTE_WAIT_FOR_COMPLETION(msg_active);
        }
    }
*/
    opal_output(0, "%s calling orte_finalize() from ofi_query_test.c ",ORTE_NAME_PRINT(ORTE_PROC_MY_NAME));
    orte_finalize();

    return 0;
}
