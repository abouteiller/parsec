/*
 * Copyright (c) 2009-2017 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 */
#ifndef __USE_ARENA_H__
#define __USE_ARENA_H__

/** @defgroup parsec_internal_arena Arenas
 *  @ingroup parsec_internal
 *    Arenas represent temporary memory allocated by the runtime engine
 *    to move and store data.
 *  @addtogroup parsec_internal_arena
 *  @{
 */

#include "parsec/runtime.h"
#include "parsec/constants.h"
#include "parsec/data.h"
#if defined(PARSEC_HAVE_STDDEF_H)
#include <stddef.h>
#endif  /* PARSEC_HAVE_STDDEF_H */

#include "parsec/class/lifo.h"

BEGIN_C_DECLS

/**
 * Maximum amount of memory each arena is allowed to manipulate.
 */
extern size_t parsec_arena_max_allocated_memory;

/**
 * Maximum amount of memory cached on each arena.
 */
extern size_t parsec_arena_max_cached_memory;

#define PARSEC_ALIGN(x,a,t) (((x)+((t)(a)-1)) & ~(((t)(a)-1)))
#define PARSEC_ALIGN_PTR(x,a,t) ((t)PARSEC_ALIGN((uintptr_t)x, a, uintptr_t))
#define PARSEC_ALIGN_PAD_AMOUNT(x,s) ((~((uintptr_t)(x))+1) & ((uintptr_t)(s)-1))

/**
 * A parsec_arena_s is a structure that manages temporary memory
 * areas passed to the user code by the runtime engine.
 *
 * Typically, network messages and data generated by tasks
 * are stored into arenas.
 */
struct parsec_arena_s {
    parsec_object_t       super;
    parsec_lifo_t         area_lifo;     /**< An arena is also a LIFO */
    size_t                alignment;     /**< alignment to be respected, elem_size should be >> alignment,
                                          *   prefix size is the minimum alignment */
    size_t                elem_size;     /**< size of one element (unpacked in memory, aka extent) */
    volatile int32_t      used;          /**< elements currently allocated from the arena */
    int32_t               max_used;      /**< maximum size of the arena in elements */
    volatile int32_t      released;      /**< elements currently released but still cached in the freelist */
    int32_t               max_released;  /**< when more that max elements are released, they are really freed
                                          *   instead of joining the lifo */
    /** some host hardware requires special allocation functions (Cuda, pinning,
     *  Open CL, ...). Defaults are to use C malloc/free
     */
    parsec_data_allocate_t data_malloc;
    parsec_data_free_t     data_free;
};
PARSEC_DECLSPEC PARSEC_OBJ_CLASS_DECLARATION(parsec_arena_t);


struct parsec_arena_chunk_s {
    /** A chunk is also a list item.
     *  This chunk is chained when it resides inside an arena's free list
     *  It is SINGLETON when ( (not in a free list) and (in debug mode) ) */
    parsec_list_item_t item;
    uint32_t           count;    /**< Number of basic elements pointed by param in this chunck */
    parsec_arena_t    *origin;   /**< Arena in which this chunck should be released */
    void              *data;     /**< Actual data pointed by this chunck */
};

/* for SSE, 16 is mandatory, most cache are 64 bit aligned */
#define PARSEC_ARENA_ALIGNMENT_64b 8
#define PARSEC_ARENA_ALIGNMENT_INT sizeof(int)
#define PARSEC_ARENA_ALIGNMENT_PTR sizeof(void*)
#define PARSEC_ARENA_ALIGNMENT_SSE 16
#define PARSEC_ARENA_ALIGNMENT_CL1 64

/**
 * Constructor for the arena class. By default this constructor
 * does not enable any caching, thus it behaves more like a
 * convenience wrapper around malloc/free than a freelist.
 *
 * The elem_size and the opaque_ddt are in strict relationship, as they
 * are both used when exchanging data across node. The opaque_ddt is used
 * to describe the memory layout of the communication, while the ele_size
 * is used to allocate the memory needed to locally store the data. This
 * allows however for flexibility, but then the application should carefully
 * use the arena when communicating (in general using a count and
 * displacement).
 *
 * @note:  There is no explicit call to release an arena. Arenas being PaRSEC objects
 * they will be automatically released when their refcount reaches 0. They
 * should therefore be manipulated using PARSEC_OBJ_RELEASE.
 */
int parsec_arena_construct(parsec_arena_t* arena,
                           size_t elem_size,
                           size_t alignment);
/**
 * Extended constructor for the arena class. It enabled the
 * caching support up to max_released number of elements,
 * and prevents the freelist to handle more than max_used
 * active elements at the same time.
 *
 * @note:  There is no explicit call to release an arena. Arenas being PaRSEC objects
 * they will be automatically released when their refcount reaches 0. They
 * should therefore be manipulated using PARSEC_OBJ_RELEASE.
 */
int parsec_arena_construct_ex(parsec_arena_t* arena,
                              size_t elem_size,
                              size_t alignment,
                              size_t max_used,
                              size_t max_released);
/**
 * @brief Create a new data copy on device @p device using arena @p arena with
 *   @p count elements of type @p dtt. This also creates a new data_t that
 *   points to the returned data copy. When this data copy is released (using
 *   PARSEC_OBJ_RELEASE), the corresponding data_t is also released.
 *
 * @param arena the arena to use for allocation
 * @param count the number of elements to allocate
 * @param device the device of the data_copy. Note: the current implementation
 *   of arenas only work for CPU memory, and that device must be a CPU device.
 * @param dtt the datatype associated with the data copy created.
 * @return parsec_data_copy_t* the new data copy, or NULL if there is not
 *   enough resource to allocate a new data copy of this type.
 */

parsec_data_copy_t *parsec_arena_get_copy(parsec_arena_t *arena,
                                          size_t count, int device,
                                          parsec_datatype_t dtt);

/**
 * @brief Allocates memory for a given data copy. This is a function used by
 *  DSLs to set the memory associated with a data copy they have created.
 *  It is also used by parsec_arena_get_copy.
 *
 * @param copy the (empty) data copy to allocate memory for. NB: the @p original
 *  field of this data copy must be set. The operation overwrites the device
 *  dtt and count of this data copy, as well as the device_private pointer.
 * @param arena the arena used for the allocation
 * @param count the number of elements to allocate
 * @param device the device of the data copy. Note: the current implementation
 *   of arenas only work for CPU mmeory, and that device must be a CPU device.
 * @param dtt the datatype associated with each element of the memory allocated
 * @return int PARSEC_SUCCESS in case of success, or an error code if there
 *   was not enough resource to satisfy the allocation request.
 */
int  parsec_arena_allocate_device_private(parsec_data_copy_t *copy,
                                          parsec_arena_t *arena,
                                          size_t count, int device,
                                          parsec_datatype_t dtt);

void parsec_arena_release(parsec_data_copy_t* ptr);

END_C_DECLS

/** @} */

#endif /* __USE_ARENA_H__ */

