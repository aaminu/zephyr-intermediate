#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE 1024

#define PRIO_LOW 7
#define PRIO_MED 5
#define PRIO_HGH 3
#define PRIO_COOP -1

void t_low_fn(void *p1, void *p2, void *p3)
{
    while (1) {
        LOG_DBG("T_LOW running");
        k_msleep(300);
    }
}

void t_med_fn(void *p1, void *p2, void *p3)
{
    while (1) {
        LOG_DBG("T_MED running");
        k_msleep(200);
    }
}

void t_high_fn(void *p1, void *p2, void *p3)
{
    while (1) {
        LOG_DBG("T_HIGH running");
        k_msleep(100);
    }
}

void coop_fn(void *p1, void *p2, void *p3)
{
    while (1) {

        for (int i = 0; i < 5; i++) {
            k_busy_wait(40000);

            LOG_INF("T_COOP step %d/5 - still holding CPU tick=%u",
                    i + 1, k_uptime_get_32());
        }

        LOG_INF("T_COOP yielding");

        k_yield();
    }
}

/*
 * Threads Behaviour:
 *
 * Zephyr uses priority based scheduling, where lower
 * priority values (smaller numbers) represent higher scheduling priorities.
 *
 * The three preemptible threads have the following priorities:
 *
 * T_LOW  (priority 7): Runs approximately every 300 ms.
 * T_MED  (priority 5): Runs approximately every 200 ms.
 * T_HIGH (priority 3): Runs approximately every 100 ms.
 *
 * Each thread calls k_msleep(), which moves it into the blocked/
 * waiting state and allows other ready threads to execute.
 *
 * When multiple preemptible threads are ready simultaneously,
 * the scheduler selects the highest priority thread first:
 *
 *     T_HIGH (3) > T_MED (5) > T_LOW (7)
 *
 * These sleep intervals determine the approximate execution
 * frequency, while priority determines the execution order
 * when multiple threads are ready.
 *
 * T_COOP has priority -1, making it a cooperative thread with
 * higher priority than all three preemptible threads.
 *
 * Once T_COOP starts running, the preemptible threads stop
 * executing because T_COOP never sleeps, blocks, or returns.
 *
 * Although T_COOP calls k_yield() after every five busy-wait
 * iterations, k_yield() only allows ready threads of equal or
 * higher priority to execute.
 *
 * Since all other application threads have lower priorities
 * (3, 5, and 7), they cannot take control of the CPU.
 *
 * Consequently, T_COOP repeatedly executes its while(1) loop,
 * preventing the three preemptible threads from running.
 *
 */
K_THREAD_DEFINE(thread_a, STACK_SIZE, t_low_fn,
                NULL, NULL, NULL, PRIO_LOW, 0, 0);
K_THREAD_DEFINE(thread_b, STACK_SIZE, t_med_fn,
                NULL, NULL, NULL, PRIO_MED, 0, 0);
K_THREAD_DEFINE(thread_c, STACK_SIZE, t_high_fn,
                NULL, NULL, NULL, PRIO_HGH, 0, 0);
K_THREAD_DEFINE(thread_d, STACK_SIZE, coop_fn,
                NULL, NULL, NULL, PRIO_COOP, 0, 0);


int main(void)
{
    return 0;
}

