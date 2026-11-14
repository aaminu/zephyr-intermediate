#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE 1024
#define PRIO_MED 5
#define INCREMENTS 10000000 /* each thread increments this many times */

/* Set to 1 for protected, 0 for unprotected */
#ifndef USE_MUTEX
#define USE_MUTEX 0
#endif

static K_MUTEX_DEFINE(counter_mutex);

/* Shared state - accessed by both worker */ 
static volatile uint32_t counter = 0;

void worker_fn(void *p1, void *p2, void *p3) 
{ 
   
    const char *name = k_thread_name_get(k_current_get()); 
    LOG_INF("[%s] Started", name); 
    
    for (int i = 0; i < INCREMENTS; i++) 
    { 
#if USE_MUTEX
        k_mutex_lock(&counter_mutex, K_FOREVER);
#endif
        counter++;

#if USE_MUTEX
        k_mutex_unlock(&counter_mutex);
#endif

    } 
    LOG_INF("[%s] finished", name); 
}


K_THREAD_DEFINE(thread_b, STACK_SIZE, worker_fn, NULL, NULL, NULL, PRIO_MED, 0, 0); 
K_THREAD_DEFINE(thread_c, STACK_SIZE, worker_fn, NULL, NULL, NULL, PRIO_MED, 0, 0); 



int main(void)
{
    printk("Counter race condition demo\n");

    /* Wait for BOTH workers to terminate */
    k_thread_join(thread_b, K_FOREVER);
    k_thread_join(thread_c, K_FOREVER);

    uint32_t expected = 2U * INCREMENTS;

    printk("Mutex: %s\n", USE_MUTEX ? "ENABLED" : "DISABLED");
    printk("Expected total: %u\n", expected);
    printk("Actual total:   %u\n", counter);
    printk("Lost updates:   %u\n", expected - counter);

    return 0;
}

