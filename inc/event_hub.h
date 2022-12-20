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

#ifndef UTILS_EVENT_HUB_C_H
#define UTILS_EVENT_HUB_C_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define EVENT_ID_USER_BASE  (0x00)
#define EVENT_ID_SYS_BASE   (0xE0)
#define EVENT_ID_END        (0xFF)

typedef void* evthub_t;
typedef unsigned char event_id;

typedef enum {
    EVENT_HUB_MODE_FIFO = 0,
    EVENT_HUB_MODE_PRIORITY
} evthub_mode;

typedef struct {
    unsigned char id;           /*!< Event indentifier */
    unsigned char priority;     /*!< Event priority (for EVENT_HUB_MODE_PRIORITY mode) */
    void *param;                /*!< The parameters that current event carries */
} event_t;

/*! \fn void (*on_event_f)(const event_t* event, void* user_data);
    \brief Callback function for event notification.
    \param event  (I) Pointer of event_hub handle.
    \param user_data  (I) User data held by event_hub.
    \return none
*/
typedef void (*on_event_f)(const event_t*, void*);

typedef struct {
    unsigned char max;          /*!< Maximum event allowed in hub */
    evthub_mode mode;           /*!< Event arrangement mode in hub */
    void *user_data;            /*!< User data held by event_hub */
    on_event_f notifier;        /*!< Callback function for event notification */
} evthub_parm;

/*! \fn void evthub_create(evthub_t *handle,on_event_f cb)
    \brief Create a event_hub handle.
    \param handle (O) Pointer of event_hub handle.
    \param param  (I) Configuration parameter of event_hub.
    \return 0 if success else error code
*/
int evthub_create(evthub_t *handle, evthub_parm *param);

/*! \fn void evthub_destory(evthub_t *handle)
    \brief Release a event_hub handle
           unprocessed events will be discarded.
    \param handle (I) Handle of event_hub.
    \return 0 if success else error code
*/
int evthub_destory(evthub_t *handle);

/*! \fn void evthub_send(evthub_t *handle,const event_t *evt)
    \brief Send a event to event_hub.
    \param handle (I) Handle of event_hub.
    \param evt    (I) Pointer of event.
    \return 0 if success else error code
*/
int evthub_send(const evthub_t handle, const event_t *evt);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*!< UTILS_EVENT_HUB_C_H */

