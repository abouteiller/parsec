/*
 * Copyright (c) 2017      The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 * These symbols are in a file by themselves to provide nice linker
 * semantics.  Since linkers generally pull in symbols by object
 * files, keeping these symbols as the only symbols in this file
 * prevents utility programs such as "ompi_info" from having to import
 * entire components just to query their version and parameters.
 */

#include "parsec/parsec_config.h"
#include "parsec/runtime.h"

#include "parsec/mca/sched/sched.h"
#include "parsec/mca/sched/ll/sched_ll.h"
#include "parsec/papi_sde.h"

/*
 * Local function
 */
static int sched_ll_component_query(mca_base_module_t **module, int *priority);
static int sched_ll_component_register(void);

/*
 * Instantiate the public struct with all of our public information
 * and pointers to our public functions in it
 */
const parsec_sched_base_component_t parsec_sched_ll_component = {

    /* First, the mca_component_t struct containing meta information
       about the component itsell */

    {
        PARSEC_SCHED_BASE_VERSION_2_0_0,

        /* Component name and version */
        "ll",
        "", /* options */
        PARSEC_VERSION_MAJOR,
        PARSEC_VERSION_MINOR,

        /* Component open and close functions */
        NULL, /*< No open: sched_ll is always available, no need to check at runtime */
        NULL, /*< No close: open did not allocate any resource, no need to release them */
        sched_ll_component_query,
        /*< specific query to return the module and add it to the list of available modules */
        sched_ll_component_register, /*< Register at least the SDE events */
        "", /*< no reserve */
    },
    {
        /* The component has no metada */
        MCA_BASE_METADATA_PARAM_NONE,
        "", /*< no reserve */
    }
};
mca_base_component_t *sched_ll_static_component(void)
{
    return (mca_base_component_t *)&parsec_sched_ll_component;
}

static int sched_ll_component_query(mca_base_module_t **module, int *priority)
{
    /* module type shoull be: const mca_base_module_t ** */
    void *ptr = (void*)&parsec_sched_ll_module;
    *priority = 2;
    *module = (mca_base_module_t *)ptr;
    return MCA_SUCCESS;
}

static int sched_ll_component_register(void)
{
    PARSEC_PAPI_SDE_DESCRIBE_COUNTER("SCHEDULER::PENDING_TASKS::SCHED=LL",
                              "the number of pending tasks for the LL scheduler");
    PARSEC_PAPI_SDE_DESCRIBE_COUNTER("SCHEDULER::PENDING_TASKS::QUEUE=<VPID>::SCHED=LL",
                              "the number of pending tasks that end up in the virtual process <VPID> for the LFQ scheduler");
    return MCA_SUCCESS;
}
