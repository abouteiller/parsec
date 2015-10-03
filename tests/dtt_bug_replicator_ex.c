/*
 * Copyright (c) 2013-2015 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 */

#include "dague.h"
#include "dague/data_distribution.h"
#include "data_dist/matrix/two_dim_rectangle_cyclic.h"
#include "dtt_bug_replicator.h"
#include "dague/arena.h"
#include <math.h>
#define N     100
#define NB    10

#define PASTE_CODE_ALLOCATE_MATRIX(DDESC, COND, TYPE, INIT_PARAMS)      \
    TYPE##_t DDESC;                                                     \
    if(COND) {                                                          \
        TYPE##_init INIT_PARAMS;                                        \
        DDESC.mat = dague_data_allocate((size_t)DDESC.super.nb_local_tiles * \
                                        (size_t)DDESC.super.bsiz *      \
                                        (size_t)dague_datadist_getsizeoftype(DDESC.super.mtype)); \
        dague_ddesc_set_key((dague_ddesc_t*)&DDESC, #DDESC);            \
    }


int main( int argc, char** argv )
{
    dague_context_t* dague;
    dague_handle_t* handle;
#if defined(HAVE_MPI)
    MPI_Datatype tile_dtt;
#endif
    dague_dtt_bug_replicator_handle_t *dtt_handle;;
    int nodes, rank;
    (void)argc; (void)argv;

#if defined(HAVE_MPI)
    MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &nodes);
    MPI_Comm_size(MPI_COMM_WORLD, &nodes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif

    dague = dague_init(1, &argc, &argv);
    assert( NULL != dague );

    PASTE_CODE_ALLOCATE_MATRIX(ddescA, 1,
        two_dim_block_cyclic, (&ddescA, matrix_RealDouble, matrix_Tile,
                                   nodes, rank, NB, NB, N, N, 0, 0,
                                   N, N, 1, 1, 1));

    handle = (dague_handle_t*) (dtt_handle = dague_dtt_bug_replicator_new(&ddescA.super.super));
    assert( NULL != handle );

#if defined(HAVE_MPI)
    dague_type_create_contiguous(NB*NB, dague_datatype_double_t, &tile_dtt);

    MPI_Type_set_name(tile_dtt, "TILE_DTT");
    MPI_Type_commit(&tile_dtt);
    dague_arena_construct(dtt_handle->arenas[DAGUE_dtt_bug_replicator_DEFAULT_ARENA],
                          NB*NB*sizeof(double),
                          DAGUE_ARENA_ALIGNMENT_SSE, tile_dtt);
#endif

    dague_enqueue( dague, handle );

    dague_context_wait(dague);

    dague_fini( &dague);
#if defined(HAVE_MPI)
    MPI_Finalize();
#endif
    return 0;
}
