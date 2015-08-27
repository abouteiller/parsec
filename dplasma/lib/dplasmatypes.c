/*
 * Copyright (c) 2010-2015 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 */

#include <core_blas.h>
#include <dague.h>
#include <dague/constants.h>
#include "dplasma.h"
#include "dplasmatypes.h"

#if defined(HAVE_MPI)
int dplasma_get_extent( MPI_Datatype dt, MPI_Aint* extent )
{
    int rc;
#if defined(HAVE_MPI_20)
    MPI_Aint lb = 0; (void)lb;
    rc = MPI_Type_get_extent(dt, &lb, extent);
#else
    rc = MPI_Type_extent( dt, extent);
#endif  /* defined(HAVE_MPI_20) */
    return (MPI_SUCCESS == rc ? DAGUE_SUCCESS : DAGUE_ERROR);
}

int dplasma_datatype_define_contiguous( dague_datatype_t oldtype,
                                        unsigned int nb_elem,
                                        int resized,
                                        dague_datatype_t* newtype )
{
    int oldsize, rc;

    /* Check if the type is valid and supported by the MPI library */
    rc = MPI_Type_size(oldtype, &oldsize);
    if( 0 == oldsize ) {
        return DAGUE_NOT_SUPPORTED;
    }
    /**
     * Define the TILE type.
     */
    rc = dague_type_create_contiguous(nb_elem, oldtype, newtype);
    if( DAGUE_SUCCESS != rc ) {
        return rc;
    }
    if( resized >= 0 ) {
        MPI_Datatype tmp = *newtype;
        rc = dague_type_create_resized(tmp, 0, resized*oldsize, newtype);
        if( DAGUE_SUCCESS != rc ) {
            return rc;
        }
        MPI_Type_free(&tmp);
    }
    MPI_Type_commit(newtype);
#if defined(HAVE_MPI_20)
    {
        char newtype_name[MPI_MAX_OBJECT_NAME], oldtype_name[MPI_MAX_OBJECT_NAME];
        int len;

        MPI_Type_get_name(oldtype, oldtype_name, &len);
        snprintf(newtype_name, MPI_MAX_OBJECT_NAME, "CONT %s*%4u", oldtype_name, nb_elem);
        MPI_Type_set_name(*newtype, newtype_name);
    }
#endif  /* defined(HAVE_MPI_20) */
    return DAGUE_SUCCESS;
}

int dplasma_datatype_define_rectangle( dague_datatype_t oldtype,
                                       unsigned int tile_mb,
                                       unsigned int tile_nb,
                                       int resized,
                                       dague_datatype_t* newtype )
{
    int oldsize, rc;

    /* Check if the type is valid and supported by the MPI library */
    MPI_Type_size(oldtype, &oldsize);
    if( 0 == oldsize ) {
        return DAGUE_NOT_SUPPORTED;
    }
    /**
     * Define the TILE type.
     */
    rc = dague_type_create_contiguous(tile_nb * tile_mb, oldtype, newtype);
    if( DAGUE_SUCCESS != rc ) {
        return rc;
    }
    if( resized >= 0 ) {
        MPI_Datatype tmp = *newtype;
        rc = dague_type_create_resized(tmp, 0, resized*oldsize, newtype);
        if( DAGUE_SUCCESS != rc ) {
            return rc;
        }
        MPI_Type_free(&tmp);
    }
    MPI_Type_commit(newtype);
#if defined(HAVE_MPI_20)
    {
        char newtype_name[MPI_MAX_OBJECT_NAME], oldtype_name[MPI_MAX_OBJECT_NAME];
        int len;

        MPI_Type_get_name(oldtype, oldtype_name, &len);
        snprintf(newtype_name, MPI_MAX_OBJECT_NAME, "RECT %s*%4u*%4u", oldtype_name, tile_mb, tile_nb);
        MPI_Type_set_name(*newtype, newtype_name);
    }
#endif  /* defined(HAVE_MPI_20) */
    return DAGUE_SUCCESS;
}

