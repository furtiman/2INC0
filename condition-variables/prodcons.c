/* 
 * Operating Systems {2INCO} Practical Assignment
 * Condition Variables Application
 *
 * Ivan Turasov (0988297)
 * Roy Meijer (1522329)
 *
 * Grading:
 * Students who hand in clean code that fully satisfies the minimum requirements will get an 8. 
 * Extra steps can lead to higher marks because we want students to take the initiative. 
 * Extra steps can be, for example, in the form of measurements added to your code, a formal 
 * analysis of deadlock freeness etc.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "prodcons.h"

#define BUFFER_INDEX BUFFER_SIZE - 1
// Condition variables
static pthread_cond_t more = PTHREAD_COND_INITIALIZER, less = PTHREAD_COND_INITIALIZER;
// Mutual exclusion variables
static pthread_mutex_t main_mutex = PTHREAD_MUTEX_INITIALIZER, buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
// Buffer
static ITEM buffer[BUFFER_SIZE];
// Functions
static void rsleep(int t);
static ITEM get_next_item(void);
// Threads
static void *producer(void *arg);
static void *consumer(void *arg);
// Buffer control variables
static ITEM items_in_buffer = 0;
static ITEM items_produced = 0;
static ITEM items_consumed = 0;

int main(void)
{
    pthread_t pro_thread[NROF_PRODUCERS];
    pthread_t con_thread;

    for (size_t i = 0; i < NROF_PRODUCERS; i++)
    {
        pthread_create(&pro_thread[i], NULL, producer, NULL);
    }
    pthread_create(&con_thread, NULL, consumer, NULL);

    /* join in loop */
    for (size_t i = 0; i < NROF_PRODUCERS; i++)
    {
        pthread_join(pro_thread[i], NULL);
    }
    pthread_join(con_thread, NULL);

    return 0;
}

/**
 * @brief Producer thread routine
 * 
 * @param arg Arguments passed to the thread
 * @return void* 
 */
static void *producer(void *arg)
{
    while (items_produced < NROF_ITEMS)
    {
        rsleep(100);
        pthread_mutex_lock(&main_mutex);
        ITEM current_item = get_next_item();
        // Make sure the items are put in the right order
        while (items_produced != current_item)
        {
            pthread_cond_wait(&more, &main_mutex); // Wait while the new item is added
        }
        // Make sure the buffer has free space
        while (items_in_buffer >= BUFFER_SIZE)
        {
            pthread_cond_wait(&less, &main_mutex); // Wait while an item is removed from the buffer
        }

        if (current_item != NROF_ITEMS)
        {
            pthread_mutex_lock(&buffer_mutex);
            buffer[BUFFER_INDEX - items_in_buffer] = current_item;
            pthread_mutex_unlock(&buffer_mutex);
            items_in_buffer++;
            items_produced++;
            pthread_cond_broadcast(&more);
        }
        pthread_mutex_unlock(&main_mutex);
    }
    return NULL;
}

/**
 * @brief Consumer thread routine
 * 
 * @param arg Arguments passed to the thread
 * @return void* 
 */
static void *consumer(void *arg)
{
    while (items_consumed < NROF_ITEMS)
    {
        rsleep(100);
        pthread_mutex_lock(&main_mutex);
        while (items_in_buffer == 0) // If the buffer is empty
        {
            pthread_cond_wait(&more, &main_mutex);
        }
        pthread_mutex_lock(&buffer_mutex);
        ITEM received_item = buffer[BUFFER_INDEX];
        items_in_buffer--;
        for (size_t i = 0; i < items_in_buffer; ++i)
        {
            buffer[BUFFER_INDEX - i] = buffer[BUFFER_INDEX - 1 - i];
        }
        pthread_mutex_unlock(&buffer_mutex);
        items_consumed++;
        pthread_cond_broadcast(&less);
        pthread_mutex_unlock(&main_mutex);
        printf("%d\n", received_item);
    }
    return NULL;
}

/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
static void rsleep(int t)
{
    static bool first_call = true;

    if (first_call == true)
    {
        srandom(time(NULL));
        first_call = false;
    }
    usleep(random() % t);
}

/* 
 * get_next_item()
 *
 * description:
 *		thread-safe function to get a next job to be executed
 *		subsequent calls of get_next_item() yields the values 0..NROF_ITEMS-1 
 *		in arbitrary order 
 *		return value NROF_ITEMS indicates that all jobs have already been given
 * 
 * parameters:
 *		none
 *
 * return value:
 *		0..NROF_ITEMS-1: job number to be executed
 *		NROF_ITEMS:		 ready
 */
static ITEM get_next_item(void)
{
    static pthread_mutex_t job_mutex = PTHREAD_MUTEX_INITIALIZER;
    static bool jobs[NROF_ITEMS + 1] = {false}; // keep track of issued jobs
    static int counter = 0;                     // seq.nr. of job to be handled
    ITEM found;                                 // item to be returned

    /* avoid deadlock: when all producers are busy but none has the next expected item for the consumer 
	 * so requirement for get_next_item: when giving the (i+n)'th item, make sure that item (i) is going to be handled (with n=nrof-producers)
	 */
    pthread_mutex_lock(&job_mutex);

    counter++;
    if (counter > NROF_ITEMS)
    {
        // we're ready
        found = NROF_ITEMS;
    }
    else
    {
        if (counter < NROF_PRODUCERS)
        {
            // for the first n-1 items: any job can be given
            // e.g. "random() % NROF_ITEMS", but here we bias the lower items
            found = (random() % (2 * NROF_PRODUCERS)) % NROF_ITEMS;
        }
        else
        {
            // deadlock-avoidance: item 'counter - NROF_PRODUCERS' must be given now
            found = counter - NROF_PRODUCERS;
            if (jobs[found])
            {
                // already handled, find a random one, with a bias for lower items
                found = (counter + (random() % NROF_PRODUCERS)) % NROF_ITEMS;
            }
        }

        // check if 'found' is really an unhandled item;
        // if not: find another one
        if (jobs[found])
        {
            // already handled, do linear search for the oldest
            found = 0;
            while (jobs[found])
            {
                found++;
            }
        }
    }
    jobs[found] = true;

    pthread_mutex_unlock(&job_mutex);
    return found;
}
