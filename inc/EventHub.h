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

#ifndef UTILS_EVENT_HUB_CPP_H
#define UTILS_EVENT_HUB_CPP_H

#include <mutex>
#include <queue>
#include <memory>
#include <thread>
#include <condition_variable>

namespace utils {
class Event;
class EventCompare;

/*! \brief A enum class for event priority
 */
enum class EvtPriority {
    kEvtPriHigh = 0,
    kEvtPriMid,
    kEvtPriLow
};

/*! Type of shared_ptr for Event */
using SpEvent = std::shared_ptr<Event>;
/*! Type of unique_ptr for Event */
using UpThread = std::unique_ptr<std::thread>;

/*! \brief Abstracted event class.
 */
class Event
{
  public:
    /*! return event identifier */
    virtual uint32_t ID() const = 0;
    /*! return event description */
    virtual const char* Name() const = 0;
    /*! return event priority */
    virtual EvtPriority Priority() const = 0;
};

/*! \brief A abstracted class for user notification interface.
 */
class EventHandler
{
  public:
    /*! user notification interface */
    virtual void OnEvent(const SpEvent evt) = 0;
};

/*! \brief Event hub class.
 */
class EventHub
{
  public:
    /*! \brief Constructor.
     *  \param handler user notification handler
     *  \param max maximum number of events in queue
     */

    EventHub(EventHandler *handler, size_t max);
    /*! \brief Destructor.
     */
    virtual ~EventHub();

    /*! \brief Asynchronous sending event method.
     */
    bool Send(const SpEvent &evt);

    /*! \brief Discard unprocessed event and terminate EventHub.
     */
    void Cancel();
#ifdef TEST_ON
    /*! \brief Unblocks waiting thread of EventHub
     *         Only for test mode
     */
    void Signal();

    /*! \brief Join the thread of EventHub
     *         Only for test mode
     */
    void Join();
#endif

  private:
    /*! \brief A element of priority queue
     */
    class Element {
      public:
        Element() : evt_(), seq_(0) {}
        Element(const SpEvent &evt, uint32_t seq)
          : evt_(evt), seq_(seq) {}
        bool operator<(const Element &orig) const;
        SpEvent evt_;
        uint32_t seq_;
    };

    /*! Type of priority_queue for SpEvent */
    using EvtQueue = std::priority_queue<Element>;

  private:
    /*! \brief Start routine for internal thread.
     */
    void StartRoutine();

    /*! \brief Loop event queue and notify user.
     */
    bool EventLoop();

  private:
    std::mutex mutex_;
    std::condition_variable cond_;
    uint32_t seq_no_;
    EvtQueue evtque_;
    size_t max_size_;
    EventHandler *handler_;
    UpThread thread_;
    bool exit_;
};

};


#endif /*!< UTILS_EVENT_HUB_CPP_H */

