#include <unistd.h>
#include <gtest/gtest.h>
#include <event_hub.c>

evthub_t handle = NULL;

static void event_recv(const event_t *evt, void *data)
{
    if (evt) {
        printf("recv: id(%u) pri(%u) param(%p)\n",evt->id, evt->priority, evt->param);
    }
}

class GlobalTest : public testing::Environment
{
  public:
    virtual void SetUp()
    {
        evthub_parm param = {
            .max = 4,
            .mode = EVENT_HUB_MODE_FIFO,
            .user_data = NULL,
            .notifier = event_recv
        };

        evthub_create(&handle, &param);
    }

    virtual void TearDown()
    {
        evthub_destory(handle);
    }
    
};

GTEST_API_ int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    testing::Environment *env = new GlobalTest();
    testing::AddGlobalTestEnvironment(env); 
    return RUN_ALL_TESTS();
}

TEST(evthub, evthub_create)
{
    int s;
    struct evthub_handle_t *evthub = (struct evthub_handle_t*)handle;
    const int max = evthub->pool.size;
    struct evtinfo_t *e5, *e[max] = {};
    for (int i=0; i<max; ++i) {
        e[i] = ALLOCATOR_ALLOC(evthub, &evthub->pool);
        EXPECT_EQ(e[i], &evthub->pool.array[i].evt);
    }

    e5 = ALLOCATOR_ALLOC(evthub, &evthub->pool);
    EXPECT_EQ(e5, (struct evtinfo_t *)NULL);

    for (int i=0; i<max; ++i) {
        s = ALLOCATOR_FREE(evthub, &evthub->pool, e[i]);
        EXPECT_EQ(s, UTILS_SUCC);
    }
}

TEST(evthub, evthub_send_fifo)
{
    int s;
    struct evthub_handle_t *evthub;
    event_t evt = {
        .id = 1,
        .priority = 1,
        .param = NULL
    };
    evthub = (struct evthub_handle_t*)handle;

    s = evthub_send(handle, &evt);
    EXPECT_EQ(s, UTILS_SUCC);
    evt.id = 2;
    evt.priority = 2;
    s = evthub_send(handle, &evt);
    EXPECT_EQ(s, UTILS_SUCC);
    evt.id = 3;
    evt.priority = 3;
    s = evthub_send(handle, &evt);
    EXPECT_EQ(s, UTILS_SUCC);
    evt.id = 4;
    evt.priority = 4;
    s = evthub_send(handle, &evt);
    EXPECT_EQ(s, UTILS_SUCC);
    evt.id = 5;
    evt.priority = 5;
    s = evthub_send(handle, &evt);
    EXPECT_EQ(s, UTILS_ERR_POOL_ALLOC);

    pthread_mutex_lock(&evthub->ctrl.mutex);
    pthread_cond_broadcast(&evthub->ctrl.cond);
    pthread_mutex_unlock(&evthub->ctrl.mutex);
    usleep(1000);
}

TEST(evthub, evthub_send_priority)
{
    int s;
    struct evthub_handle_t *evthub;
    event_t evt = {
        .id = 1,
        .priority = 1,
        .param = NULL
    };
    evthub = (struct evthub_handle_t*)handle;
    evthub->mode = EVENT_HUB_MODE_PRIORITY;

    s = evthub_send(handle, &evt);
    EXPECT_EQ(s, UTILS_SUCC);
    evt.id = 2;
    evt.priority = 2;
    s = evthub_send(handle, &evt);
    EXPECT_EQ(s, UTILS_SUCC);
    evt.id = 3;
    evt.priority = 3;
    s = evthub_send(handle, &evt);
    EXPECT_EQ(s, UTILS_SUCC);
    evt.id = 4;
    evt.priority = 4;
    s = evthub_send(handle, &evt);
    EXPECT_EQ(s, UTILS_SUCC);
    evt.id = 5;
    evt.priority = 5;
    s = evthub_send(handle, &evt);
    EXPECT_EQ(s, UTILS_ERR_POOL_ALLOC);

    pthread_mutex_lock(&evthub->ctrl.mutex);
    pthread_cond_broadcast(&evthub->ctrl.cond);
    pthread_mutex_unlock(&evthub->ctrl.mutex);
    usleep(1000);
}

