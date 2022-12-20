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
#include "allocator.h"
#include "event_hub.h"

struct evtinfo_t {
    struct listnode node;
    event_t evt;
};

struct thread_ctrl_t {
    unsigned char exit;
    pthread_t tid;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
};


ALLOCATOR_DECLARE(evthub, struct evtinfo_t);
ALLOCATOR_IMPLEMENT(evthub, struct evtinfo_t);

struct evthub_handle_t {
    struct listnode list;
    struct thread_ctrl_t ctrl;  /*!< Control thread resource */
    evthub_mode mode;
    void *user_data;
    on_event_f notifier;
    ALLOCATOR_DEFINE(evthub, pool);
};

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
            ALLOCATOR_FREE(evthub, &evthub->pool, e);
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
    RETURN_IF_NULL(handle, UTILS_ERR_PTR);
    RETURN_IF_NULL(param, UTILS_ERR_PTR);
    RETURN_IF_NULL(param->notifier, UTILS_ERR_PARAM);
    RETURN_IF_TRUE(param->max < 1, UTILS_ERR_PARAM);

    size = sizeof(struct evthub_handle_t);
    evthub = (struct evthub_handle_t*)malloc(size);
    RETURN_IF_NULL(evthub, UTILS_ERR_MALLOC);
    *handle = (evthub_t)evthub;

    /*! Initialize eventhub */
    list_init(&evthub->list);
    evthub->user_data = param->user_data;
    evthub->notifier = param->notifier;
    evthub->ctrl.exit = false;
    s = ALLOCATOR_CREATE(evthub, &evthub->pool, param->max);
    RETURN_IF_FAIL(s, s);

    pthread_cond_init(&evthub->ctrl.cond, NULL);
    pthread_mutex_init(&evthub->ctrl.mutex, NULL);

    s = pthread_create(&evthub->ctrl.tid, NULL, &thread_routine, *handle);
    RETURN_IF_FAIL(s, UTILS_ERR_THREAD);
    return UTILS_SUCC;
}

int evthub_destory(evthub_t *handle)
{
    RETURN_IF_NULL(handle, UTILS_ERR_PTR);
    struct evthub_handle_t *evthub = (struct evthub_handle_t*)(*handle);
    RETURN_IF_NULL(evthub, UTILS_ERR_PTR);
    /*! Notify thread to exit  */
    pthread_mutex_lock(&evthub->ctrl.mutex);
    evthub->ctrl.exit = true;
    pthread_cond_broadcast(&evthub->ctrl.cond);
    pthread_mutex_unlock(&evthub->ctrl.mutex);

    /*! Waiting untill thread is exited */
    pthread_join(evthub->ctrl.tid, NULL);

    ALLOCATOR_DESTORY(evthub, &evthub->pool);
    pthread_mutex_destroy(&evthub->ctrl.mutex);
    pthread_cond_destroy(&evthub->ctrl.cond);
    free(*handle);
    *handle = NULL;
    return UTILS_SUCC;
}

int evthub_send(const evthub_t handle, const event_t *evt)
{
    struct evtinfo_t *e;
    struct evthub_handle_t *evthub;

    RETURN_IF_NULL(evt, UTILS_ERR_PTR);
    RETURN_IF_NULL(handle, UTILS_ERR_PTR);
    evthub = (struct evthub_handle_t*)handle;

    /*! Allocate event information */
    e = ALLOCATOR_ALLOC(evthub, &evthub->pool);
    RETURN_IF_NULL(e, UTILS_ERR_POOL_ALLOC);
    list_init(&e->node);

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
    return UTILS_SUCC;
}

