#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(l4_homework, LOG_LEVEL_DBG);

#define STACK_SIZE       2048
#define SENSOR_PERIOD_MS  100

/*ChannelMessage - Shared aka No Ownership*/
struct sensor_data
{
    int32_t flow_l_s;
    int32_t temperature_c;
    uint32_t timestamp_ms;
    uint8_t seq;
};

/* Proto */
static void display_listener_cb(const struct zbus_channel *chan);

/* Observers */
ZBUS_LISTENER_DEFINE(display_listener, display_listener_cb); // Listerner
ZBUS_SUBSCRIBER_DEFINE(logger_sub, 4); // Observer

/* Channel */

ZBUS_CHAN_DEFINE(sensor_chan, 
                 struct sensor_data,
                 NULL, 
                 NULL,
                 ZBUS_OBSERVERS(display_listener, logger_sub),
                 ZBUS_MSG_INIT(.flow_l_s = 0,
                               .temperature_c = 0,
                               .timestamp_ms = 0,
                               .seq = 0));


/*Listener-synchronous*/

static void display_listener_cb(const struct zbus_channel *chan)
{
    const struct sensor_data *msg =
        (const struct sensor_data *)zbus_chan_const_msg(chan);

    LOG_INF("[DISPLAY-LIS] thread=%s seq=%u flow=%d l/s temp=%d C",
            k_thread_name_get(k_current_get()),
            msg->seq,
            msg->flow_l_s,
            msg->temperature_c);
}


/*  Publisher */
static void sensor_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    k_thread_name_set(k_current_get(), "sensor");
    int i = 0;

    while (true)
    {
        i++;
        struct sensor_data data = {
            .flow_l_s = 5 + (i * 47),
            .temperature_c = 24000 + (i * 350),
            .timestamp_ms = k_uptime_get_32(),
            .seq = (uint8_t)i,
        };

        LOG_INF("[SENSOR] publish seq=%u",
                data.seq);

        int ret = zbus_chan_pub(&sensor_chan, &data, K_MSEC(100));
        if (ret != 0) 
        {
            LOG_WRN("[SENSOR] publish failed ret=%d", ret);
        }

        k_msleep(SENSOR_PERIOD_MS);
    }
}


/*Message subscriber - logger*/
static void logger_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    k_thread_name_set(k_current_get(), "logger");

    const struct zbus_channel *chan;

    while (true)
    {
        int ret = zbus_sub_wait(&logger_sub, &chan, K_FOREVER);
        if (ret != 0) 
        {
            LOG_ERR("[LOGGER-SUB] Error Out");
            continue;
        }

        struct sensor_data msg;
        ret = zbus_chan_read(chan, &msg, K_MSEC(100));
        if (ret != 0)
        {
            LOG_WRN("[LOGGER-SUB] read failed ret=%d", ret);
            continue;
        }

        LOG_INF("[LOGGER-MSG] thread=%s seq=%u flow=%d l/s temp=%d C latency=%ums",
                k_thread_name_get(k_current_get()),
                msg.seq,
                msg.flow_l_s,
                msg.temperature_c,
                k_uptime_get_32() - msg.timestamp_ms);

        k_msleep(350);
        
    }
}


/*Threads*/

K_THREAD_DEFINE(sensor_thread, STACK_SIZE, sensor_thread_fn,
                NULL, NULL, NULL, 5, 0, 0);

K_THREAD_DEFINE(logger_thread, STACK_SIZE, logger_thread_fn,
                NULL, NULL, NULL, 6, 0, 0);

/* Main */
int main(void)
{
    LOG_INF("sensor publishes every %dms", SENSOR_PERIOD_MS);
    LOG_INF("display listener runs in publisher context");
    LOG_INF("logger uses a regular subscriber");

    return 0;
}
