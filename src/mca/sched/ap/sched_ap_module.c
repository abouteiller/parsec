/**
 * Copyright (c) 2013-2014 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */

#include "dague_config.h"
#include "dague_internal.h"
#include "debug.h"
#include "dague/mca/sched/sched.h"
#include "dague/mca/sched/ap/sched_ap.h"
#include "dequeue.h"
#include "dague/mca/pins/pins.h"
static int SYSTEM_NEIGHBOR = 0;

/**
 * Module functions
 */
static int sched_ap_install(dague_context_t* master);
static int sched_ap_schedule(dague_execution_unit_t* eu_context, dague_execution_context_t* new_context);
static dague_execution_context_t *sched_ap_select( dague_execution_unit_t *eu_context );
static int flow_ap_init(dague_execution_unit_t* eu_context, struct dague_barrier_t* barrier);
static void sched_ap_remove(dague_context_t* master);

const dague_sched_module_t dague_sched_ap_module = {
    &dague_sched_ap_component,
    {
        sched_ap_install,
        flow_ap_init,
        sched_ap_schedule,
        sched_ap_select,
        NULL,
        sched_ap_remove
    }
};

static int sched_ap_install( dague_context_t *master )
{
	SYSTEM_NEIGHBOR = master->nb_vp * master->virtual_processes[0]->nb_cores;
    return 0;
}

static int flow_ap_init(dague_execution_unit_t* eu_context, struct dague_barrier_t* barrier)
{
    dague_context_t *master = eu_context->virtual_process->dague_context;
    dague_execution_unit_t *eu;
    dague_vp_t *vp;
    int p, t;

    for(p = 0; p < master->nb_vp; p++) {
        vp = master->virtual_processes[p];
        for(t = 0; t < vp->nb_cores; t++) {
            eu = vp->execution_units[t];
            if( eu->th_id == 0 ) {
                eu->scheduler_object = (dague_list_t*)malloc(sizeof(dague_list_t));
                OBJ_CONSTRUCT(eu->scheduler_object, dague_list_t);
            } else {
                eu->scheduler_object = eu->virtual_process->execution_units[0]->scheduler_object;
            }
        }
    }
    (void)barrier;
    return 0;
}

static dague_execution_context_t *sched_ap_select( dague_execution_unit_t *eu_context )
{
	dague_execution_context_t * context = 
		(dague_execution_context_t*)dague_list_pop_front((dague_list_t*)eu_context->scheduler_object);
	if (NULL != context)
		context->victim_core = SYSTEM_NEIGHBOR;
	return context;
}

static int sched_ap_schedule( dague_execution_unit_t* eu_context,
                              dague_execution_context_t* new_context )
{
#if DAGUE_DEBUG_VERBOSE >= 3
    dague_list_item_t *it = (dague_list_item_t*)new_context;
    char tmp[MAX_TASK_STRLEN];
    do {
        DEBUG3(("AP:\t Pushing task %s\n",
                dague_snprintf_execution_context(tmp, MAX_TASK_STRLEN, (dague_execution_context_t*)it)));
        it = (dague_list_item_t*)((dague_list_item_t*)it)->list_next;
    } while( it != (dague_list_item_t*)new_context );
#endif
    dague_list_chain_sorted((dague_list_t*)eu_context->scheduler_object,
                            (dague_list_item_t*)new_context,
                            dague_execution_context_priority_comparator);
    return 0;
}

static void sched_ap_remove( dague_context_t *master )
{
    int p, t;
    dague_vp_t *vp;
    dague_execution_unit_t *eu;

    for(p = 0; p < master->nb_vp; p++) {
        vp = master->virtual_processes[p];
        for(t = 0; t < vp->nb_cores; t++) {
            eu = vp->execution_units[t];
            if( eu->th_id == 0 ) {
                OBJ_DESTRUCT( eu->scheduler_object );
                free(eu->scheduler_object);
            }
            eu->scheduler_object = NULL;
        }
    }
}