/*
 * A very simple utilities that exchange event asynchronously via another thread.
 *
 * Author wanch
 * Date 2022/11/23
 * Email wzhhnet@gmail.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "list.h"
#include "bitops.h"
#include "event_hub.h"

#define RETURN_IF_FAIL(ret, code)   \
    do {                            \
        if (ret != 0)               \
            return code;            \
    } while (0)

#define RETURN_IF_NULL(ptr, code)   \
    do {                            \
        if (ptr == NULL)            \
            return code;            \
    } while (0)

#define RETURN_IF_TRUE(cond, code)  \
    do {                            \
        if (cond)                   \
            return code;            \
    } while (0)

#ifndef EVTHUB_TRUE
#define EVTHUB_TURE     1
#endif

#ifndef EVTHUB_FALSE
#define EVTHUB_FALSE    0
#endif

/** Common error code 
  */
enum {
    EVTHUB_SUCC = 0,
    EVTHUB_ERR_PTR,
    EVTHUB_ERR_PARAM,
    EVTHUB_ERR_MALLOC,
    EVTHUB_ERR_THREAD,
    EVTHUB_ERR_POOL_MEM,
    EVTHUB_ERR_POOL_SIZE,
    EVTHUB_ERR_POOL_FULL,
    EVTHUB_ERR_POOL_FREE,
    EVTHUB_ERR_POOL_ALLOC,
};

struct evtinfo_t {
    struct listnode node;
    int bit;
    event_t evt;
};

struct thread_ctrl_t {
    unsigned char exit;
    pthread_t tid;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
};

struct event_pool_t {
    pthread_mutex_t mutex;      /*!< Protect event pool */
    unsigned char size;         /*!< Size of event pool */
    unsigned int *bits;
    struct evtinfo_t *array;
};

struct evthub_handle_t {
    struct listnode list;
    struct thread_ctrl_t ctrl;  /*!< Control thread resource */
    evthub_mode mode;
    void *user_data;
    on_event_f notifier;
    struct event_pool_t pool;
};

static int event_pool_create(struct event_pool_t *pool)
{
    char *mem;
    unsigned int b_size, e_size;
    RETURN_IF_TRUE(!pool->size, EVTHUB_ERR_POOL_SIZE);

    b_size = sizeof(unsigned int) * BITS_TO_WORDS(pool->size);
    e_size = sizeof(struct evtinfo_t) * pool->size;

    mem = (char*)malloc(b_size + e_size);
    RETURN_IF_NULL(mem, EVTHUB_ERR_POOL_MEM);

    pthread_mutex_init(&pool->mutex, NULL);
    pool->bits = (unsigned int*)mem;
    pool->array = (struct evtinfo_t*)(mem + b_size);
    bitmask_init(pool->bits, (int)pool->size);

    return EVTHUB_SUCC;
}

static void event_pool_destory(struct event_pool_t *pool)
{
    if (pool->bits) {
        free(pool->bits);
        pool->bits = NULL;
        pool->array = NULL;
    }

    pool->size = 0;
    pthread_mutex_destroy(&pool->mutex);
}

static struct evtinfo_t* event_pool_alloc(struct event_pool_t *pool)
{
    int max, bit;
    struct evtinfo_t* s = NULL;

    pthread_mutex_lock(&pool->mutex);
    max = (int)pool->size;
    bit = bitmask_ffz(pool->bits, max);
    if (bit >= 0 && bit < max) {
        pool->array[bit].bit = bit;
        bitmask_set(pool->bits, bit);
        s = &pool->array[bit];
        list_init(&s->node);
    }
    pthread_mutex_unlock(&pool->mutex);

    return s;
}

static int event_pool_free(struct event_pool_t *pool, struct evtinfo_t *evt)
{
    if (evt->bit >= 0 && evt->bit < (int)pool->size) {
        pthread_mutex_lock(&pool->mutex);
        bitmask_clear(pool->bits, evt->bit);
        pthread_mutex_unlock(&pool->mutex);
        return EVTHUB_SUCC;
    }

    return EVTHUB_ERR_POOL_FREE;
}

