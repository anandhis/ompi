/*
 * Copyright (c) 2015      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef MCA_RML_OFI_RML_OFI_H
#define MCA_RML_OFI_RML_OFI_H

#include "orte_config.h"

#include "opal/dss/dss_types.h"
#include "opal/mca/event/event.h"
#include "opal/mca/pmix/pmix.h"
#include "orte/mca/rml/base/base.h"

#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_tagged.h>

#include "rml_ofi_request.h"

//[A]
/** the maximum open conduit - assuming system will have no more than 20 transports*/
#define MAX_CONDUIT  20

/** The OPAL key values  **/
#define OPAL_RML_OFI_FI_SOCKADDR_IN                "rml.ofi.fisockaddrin"     // (char*)  ofi socket address (type IN) of the node process is running on
#define OPAL_RML_OFI_FI_ADDR_PSMX                  "rml.ofi.fiaddrpsmx"       // (char*)  ofi socket address (type PSM) of the node process is running on

// MULTI_BUF_SIZE_FACTOR defines how large the multi recv buffer will be.
// The minimum value of the factor is 2 which will set the multi recv buffer
// size to be twice the size of the send buffer. In order to use FI_MULTI_RECV
// feature efficiently, we need to have a large recv buffer so that we don't
// to repost the buffer often to get the remaining data when the buffer is full
#define MULTI_BUF_SIZE_FACTOR 4
#define DEFAULT_MULTI_BUF_SIZE (1024 * 1024)


#define CLOSE_FID(fd)			\
	do {							\
		if ((fd)) {					\
			fi_close(&(fd)->fid);	\
			fd = NULL;				\
		}							\
	} while (0);


#define RML_OFI_RETRY_UNTIL_DONE(FUNC)         \
    do {                                       \
        do {                                   \
            ret = FUNC;                        \
            if(OPAL_LIKELY(0 == ret)) {break;} \
        } while(-FI_EAGAIN == ret);            \
    } while(0);

BEGIN_C_DECLS


/** This structure will hold the ep and all ofi objects for each transport
and also the corresponding fi_info
**/
typedef struct {

    /** conduit ID **/
    uint8_t conduit_id;

    /** fi_info for this transport */
    struct fi_info *fabric_info;

    /** Fabric Domain handle */
    struct fid_fabric *fabric;

    /** Access Domain handle */
    struct fid_domain *domain;

    /** Address vector handle */
    struct fid_av *av;

    /** Completion queue handle */
    struct fid_cq *cq;

    /** Endpoint to communicate on */
    struct fid_ep *ep;

    /** Endpoint name */
    char ep_name[FI_NAME_MAX];

    /** Endpoint name length */
    size_t epnamelen;

    /** OFI memory region */
    struct fid_mr *mr_multi_recv;

   /** buffer for tx and rx */
   void *rxbuf;

   uint64_t rxbuf_size;
	
	 /* event,fd associated with the cq */ 
	 int fd;

	/*event associated with progress fn */
	opal_event_t progress_event;
  bool progress_ev_active;

} ofi_transport_conduit_t;


typedef struct {
    struct orte_rml_base_module_t super;

    /** Fabric info structure of all supported transports in system **/
    struct fi_info *fi_info_list;

   /** OFI ep and corr fi_info for all the transports (conduit) **/
    ofi_transport_conduit_t ofi_conduits[MAX_CONDUIT];

    /** "Any source" address */
    fi_addr_t any_addr;

    /** number of Conduits currently opened **/
    uint8_t conduit_open_num;

    opal_list_t              exceptions;
    opal_list_t              queued_routing_messages;
    opal_event_t            *timer_event;
    struct timeval           timeout;
	  
		/* event base */
		opal_event_base_t				*ev_base;   
		//[ TODO] ?? is progress thread and other inits done in oob_tcp reqd ?

} orte_rml_ofi_module_t;
//[A]

ORTE_MODULE_DECLSPEC extern orte_rml_component_t mca_rml_ofi_component;
extern orte_rml_ofi_module_t orte_rml_ofi;

int orte_rml_ofi_enable_comm(void);
void orte_rml_ofi_fini(void);
int orte_rml_ofi_ft_event(int state);
int orte_rml_ofi_query_transports(opal_value_t **providers);
int orte_rml_ofi_send_buffer_transport_nb(int conduit_id,
                                              orte_process_name_t* peer,
                                              struct opal_buffer_t* buffer,
                                              orte_rml_tag_t tag,
                                              orte_rml_buffer_callback_fn_t cbfunc,
                                              void* cbdata);
void free_conduit_resources( int conduit_id);
void print_provider_list_info (struct fi_info *fi );


int orte_rml_ofi_progress_no_inline(void);

__opal_attribute_always_inline__ static inline int
orte_rml_ofi_progress(ofi_transport_conduit_t* conduit)
{	
	ssize_t ret;
	int count=0;	/* number of messages read and processed */
	struct fi_cq_data_entry wc = { 0 };
	struct fi_cq_err_entry error = { 0 };
  orte_rml_ofi_request_t *ofi_req = NULL;

	/**
    * Read the work completions from the CQ.
    * From the completion's op_context, we get the associated OFI request.
    * Call the request's callback.
  */
  while (true) {
			/* Read the cq - that triggered the libevent to call this progress fn.
			*	 [backup logic]  if this fails, then logic has to be re-written to check all cq in a loop ?  
			*/
      ret = fi_cq_read(conduit->cq, (void *)&wc, 1);  
			 if (0 < ret) {
       		count++;
       	  if (NULL != wc.op_context) {
							/* get the context from the wc and call the message handler */
								ofi_req = TO_OFI_REQ(wc.op_context);
                assert(ofi_req);
                ret = ofi_req->event_callback(&wc, ofi_req);
                if (ORTE_SUCCESS != ret) {
                    opal_output(orte_rml_base_framework.framework_output,
                                "Error returned by OFI request event callback: %zd",
                                ret);
                    abort();
                }
					}
			} else if (ret == -FI_EAVAIL) {
		 				/**
             * An error occured and is being reported via the CQ.
             * Read the error and forward it to the upper layer.
             */
            ret = fi_cq_readerr(conduit->cq,
                                &error,
                                0);
            if (0 > ret) {
                opal_output(orte_rml_base_framework.framework_output,
                            "Error returned from fi_cq_readerr: %zd", ret);
                abort();
            }

            assert(error.op_context);
						/* get the context from wc and call the error handler */
            ofi_req = TO_OFI_REQ(error.op_context);
            assert(ofi_req);
            ret = ofi_req->error_callback(&error, ofi_req);
            if (ORTE_SUCCESS != ret) {
                opal_output(orte_rml_base_framework.framework_output,
                        "Error returned by request error callback: %zd",
                        ret);
                abort();
            } 
        } else {
            /**
             * The CQ is empty. Return.
             */
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
 	  /* call the progress fn to read the cq and process the message
		*	 for the conduit */
		count = orte_rml_ofi_progress(conduit);
		return count;
}
END_C_DECLS

#endif

