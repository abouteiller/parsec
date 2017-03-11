/*
 * Copyright (c) 2009-2010 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 */

#ifndef BINDTHREAD_H
#define BINDTHREAD_H

/** @addtogroup parsec_internal_binding
 *  @{
 */

BEGIN_C_DECLS

int parsec_bindthread(int cpu, int ht);

#if defined(PARSEC_HAVE_HWLOC)
#include <hwloc.h>
int parsec_bindthread_mask(hwloc_cpuset_t cpuset);
#endif

/** @} */

END_C_DECLS

#endif /* BINDTHREAD_H */