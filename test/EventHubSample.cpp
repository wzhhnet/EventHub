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

#include <unistd.h>
#include <EventHub.h>

using namespace utils;

class SampleEvent : public Event
{
  public:
    SampleEvent(uint32_t id, const char *name, EvtPriority pri)
      : id_(id), name_(name), pri_(pri) {}
    virtual ~SampleEvent() {}

    virtual uint32_t ID() const{ return id_;}
    virtual const char* Name() const { return name_.c_str(); }
    virtual EvtPriority Priority() const { return pri_; }

  private:
    uint32_t id_;
    std::string name_;
    EvtPriority pri_;
};

class SampleHandler : public EventHandler
{
  public:
    virtual ~SampleHandler() {}
    virtual void OnEvent(const SpEvent evt)
    {
        if (evt != nullptr) {
            printf("recv: ID=%u Name=%s, Priority=%d\n",
                evt->ID(), evt->Name(), static_cast<int>(evt->Priority()));
        }
    }
};

int main(int argc, char **argv)
{
    std::unique_ptr<SampleHandler> handler(new SampleHandler());
    std::unique_ptr<EventHub> evthub(new EventHub(handler.get(), 4));

    usleep(1000);
    SpEvent evt1(new SampleEvent(1, "event1", EvtPriority::kEvtPriLow));
    SpEvent evt2(new SampleEvent(2, "event2", EvtPriority::kEvtPriMid));
    SpEvent evt3(new SampleEvent(3, "event3", EvtPriority::kEvtPriHigh));
    SpEvent evt4(new SampleEvent(4, "event4", EvtPriority::kEvtPriHigh));

    evthub->Send(evt1);
    evthub->Send(evt2);
    evthub->Send(evt3);
    evthub->Send(evt4);
    evthub->Signal();

#if 0 // test cancel
    std::thread t([&](){
        usleep(1000);
        evthub->Cancel();
    });

    evthub->Join();
    t.join();
#else
    usleep(1000);
#endif

    printf("main thread end\n");
    return 0;
}