int dplasma_datatype_define_upper( dague_datatype_t oldtype,
                                   unsigned int tile_nb, int diag,
                                   dague_datatype_t* newtype )
{
    int *blocklens, *indices, oldsize, rc;
    unsigned int i;
    MPI_Datatype tmp;

    diag = (diag == 0) ? 1 : 0;
    blocklens = (int*)malloc( tile_nb * sizeof(int) );
    indices = (int*)malloc( tile_nb * sizeof(int) );

    /* UPPER_TILE with the diagonal */
    for( i = diag; i < tile_nb; i++ ) {
        blocklens[i] = i + 1 - diag;
        indices[i] = i * tile_nb;
    }
    rc = dague_type_create_indexed(tile_nb-diag, blocklens+diag, indices+diag, oldtype, &tmp);
    if( DAGUE_SUCCESS != rc ) {
        return rc;
    }
    MPI_Type_size(oldtype, &oldsize);
    dague_type_create_resized( tmp, 0, tile_nb*tile_nb*oldsize, newtype);
#if defined(HAVE_MPI_20)
    {
        char newtype_name[MPI_MAX_OBJECT_NAME], oldtype_name[MPI_MAX_OBJECT_NAME];
        int len;

        MPI_Type_get_name(oldtype, oldtype_name, &len);
        snprintf(newtype_name, MPI_MAX_OBJECT_NAME, "UPPER %s*%4u", oldtype_name, tile_nb);
        MPI_Type_set_name(*newtype, newtype_name);
    }
#endif  /* defined(HAVE_MPI_20) */
    MPI_Type_commit(newtype);
    MPI_Type_free(&tmp);
    free(blocklens);
    free(indices);
    return DAGUE_SUCCESS;
}

int dplasma_datatype_define_lower( dague_datatype_t oldtype,
                                   unsigned int tile_nb, int diag,
                                   dague_datatype_t* newtype )
{
    int *blocklens, *indices, oldsize, rc;
    unsigned int i;
    MPI_Datatype tmp;

    diag = (diag == 0) ? 1 : 0;
    blocklens = (int*)malloc( tile_nb * sizeof(int) );
    indices = (int*)malloc( tile_nb * sizeof(int) );

    /* LOWER_TILE with or without the diagonal */
    for( i = 0; i < tile_nb-diag; i++ ) {
        blocklens[i] = tile_nb - i - diag;
        indices[i] = i * tile_nb + i + diag;
    }
    rc = dague_type_create_indexed(tile_nb-diag, blocklens, indices, oldtype, &tmp);
    if( DAGUE_SUCCESS != rc ) {
        return rc;
    }
    MPI_Type_size(oldtype, &oldsize);

    dague_type_create_resized( tmp, 0, tile_nb*tile_nb*oldsize, newtype);
#if defined(HAVE_MPI_20)
    {
        char newtype_name[MPI_MAX_OBJECT_NAME], oldtype_name[MPI_MAX_OBJECT_NAME];
        int len;

        MPI_Type_get_name(oldtype, oldtype_name, &len);
        snprintf(newtype_name, MPI_MAX_OBJECT_NAME, "LOWER %s*%4u", oldtype_name, tile_nb);
        MPI_Type_set_name(*newtype, newtype_name);
    }
#endif  /* defined(HAVE_MPI_20) */
    MPI_Type_commit(newtype);
    MPI_Type_free(&tmp);
    free(blocklens);
    free(indices);
    return DAGUE_SUCCESS;
}

int dplasma_datatype_undefine_type(dague_datatype_t* type)
{
    return (MPI_SUCCESS == MPI_Type_free(type) ? DAGUE_SUCCESS : DAGUE_ERROR);
}

#else

static inline int
dplasma_get_extent( MPI_Datatype dt, MPI_Aint* extent )
{
    return DAGUE_SUCCESS;
}

static inline int
dplasma_datatype_define_contiguous( dague_datatype_t oldtype,
                                    unsigned int nb_elem,
                                    int resized,
                                    dague_datatype_t* newtype )
{
    (void)oldtype; (void)tile_mb; (void)tile_nb; (void)resized; (void)newtype;
    return DAGUE_SUCCESS;
}

static inline int
dplasma_datatype_define_rectangle( dague_datatype_t oldtype,
                                   unsigned int tile_mb,
                                   unsigned int tile_nb,
                                   int resized,
                                   dague_datatype_t* newtype )
{
    (void)oldtype; (void)tile_mb; (void)tile_nb; (void)resized; (void)newtype;
    return DAGUE_SUCCESS;
}

static inline int
dplasma_datatype_define_lower( dague_datatype_t oldtype,
                               unsigned int tile_nb, int diag,
                               dague_datatype_t* newtype )
{
    (void)oldtype; (void)tile_nb; (void)diag; (void)newtype;
    return DAGUE_SUCCESS;
}