static void* thread_routine(evthub_t handle)
{
    struct evthub_handle_t *evthub = (struct evthub_handle_t*)handle;
    RETURN_IF_NULL(evthub, NULL);
    while (!evthub->ctrl.exit) {
        pthread_mutex_lock(&evthub->ctrl.mutex);
        if (list_empty(&evthub->list)) {
            pthread_cond_wait(&evthub->ctrl.cond, &evthub->ctrl.mutex);
            pthread_mutex_unlock(&evthub->ctrl.mutex);
        } else {
            struct listnode *n;
            struct evtinfo_t *e;
            /*! fetch event in the front of list */
            n = list_head(&evthub->list);
            list_remove(n);
            pthread_mutex_unlock(&evthub->ctrl.mutex);

            /*! restore event info and notify user */
            e = list_entry(n, struct evtinfo_t, node);
            evthub->notifier(&e->evt, evthub->user_data);
            /*! Release event to pool */
            event_pool_free(&evthub->pool, e);
        }
    }

    return NULL;
}

int evthub_create(evthub_t *handle, evthub_parm *param)
{
    int s;
    unsigned int size;
    struct evthub_handle_t *evthub;

    /*! Parameter check */
    RETURN_IF_NULL(handle, EVTHUB_ERR_PTR);
    RETURN_IF_NULL(param, EVTHUB_ERR_PTR);
    RETURN_IF_NULL(param->notifier, EVTHUB_ERR_PARAM);
    RETURN_IF_TRUE(param->max < 1, EVTHUB_ERR_PARAM);

    size = sizeof(struct evthub_handle_t)
        + sizeof(struct evtinfo_t) * param->max;
    evthub = (struct evthub_handle_t*)malloc(size);
    RETURN_IF_NULL(evthub, EVTHUB_ERR_MALLOC);
    *handle = (evthub_t)evthub;

    /*! Initialize eventhub */
    list_init(&evthub->list);
    evthub->user_data = param->user_data;
    evthub->notifier = param->notifier;
    evthub->ctrl.exit = EVTHUB_FALSE;
    evthub->pool.size = param->max;
    s = event_pool_create(&evthub->pool);
    RETURN_IF_FAIL(s, s);

    pthread_cond_init(&evthub->ctrl.cond, NULL);
    pthread_mutex_init(&evthub->ctrl.mutex, NULL);

    s = pthread_create(&evthub->ctrl.tid, NULL, &thread_routine, *handle);
    RETURN_IF_FAIL(s, EVTHUB_ERR_THREAD);
    return EVTHUB_SUCC;
}

int evthub_destory(evthub_t handle)
{
    RETURN_IF_NULL(handle, EVTHUB_ERR_PTR);
    struct evthub_handle_t *evthub = (struct evthub_handle_t*)handle;

    /*! Notify thread to exit  */
    pthread_mutex_lock(&evthub->ctrl.mutex);
    evthub->ctrl.exit = EVTHUB_TURE;
    pthread_cond_broadcast(&evthub->ctrl.cond);
    pthread_mutex_unlock(&evthub->ctrl.mutex);

    /*! Waiting untill thread is exited */
    pthread_join(evthub->ctrl.tid, NULL);

    event_pool_destory(&evthub->pool);
    pthread_mutex_destroy(&evthub->ctrl.mutex);
    pthread_cond_destroy(&evthub->ctrl.cond);
    return EVTHUB_SUCC;
}

int evthub_send(const evthub_t handle, const event_t *evt)
{
    struct evtinfo_t *e;
    struct evthub_handle_t *evthub;

    RETURN_IF_NULL(evt, EVTHUB_ERR_PTR);
    RETURN_IF_NULL(handle, EVTHUB_ERR_PTR);
    evthub = (struct evthub_handle_t*)handle;

    /*! Allocate event information */
    e = event_pool_alloc(&evthub->pool);
    RETURN_IF_NULL(e, EVTHUB_ERR_POOL_ALLOC);

    /*! Insert event to list */
    memcpy(&e->evt, evt, sizeof(event_t));
    pthread_mutex_lock(&evthub->ctrl.mutex);
    if (evthub->mode == EVENT_HUB_MODE_FIFO) {
        list_add_tail(&evthub->list, &e->node);
    } else {
        struct listnode *node, *n;
        struct evtinfo_t *entry;
        list_for_each_safe(node, n, &evthub->list) {
            entry = list_entry(node, struct evtinfo_t, node);
            if (e->evt.priority > entry->evt.priority) {
                /*! insert new node to ahead of current node */
                list_add_tail(node, &e->node);
                break;
            }
        }
        if (node == &evthub->list) {
            list_add_tail(&evthub->list, &e->node);
        }
    }

    /*! Notify thread to process event */
#ifndef TEST_ON
    pthread_cond_broadcast(&evthub->ctrl.cond);
#endif
    pthread_mutex_unlock(&evthub->ctrl.mutex);
    return EVTHUB_SUCC;
}

