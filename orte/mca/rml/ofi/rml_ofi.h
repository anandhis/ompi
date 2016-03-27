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

#include "orte/mca/rml/base/base.h"

#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_tagged.h>

BEGIN_C_DECLS

typedef struct {
    struct orte_rml_base_module_t super;
 
    /** Fabric info structure of all supported transports in system **/
    struct fi_info *fi_info_list;
	
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

    /** "Any source" address */
    fi_addr_t any_addr;

    opal_list_t              exceptions;
    opal_list_t              queued_routing_messages;
    opal_event_t            *timer_event;
    struct timeval           timeout;
} orte_rml_ofi_module_t;

ORTE_MODULE_DECLSPEC extern orte_rml_component_t mca_rml_ofi_component;
extern orte_rml_ofi_module_t orte_rml_ofi;

int orte_rml_ofi_init(void);
void orte_rml_ofi_fini(void);
int orte_rml_ofi_ft_event(int state);
int orte_rml_ofi_query_transports(opal_value_t **providers);

END_C_DECLS

#endif
