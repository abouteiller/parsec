/*
 * Copyright (c) 2009      The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef USE_MPI
#include <mpi.h>
#endif /* USE_MPI */

#include "dague.h"
#include "data_dist/matrix/tabular_distribution/tabular_distribution.h"

/* tiles arranged in colum major*/

static uint32_t td_get_rank_for_tile(dague_ddesc_t * desc, ...)
{
    int m, n, res;
    va_list ap;
    tabular_distribution_t * Ddesc;
    Ddesc = (tabular_distribution_t *)desc;
    va_start(ap, desc);
    m = va_arg(ap, int);
    n = va_arg(ap, int);
    va_end(ap);

    /* asking for tile (m,n) in submatrix, compute which tile it corresponds in full matrix */
    m += ((tiled_matrix_desc_t *)desc)->i;
    n += ((tiled_matrix_desc_t *)desc)->j;
    
    res = (Ddesc->super.lmt * n) + m;
    return Ddesc->tiles_table[res].rank;
}



static void * td_get_local_tile(dague_ddesc_t * desc, ...)
{
    int res, m, n;
    tabular_distribution_t * Ddesc;
    va_list ap;
    Ddesc = (tabular_distribution_t *)desc;
    va_start(ap, desc);
    m = va_arg(ap, int);
    n = va_arg(ap, int);
    va_end(ap);

     /* asking for tile (m,n) in submatrix, compute which tile it corresponds in full matrix */
    m += ((tiled_matrix_desc_t *)desc)->i;
    n += ((tiled_matrix_desc_t *)desc)->j;

#ifdef DISTRIBUTED
    assert(desc->myrank == td_get_rank_for_tile(desc, m, n));    
#endif /* DISTRIBUTED */

    res = (Ddesc->super.lmt * n) + m;
    
    return  Ddesc->tiles_table[res].tile;
}

void tabular_distribution_init(tabular_distribution_t * Ddesc, enum matrix_type mtype, unsigned int nodes, unsigned int cores, unsigned int myrank, unsigned int mb, unsigned int nb, unsigned int lm, unsigned int ln, unsigned int i, unsigned int j, unsigned int m, unsigned int n, unsigned int * table )
{
    size_t res;
    unsigned int total = 0;
    

    // Filling matrix description with user parameter
    Ddesc->super.super.nodes = nodes ;
    Ddesc->super.super.cores = cores ;
    Ddesc->super.super.myrank = myrank ;
    Ddesc->super.mtype = mtype;
    Ddesc->super.mb = mb;
    Ddesc->super.nb = nb;
    Ddesc->super.lm = lm;
    Ddesc->super.ln = ln;
    Ddesc->super.i = i;
    Ddesc->super.j = j;
    Ddesc->super.m = m;
    Ddesc->super.n = n;

    // Matrix derived parameters
    Ddesc->super.lmt = ((Ddesc->super.lm)%(Ddesc->super.mb)==0) ? ((Ddesc->super.lm)/(Ddesc->super.mb)) : ((Ddesc->super.lm)/(Ddesc->super.mb) + 1);
    Ddesc->super.lnt = ((Ddesc->super.ln)%(Ddesc->super.nb)==0) ? ((Ddesc->super.ln)/(Ddesc->super.nb)) : ((Ddesc->super.ln)/(Ddesc->super.nb) + 1);
    Ddesc->super.bsiz =  Ddesc->super.mb * Ddesc->super.nb;

    // Submatrix parameters    
    Ddesc->super.mt = ((Ddesc->super.m)%(Ddesc->super.mb)==0) ? ((Ddesc->super.m)/(Ddesc->super.mb)) : ((Ddesc->super.m)/(Ddesc->super.mb) + 1);
    Ddesc->super.nt = ((Ddesc->super.n)%(Ddesc->super.nb)==0) ? ((Ddesc->super.n)/(Ddesc->super.nb)) : ((Ddesc->super.n)/(Ddesc->super.nb) + 1);
    

    /* allocate the table*/
    Ddesc->tiles_table = malloc((Ddesc->super.lmt) * (Ddesc->super.lnt) * sizeof(tile_elem_t));

    
    /*
    for (res = 0 ; res < (Ddesc->super.lmt) * (Ddesc->super.lnt) ; res++)
        {
            Ddesc->tiles_table[res].rank = table[res];
            if(table[res] == myrank) 
                {
                    Ddesc->tiles_table[res].tile = dague_data_allocate( Ddesc->super.bsiz * (size_t) Ddesc->super.mtype);
                    if (Ddesc->tiles_table[res].tile == NULL)
                        {
                            perror("matrix memory allocation failed\n");
                            exit(-1);
                        }
                    
                }
            else
                Ddesc->tiles_table[res].tile = NULL;
        }
    */
    for (res = 0 ; res < (Ddesc->super.lmt) * (Ddesc->super.lnt) ; res++)
        {
            Ddesc->tiles_table[res].rank = table[res];
            if(table[res] == myrank) 
                {
                    total++;   
                }
        }
    Ddesc->super.nb_local_tiles = total;
    Ddesc->super.super.rank_of =  td_get_rank_for_tile;
    Ddesc->super.super.data_of =  td_get_local_tile;
    
}


unsigned int * create_2dbc(unsigned int size, unsigned int block, unsigned int nbproc, unsigned int Grow)
{
    unsigned int nbtiles;
    unsigned int * res;
    unsigned int Gcol;
    unsigned int i, j, k, cr, rr, rank;
    if (nbproc % Grow != 0)
        {
            printf("bad process grid\n");
            return NULL;
        }

    nbtiles = (size + block - 1) / block;
    res = malloc(nbtiles * nbtiles * sizeof(unsigned int));
    
    if (res == NULL)
        {
            printf("malloc failed for table creation\n");
            return NULL;
        }
    
    Gcol = nbproc / Grow;

    k = 0;
    for ( j = 0 ; j < nbtiles ; j++)
        for ( i = 0 ; i < nbtiles ; i++)
            {
                rr = i % Grow;
                cr = j % Gcol;
                rank = (rr * Gcol) + cr;
                res[k]= rank;
                k++;
            }
    return res;
    
}
