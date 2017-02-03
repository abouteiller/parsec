/*
 * Copyright (c) 2009-2015 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 */

#ifndef _parsec_prof_grapher_h
#define _parsec_prof_grapher_h

/** 
 *  @addtogroup parsec_internal_profiling
 *  @{
 */

#include "parsec_config.h"

#include "parsec_internal.h"
#include "parsec/execution_unit.h"

BEGIN_C_DECLS

void  parsec_prof_grapher_init(const char *base_filename, int nbthreads);
void  parsec_prof_grapher_task(const parsec_execution_context_t *context, int thread_id, int vp_id, int task_hash);
void  parsec_prof_grapher_dep(const parsec_execution_context_t* from, const parsec_execution_context_t* to,
                             int  dependency_activates_task,
                             const parsec_flow_t* origin_flow, const parsec_flow_t* dest_flow);
char *parsec_prof_grapher_taskid(const parsec_execution_context_t *context, char *tmp, int length);
void  parsec_prof_grapher_fini(void);

char *unique_color(int index, int colorspace);

END_C_DECLS

/** @} */

#endif /* _parsec_prof_grapher_h */
