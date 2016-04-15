/*
 * Copyright (c) 2015      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "orte_config.h"

#include "opal/dss/dss_types.h"
#include "opal/util/output.h"
#include "opal/mca/event/event.h"

#include "orte/mca/rml/base/base.h"
#include "orte/mca/rml/rml_types.h"

#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_tagged.h>

#include "rml_ofi.h"



OBJ_CLASS_INSTANCE(orte_rml_ofi_request_t,
                   orte_rml_send_t,
                   NULL, NULL);

static void send_msg(int fd, short args, void *cbdata)
{
    orte_rml_send_request_t *req = (orte_rml_send_request_t*)cbdata;
	char dest_ep_name[FI_NAME_MAX]={0};
	size_t dest_ep_namelen = 0;
	int ret = OPAL_ERROR;
	struct orte_rml_ofi_request_t* ofi_send_req = malloc(sizeof(struct orte_rml_ofi_request_t));

   
    OPAL_OUTPUT_VERBOSE((1, orte_rml_base_framework.framework_output,
                         "%s send_msg to peer %s at tag %d",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         ORTE_NAME_PRINT(req->send.peer), req->send.tag));

	// populate the ofi_request_t with the callback fns to be called by the progress fn - and release it in the send-completion in progresshandler
	ofi_send_req->send = req->send;
	// get the peer address by doing modex_receive
	OPAL_MODEX_RECV_STRING(ret, OPAL_RML_OFI_FI_SOCKADDR_IN, &req->send.dst,&dest_ep_name[0], &dest_ep_namelen);
	if ( OPAL_SUCCESS == ret) {
		OPAL_OUTPUT_VERBOSE((1, orte_rml_base_framework.framework_output,
                         "%s OPAL_MODEX_RECV succeded, %s peer ep name obtained. length=%d",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         ORTE_NAME_PRINT(req->send.peer), dest_ep_namelen));
		
	} else {
		// [TODO] print error and call the send-callback fn with error
		OPAL_OUTPUT_VERBOSE((1, orte_rml_base_framework.framework_output,
                         "%s OPAL_MODEX_RECV failed to obtain  %s peer ep name ",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         ORTE_NAME_PRINT(req->send.peer)));
	}
	// copy the tag and buffer to the ofi_send_req->buffer do an actual fi_send() 
	// wait for completion to call the send callback fn (or) let progressfn handle this

    OBJ_RELEASE(req);
}


int orte_rml_ofi_send_buffer_transport_nb(int conduit_id,
                                              orte_process_name_t* peer,
                                              struct opal_buffer_t* buffer,
                                              orte_rml_tag_t tag,
                                              orte_rml_buffer_transport_callback_fn_t cbfunc,
                                              void* cbdata)
{
	orte_rml_send_request_t *req;

    OPAL_OUTPUT_VERBOSE((1, orte_rml_base_framework.framework_output,
                         "%s rml_ofi_send_buffer_transport to peer %s at tag %d",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         ORTE_NAME_PRINT(peer), tag));

	
	if( (0 > conduit_id) || ( conduit_id > orte_rml_ofi.conduit_open_num ) ) {
		/* Invalid conduit ID provided */
	    ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;	
	}
    if (ORTE_RML_TAG_INVALID == tag) {
        /* cannot send to an invalid tag */
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }
    if (NULL == peer ||
        OPAL_EQUAL == orte_util_compare_name_fields(ORTE_NS_CMP_ALL, ORTE_NAME_INVALID, peer)) {
        /* cannot send to an invalid peer */
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }
    /* get ourselves into an event to protect against
     * race conditions and threads
     */
	req = OBJ_NEW(orte_rml_send_request_t);   
	req->conduit_id = conduit_id;
    req->send.dst = *peer;
    req->send.buffer = buffer;
    req->send.tag = tag;
    req->send.cbfunc.buffer = cbfunc;
    req->send.cbdata = cbdata;

    /* setup the event for the send callback */
    opal_event_set(orte_event_base, &req->ev, -1, OPAL_EV_WRITE, send_msg, req);
    opal_event_set_priority(&req->ev, ORTE_MSG_PRI);
    opal_event_active(&req->ev, OPAL_EV_WRITE, 1);

	return ORTE_SUCCESS;
}

