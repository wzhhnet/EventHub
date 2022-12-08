
#ifndef UTILS_ALLOCATOR_H
#define UTILS_ALLOCATOR_H


#include <pthread.h>
#include "list.h"
#include "errors.h"
#include "bitops.h"

#define ALLOCATOR_DECLARE(PRODUCT, TYPE)    \
    struct PRODUCT##_element {              \
        int bit;                            \
        TYPE evt;                           \
    };                                      \
    struct PRODUCT##_allocator {            \
        pthread_mutex_t mutex;              \
        unsigned char size;                 \
        unsigned int *bits;                 \
        struct PRODUCT##_element *array;    \
    }

#define ALLOCATOR_DEFINE(PRODUCT, VAR)         \
    struct PRODUCT##_allocator  VAR

#define ALLOCATOR_CREATE(PRODUCT, allocator, size)  \
    PRODUCT##_allocator_create(allocator, size)
#define ALLOCATOR_DESTORY(PRODUCT, allocator)       \
    PRODUCT##_allocator_destory(allocator)
#define ALLOCATOR_ALLOC(PRODUCT, allocator)         \
    PRODUCT##_allocator_alloc(allocator)
#define ALLOCATOR_FREE(PRODUCT, allocator, element) \
    PRODUCT##_allocator_free(allocator, element)

#define ALLOCATOR_IMPLEMENT(PRODUCT, TYPE)                                      \
    static int PRODUCT##_allocator_create(                                      \
        struct PRODUCT##_allocator *inst, unsigned char size)                   \
    {                                                                           \
        char *mem;                                                              \
        unsigned int b_size, e_size;                                            \
        RETURN_IF_TRUE(!size, UTILS_ERR_POOL_SIZE);                             \
        memset(inst, 0, sizeof(struct PRODUCT##_allocator));                    \
        b_size = sizeof(unsigned int) * BITS_TO_WORDS(size);                    \
        e_size = sizeof(struct PRODUCT##_element) * size;                       \
        mem = (char*)malloc(b_size + e_size);                                   \
        RETURN_IF_NULL(mem, UTILS_ERR_POOL_MEM);                                \
        pthread_mutex_init(&inst->mutex, NULL);                                 \
        inst->bits = (unsigned int*)mem;                                        \
        inst->size = size;                                                      \
        inst->array = (struct PRODUCT##_element*)(mem + b_size);                \
        bitmask_init(inst->bits, (int)inst->size);                              \
        return UTILS_SUCC;                                                      \
    };                                                                          \
    static void PRODUCT##_allocator_destory(struct PRODUCT##_allocator *inst)   \
    {                                                                           \
        if (inst->bits) {                                                       \
            free(inst->bits);                                                   \
            inst->bits = NULL;                                                  \
            inst->array = NULL;                                                 \
        }                                                                       \
        inst->size = 0;                                                         \
        pthread_mutex_destroy(&inst->mutex);                                    \
    };                                                                          \
    static TYPE* PRODUCT##_allocator_alloc(struct PRODUCT##_allocator *inst)    \
    {                                                                           \
        int max, bit;                                                           \
        TYPE* s = NULL;                                                         \
        pthread_mutex_lock(&inst->mutex);                                       \
        max = (int)inst->size;                                                  \
        bit = bitmask_ffz(inst->bits, max);                                     \
        if (bit >= 0 && bit < max) {                                            \
            inst->array[bit].bit = bit;                                         \
            bitmask_set(inst->bits, bit);                                       \
            s = &inst->array[bit].evt;                                          \
        }                                                                       \
        pthread_mutex_unlock(&inst->mutex);                                     \
        return s;                                                               \
    };                                                                          \
    static int PRODUCT##_allocator_free(                                        \
            struct PRODUCT##_allocator *inst, TYPE *evt)                        \
    {                                                                           \
        struct PRODUCT##_element *e;                                            \
        e = container_of(evt, struct PRODUCT##_element, evt);                   \
        if (e->bit >= 0 && e->bit < (int)inst->size) {                          \
            pthread_mutex_lock(&inst->mutex);                                   \
            bitmask_clear(inst->bits, e->bit);                                  \
            pthread_mutex_unlock(&inst->mutex);                                 \
            return UTILS_SUCC;                                                  \
        }                                                                       \
        return UTILS_ERR_POOL_FREE;                                             \
    }


#endif /*!< UTILS_ALLOCATOR_H */