static inline int
dplasma_datatype_define_upper( dague_datatype_t oldtype,
                               unsigned int tile_nb, int diag,
                               dague_datatype_t* newtype )
{
    (void)oldtype; (void)tile_nb; (void)diag; (void)newtype;
    return DAGUE_SUCCESS;
}

static inline int
dplasma_datatype_undefine_type(dague_datatype_t* type)
{
    (void)type;
    return DAGUE_SUCCESS;
}

#endif /* defined(HAVE_MPI) */

int dplasma_datatype_define_tile( dague_datatype_t oldtype,
                                  unsigned int tile_nb,
                                  dague_datatype_t* newtype )
{
    return dplasma_datatype_define_rectangle(oldtype, tile_nb, tile_nb, -1, newtype);
}

int dplasma_add2arena_contiguous( dague_arena_t *arena,
                                  size_t elem_size,
                                  size_t alignment,
                                  dague_datatype_t oldtype,
                                  unsigned int nb_elem,
                                  int resized )
{
    dague_datatype_t newtype = NULL;
    MPI_Aint extent = 0;
    int rc;

    (void)elem_size;

    rc = dplasma_datatype_define_contiguous(oldtype, nb_elem, resized, &newtype);
    if( DAGUE_SUCCESS != rc ) {
        return rc;
    }
    rc = dplasma_get_extent(newtype, &extent);
    if( DAGUE_SUCCESS != rc ) {
        return rc;
    }
    rc = dague_arena_construct(arena, extent, alignment, newtype);
    if( DAGUE_SUCCESS != rc ) {
        return rc;
    }

    return DAGUE_SUCCESS;
}

int dplasma_add2arena_rectangle( dague_arena_t *arena,
                                 size_t elem_size,
                                 size_t alignment,
                                 dague_datatype_t oldtype,
                                 unsigned int tile_mb,
                                 unsigned int tile_nb,
                                 int resized )
{
    dague_datatype_t newtype = NULL;
    MPI_Aint extent = 0;
    int rc;

    (void)elem_size;

    rc = dplasma_datatype_define_rectangle(oldtype, tile_mb, tile_nb, resized, &newtype);
    if( DAGUE_SUCCESS != rc ) {
        return rc;
    }
    rc = dplasma_get_extent(newtype, &extent);
    if( DAGUE_SUCCESS != rc ) {
        return rc;
    }
    rc = dague_arena_construct(arena, extent, alignment, newtype);
    if( DAGUE_SUCCESS != rc ) {
        return rc;
    }

    return 0;
}

int dplasma_add2arena_tile( dague_arena_t *arena,
                            size_t elem_size,
                            size_t alignment,
                            dague_datatype_t oldtype,
                            unsigned int tile_mb )
{
    return dplasma_add2arena_rectangle( arena, elem_size, alignment,
                                        oldtype, tile_mb, tile_mb, -1);
}

int dplasma_add2arena_upper( dague_arena_t *arena,
                             size_t elem_size,
                             size_t alignment,
                             dague_datatype_t oldtype,
                             unsigned int tile_mb,  int diag )
{
    dague_datatype_t newtype = NULL;
    MPI_Aint extent = 0;
    int rc;
    (void)elem_size;

    rc = dplasma_datatype_define_upper( oldtype, tile_mb, diag, &newtype);
    if( DAGUE_SUCCESS != rc ) {
        return rc;
    }
    rc = dplasma_get_extent(newtype, &extent);
    if( DAGUE_SUCCESS != rc ) {
        return rc;
    }
    rc = dague_arena_construct(arena, extent, alignment, newtype);
    if( DAGUE_SUCCESS != rc ) {
        return rc;
    }

    return 0;
}

int dplasma_add2arena_lower( dague_arena_t *arena,
                             size_t elem_size,
                             size_t alignment,
                             dague_datatype_t oldtype,
                             unsigned int tile_mb, int diag )
{
    dague_datatype_t newtype = NULL;
    MPI_Aint extent = 0;
    int rc;
    (void)elem_size;

    rc = dplasma_datatype_define_lower( oldtype, tile_mb, diag, &newtype);
    if( DAGUE_SUCCESS != rc ) {
        return rc;
    }
    rc = dplasma_get_extent(newtype, &extent);
    if( DAGUE_SUCCESS != rc ) {
        return rc;
    }
    rc = dague_arena_construct(arena, extent, alignment, newtype);
    if( DAGUE_SUCCESS != rc ) {
        return rc;
    }

    return 0;
}
