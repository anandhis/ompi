/*
 * Copyright (c) 2015      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef ORTE_RML_OFI_REQUEST_H
#define ORTE_RML_OFI_REQUEST_H

#include "rml_ofi.h"

#define TO_OFI_REQ(_ptr_ctx) \
    container_of((_ptr_ctx), struct orte_rml_ofi_request_t, ctx)

typedef enum {
    ORTE_RML_OFI_SEND,
    ORTE_RML_OFI_RECV,
    ORTE_RML_OFI_ACK,
    ORTE_RML_OFI_PROBE
} orte_rml_ofi_request_type_t;

struct orte_rml_ofi_request_t {
    orte_rml_send_request_t super;

    /** OFI Request type */
    orte_rml_ofi_request_type_t type;

    /** OFI context */
    struct fi_context ctx;

    /** Completion count used by blocking and/or synchronous operations */
    volatile int completion_count;

    /** Event callback */
    int (*event_callback)(struct fi_cq_data_entry *wc,
                          struct orte_rml_ofi_request_t*);

    /** Error callback */
    int (*error_callback)(struct fi_cq_err_entry *error,
                          struct orte_rml_ofi_request_t*);

    
    /** Reference to the RML used to lookup */
    /*  source of an ANY_SOURCE Recv        */
    struct orte_rml_base_module_t* rml;

    /** Pack buffer */
    void *buffer;

    /** Pack buffer size */
    size_t length;

  
    /** Flag to prevent MPI_Cancel from cancelling a started Recv request */
    volatile bool req_started;

    /** Request's tag used in case of an error. */
    uint64_t match_bits;

    /** Remote OFI address used when a Recv needs to be ACKed */
    fi_addr_t remote_addr;

    /** Parent request which needs to be ACKed (e.g. Synchronous Send) */
    struct orte_rml_ofi_request_t *parent;

    /** Pointer to Mrecv request to complete */ /*  [A] not sure what this is, but leaving it in here for now */
    struct mca_rml_request_t *mrecv_req;
};
typedef struct orte_rml_ofi_request_t orte_rml_ofi_request_t;

#endif
