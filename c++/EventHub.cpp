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

#include "EventHub.h"

namespace utils {

EventHub::EventHub(EventHandler *handler, size_t max)
  : mutex_()
  , cond_()
  , seq_no_(0)
  , evtque_()
  , max_size_(max)
  , handler_(handler)
  , thread_(new std::thread(&EventHub::StartRoutine, this))
  , exit_(false)
{
}

EventHub::~EventHub()
{
    if (!exit_) {
        std::unique_lock<std::mutex> l(mutex_);
        exit_ = true;
        cond_.notify_one();
    }
    if (thread_->joinable()) {
        thread_->join();
    }
}

bool EventHub::Send(const SpEvent &evt)
{
    if (evt == nullptr) return false;
    {
        std::unique_lock<std::mutex> l(mutex_);
        if (evtque_.size() >= max_size_) {
            return false; // Event hub is full.
        }
        evtque_.emplace(evt, seq_no_++);
    }
#ifndef TEST_ON
    cond_.notify_one();
#endif

    return true;
}

void EventHub::Cancel()
{
    std::unique_lock<std::mutex> l(mutex_);
    exit_ = true;
    cond_.notify_one();
}

#ifdef TEST_ON
void EventHub::Signal() { cond_.notify_one(); }

void EventHub::Join() {
    if (thread_->joinable()) {
        thread_->join();
    }
}
#endif

void EventHub::StartRoutine()
{
    while (EventLoop());
}

bool EventHub::EventLoop()
{
    Element e;
    {
        std::unique_lock<std::mutex> l(mutex_);
        if (exit_) return false;
        if (evtque_.empty()) {
            seq_no_= 0; // reset sequence number
            cond_.wait(l);
            return true;
        } else {
            e = evtque_.top();
            evtque_.pop();
        }
    }

    if (handler_)
        handler_->OnEvent(e.evt_);

    return true;
}

bool EventHub::Element::operator<(const Element &orig) const
{
    if (evt_->Priority() != orig.evt_->Priority()) {
        return evt_->Priority() > orig.evt_->Priority();
    } else {
        return seq_ > orig.seq_;
    }
}

};

