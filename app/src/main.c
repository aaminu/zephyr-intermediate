#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/task_wdt/task_wdt.h>

LOG_MODULE_REGISTER(l5_task1, LOG_LEVEL_DBG);

#define STACK_SIZE              2048
#define SENSOR_PERIOD_MS        100
#define QUEUE_CAPACITY          8
#define QUEUE_WARNING_PERCENT   75
#define HEALTH_PERIOD_MS        500
#define CONSUMER_WDT_TIMEOUT_MS 2000
#define STUCK_CONSUMER_MS       5000
#define STUCK_AFTER_MESSAGES    5


struct sensor_data
{
    int32_t flow_l_s;
    int32_t temperature_c;
    uint32_t timestamp_ms;
    uint8_t seq;
};

/*Queue*/
K_MSGQ_DEFINE(sensor_queue, sizeof(struct sensor_data), QUEUE_CAPACITY, 4);

/* Watchdog*/
static int consumer_wdt_channel;

/*Task watchdog callback*/
static void consumer_wdt_callback(int channel_id, void *user_data)
{
    ARG_UNUSED(user_data);

    LOG_ERR("========================================");
    LOG_ERR("[WDT] CONSUMER WATCHDOG FIRED!");
    LOG_ERR("[WDT] channel=%d", channel_id);
    LOG_ERR("[WDT] consumer failed to make progress");
    LOG_ERR("========================================");
}

/* Producer*/
static void sensor_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    k_thread_name_set(k_current_get(), "sensor");

    uint8_t seq = 0;

    while (true)
    {
        seq++;

        struct sensor_data data = {
            .flow_l_s = 5 + (seq * 47),
            .temperature_c = 24000 + (seq * 350),
            .timestamp_ms = k_uptime_get_32(),
            .seq = seq,
        };

        uint32_t used = k_msgq_num_used_get(&sensor_queue);

        LOG_INF("[PRODUCER] seq=%u queue=%u/%u", data.seq, used, QUEUE_CAPACITY);

        int ret = k_msgq_put(&sensor_queue, &data, K_NO_WAIT);
        if (ret)
        {
            LOG_WRN("[PRODUCER] QUEUE FULL - dropping seq=%u",data.seq);
        }

        k_msleep(SENSOR_PERIOD_MS);
    }
}


/* Consumer*/
static void logger_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    k_thread_name_set(k_current_get(), "logger");

    struct sensor_data msg;

    uint32_t consumed = 0;
    bool failure_injected = false;

    while (true)
    {
        int ret = k_msgq_get(&sensor_queue, &msg, K_FOREVER);
        if (ret)
        {
            LOG_ERR("[CONSUMER] queue read failed ret=%d", ret);
            continue;
        }

        consumed++;
        LOG_INF("[CONSUMER] seq=%u flow=%d temp=%d latency=%ums",
                msg.seq,
                msg.flow_l_s,
                msg.temperature_c,
                k_uptime_get_32() - msg.timestamp_ms);

        /* Feed the dog*/
        ret = task_wdt_feed(consumer_wdt_channel);
        if (ret)
        {
            LOG_ERR("[CONSUMER] watchdog feed failed ret=%d", ret);
        }


        /*
         * Failure injection.
         *
         * Simulate the consumer becoming stuck.
         */
        if (!failure_injected && consumed >= STUCK_AFTER_MESSAGES)
        {
            failure_injected = true;

            LOG_ERR("========================================");
            LOG_ERR("[CONSUMER] SIMULATING STUCK THREAD");
            LOG_ERR("[CONSUMER] sleeping for %d ms", STUCK_CONSUMER_MS);
            LOG_ERR("========================================");

            k_msleep(STUCK_CONSUMER_MS);
            LOG_INF("[CONSUMER] recovered from simulated stall");
        }

        /*Normal consumer processing delay*/
        k_msleep(350);
    }
}


/* Health Monitor */
static void health_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    k_thread_name_set(k_current_get(), "health");

    while (true)
    {
        uint32_t used = k_msgq_num_used_get(&sensor_queue);
        uint32_t percent = (used * 100U) / QUEUE_CAPACITY;

        LOG_DBG("[HEALTH] queue=%u/%u (%u%%)",
                used,
                QUEUE_CAPACITY,
                percent);

        if (percent >= QUEUE_WARNING_PERCENT)
        {
            LOG_WRN("[HEALTH] QUEUE HIGH: %u/%u (%u%%)",
                    used,
                    QUEUE_CAPACITY,
                    percent);
        }

        k_msleep(HEALTH_PERIOD_MS);
    }
}


/* Threads */

K_THREAD_DEFINE(sensor_thread, STACK_SIZE, sensor_thread_fn, NULL, NULL, NULL, 5, 0, 0);

K_THREAD_DEFINE(logger_thread, STACK_SIZE,logger_thread_fn, NULL, NULL, NULL, 6, 0, 0);

K_THREAD_DEFINE(health_thread, STACK_SIZE, health_thread_fn, NULL, NULL, NULL, 7, 0, 0);


int main(void)
{
    LOG_INF("========================================");
    LOG_INF("Lecture 5 - Task 1");
    LOG_INF("Reliability under pressure");
    LOG_INF("========================================");

    LOG_INF("Producer period: %d ms",
            SENSOR_PERIOD_MS);

    LOG_INF("Queue capacity: %d",
            QUEUE_CAPACITY);

    LOG_INF("Health warning threshold: %d%%",
            QUEUE_WARNING_PERCENT);

    LOG_INF("Consumer watchdog: %d ms",
            CONSUMER_WDT_TIMEOUT_MS);


    /*
     * Initialize software task watchdog. No hardware watchdog
     * as the fallback for this exercise.*/
    int ret = task_wdt_init(NULL);
    if (ret)
    {
        LOG_ERR("task_wdt_init failed ret=%d", ret);
        return ret;
    }

    /* Register watchdog channel for consumer.*/
    consumer_wdt_channel = task_wdt_add(CONSUMER_WDT_TIMEOUT_MS, consumer_wdt_callback, NULL);
    if (consumer_wdt_channel < 0)
    {
        LOG_ERR("task_wdt_add failed ret=%d", consumer_wdt_channel);
        return consumer_wdt_channel;
    }

    LOG_INF("Consumer watchdog channel=%d", consumer_wdt_channel);
    
    ret = task_wdt_feed(consumer_wdt_channel);
    if (ret)
    {
        LOG_ERR("Initial watchdog feed failed ret=%d", ret);
    }

    return 0;
}