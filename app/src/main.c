#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE 1024

#define PRIO_LOW 7
#define PRIO_MED 5
#define PRIO_HGH 3
#define PRIO_COOP -1
#define INCREMENTS 1000000 /* each thread increments this many times */

static K_MUTEX_DEFINE(counter_mutex);

/* Shared state - intentionally unprotected */ static volatile uint32_t counter;

void t_low_fn(void *p1, void *p2, void *p3)
{
    while (1) {
        LOG_DBG("T_LOW running");
        k_msleep(300);
    }
}

void t_med_fn(void *p1, void *p2, void *p3) 
{ 
   
    const char *name = k_thread_name_get(k_current_get()); 
    LOG_INF("[%s] Started: %u", name, counter); 
    
    for (int i = 0; i < INCREMENTS; i++) 
    { 
        k_mutex_lock(&counter_mutex, K_FOREVER);
        counter++; 
        k_mutex_unlock(&counter_mutex);
    } 
    LOG_INF("[%s] finished: %u", name, counter); 
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
    for (int i = 0; i < 5; i++) 
    {
        k_busy_wait(40000);

        LOG_INF("T_COOP step %d/5 - still holding CPU tick=%u",
                i + 1, k_uptime_get_32());
    }

    LOG_INF("T_COOP yielding");
    k_yield();
}

K_THREAD_DEFINE(thread_a, STACK_SIZE, t_low_fn, NULL, NULL, NULL, PRIO_LOW, 0, 0); 
K_THREAD_DEFINE(thread_b, STACK_SIZE, t_med_fn, NULL, NULL, NULL, PRIO_MED, 0, 0); 
K_THREAD_DEFINE(thread_c, STACK_SIZE, t_med_fn, NULL, NULL, NULL, PRIO_MED, 0, 0); 
// K_THREAD_DEFINE(thread_d, STACK_SIZE, coop_fn, NULL, NULL, NULL, PRIO_COOP, 0, 0);


int main(void)
{
    printk("Hello World");
    return 0;
}

