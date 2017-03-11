/*
 * Copyright (c) 2010-2016 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 */

#include "parsec_config.h"
#include "parsec_prof_grapher.h"
#if defined(PARSEC_PROF_TRACE)
#include "parsec/parsec_binary_profile.h"
#endif
#include "parsec/utils/colors.h"
#include <errno.h>

#if defined(PARSEC_PROF_GRAPHER)

FILE *grapher_file = NULL;
static int nbfuncs = -1;
static char **colors = NULL;

void parsec_prof_grapher_init(const char *base_filename, int nbthreads)
{
    char *filename;
    int t, size = 1, rank = 0;

#if defined(DISTRIBUTED) && defined(PARSEC_HAVE_MPI)
    char *format;
    int l10 = 0, cs;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    cs = size;
    while(cs > 0) {
      l10++;
      cs = cs/10;
    }
    asprintf(&format, "%%s-%%0%dd.dot", l10);
    asprintf(&filename, format, base_filename, rank);
    free(format);
#else
    asprintf(&filename, "%s.dot", base_filename);
#endif

    grapher_file = fopen(filename, "w");
    if( NULL == grapher_file ) {
        parsec_warning("Grapher:\tunable to create %s (%s) -- DOT graphing disabled", filename, strerror(errno));
        free(filename);
        return;
    } else {
        free(filename);
    }
    fprintf(grapher_file, "digraph G {\n");
    fflush(grapher_file);

    srandom(size*(rank+1));  /* for consistent color generation */
    (void)nbthreads;
    nbfuncs = 128;
    colors = (char**)malloc(nbfuncs * sizeof(char*));
    for(t = 0; t < nbfuncs; t++)
        colors[t] = unique_color(rank * nbfuncs + t, size * nbfuncs);
}

char *parsec_prof_grapher_taskid(const parsec_execution_context_t *exec_context, char *tmp, int length)
{
    const parsec_function_t* function = exec_context->function;
    unsigned int i, index = 0;

    assert( NULL!= exec_context->parsec_handle );
    index += snprintf( tmp + index, length - index, "%s_%u", function->name, exec_context->parsec_handle->handle_id );
    for( i = 0; i < function->nb_parameters; i++ ) {
        index += snprintf( tmp + index, length - index, "_%d",
                           exec_context->locals[function->params[i]->context_index].value );
    }

    return tmp;
}

void parsec_prof_grapher_task(const parsec_execution_context_t *context,
                             int thread_id, int vp_id, int task_hash)
{
    if( NULL != grapher_file ) {
        char tmp[MAX_TASK_STRLEN], nmp[MAX_TASK_STRLEN];
        parsec_snprintf_execution_context(tmp, MAX_TASK_STRLEN, context);
        parsec_prof_grapher_taskid(context, nmp, MAX_TASK_STRLEN);
#if defined(PARSEC_SIM)
#  if defined(PARSEC_PROF_TRACE)
        fprintf(grapher_file,
                "%s [shape=\"polygon\",style=filled,fillcolor=\"%s\","
                "fontcolor=\"black\",label=\"<%d/%d> %s [%d]\","
                "tooltip=\"hid=%u:did=%d:tname=%s:tid=%d\"];\n",
                nmp, colors[context->function->function_id % nbfuncs],
                thread_id, vp_id, tmp, context->sim_exec_date,
                context->parsec_handle->handle_id,
                context->parsec_handle->profiling_array != NULL 
                    ? BASE_KEY(context->parsec_handle->profiling_array[2*context->function->function_id])
                    : -1,
                context->function->name,
                task_hash);
#  else
        fprintf(grapher_file,
                "%s [shape=\"polygon\",style=filled,fillcolor=\"%s\","
                "fontcolor=\"black\",label=\"<%d/%d> %s [%d]\","
                "tooltip=\"hid=%u:tname=%s:tid=%d\"];\n",
                nmp, colors[context->function->function_id % nbfuncs],
                thread_id, vp_id, tmp, context->sim_exec_date,
                context->parsec_handle->handle_id,
                context->function->name,
                task_hash);
#  endif
#else
#  if defined(PARSEC_PROF_TRACE)
        fprintf(grapher_file,
                "%s [shape=\"polygon\",style=filled,fillcolor=\"%s\","
                "fontcolor=\"black\",label=\"<%d/%d> %s\","
                "tooltip=\"hid=%u:did=%d:tname=%s:tid=%d\"];\n",
                nmp, colors[context->function->function_id % nbfuncs],
                thread_id, vp_id, tmp,
                context->parsec_handle->handle_id,
                context->parsec_handle->profiling_array != NULL 
                    ? BASE_KEY(context->parsec_handle->profiling_array[2*context->function->function_id])
                    : -1,
                context->function->name,
                task_hash);
#  else
        fprintf(grapher_file,
                "%s [shape=\"polygon\",style=filled,fillcolor=\"%s\","
                "fontcolor=\"black\",label=\"<%d/%d> %s\","
                "tooltip=\"hid=%u:tname=%s:tid=%d\"];\n",
                nmp, colors[context->function->function_id % nbfuncs],
                thread_id, vp_id, tmp,
                context->parsec_handle->handle_id,
                context->function->name,
                task_hash);
#  endif
#endif
        fflush(grapher_file);
    }
}

void parsec_prof_grapher_dep(const parsec_execution_context_t* from, const parsec_execution_context_t* to,
                            int dependency_activates_task,
                            const parsec_flow_t* origin_flow, const parsec_flow_t* dest_flow)
{
    if( NULL != grapher_file ) {
        char tmp[128];
        int index = 0;

        parsec_prof_grapher_taskid( from, tmp, 128 );
        index = strlen(tmp);
        index += snprintf( tmp + index, 128 - index, " -> " );
        parsec_prof_grapher_taskid( to, tmp + index, 128 - index - 4 );
        fprintf(grapher_file,
                "%s [label=\"%s=>%s\",color=\"#%s\",style=\"%s\"]\n",
                tmp, origin_flow->name, dest_flow->name,
                dependency_activates_task ? "00FF00" : "FF0000",
                ((dest_flow->flow_flags == FLOW_ACCESS_NONE) ? "dotted":
                 (dest_flow->flow_flags == FLOW_ACCESS_RW) ? "solid" : "dashed"));
        fflush(grapher_file);
    }
}

void parsec_prof_grapher_fini(void)
{
    int t;

    if( NULL == grapher_file ) return;

    fprintf(grapher_file, "}\n");
    fclose(grapher_file);
    for(t = 0; t < nbfuncs; t++)
        free(colors[t]);
    free(colors);
    colors = NULL;
    grapher_file = NULL;
}

#endif /* PARSEC_PROF_GRAPHER */