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

#include <assert.h>

#include "prodcons.h"

/* buffer control variables */
static uint8_t occupied = 0, nextin = 0, nextout = 0;
/* condition variables */
static pthread_cond_t more = PTHREAD_COND_INITIALIZER, less = PTHREAD_COND_INITIALIZER;
/* mutual exclusion variables */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* buffer */
static ITEM buffer[BUFFER_SIZE];

/* functions */
static void rsleep(int t);
static ITEM get_next_item(void);

/* threads */
static void *producer(void *arg);
static void *consumer(void *arg);

int main(void)
{
	/* defines threads for producers and one consumer thread */
	pthread_t pro_thread[NROF_PRODUCERS];
	pthread_t con_thread;

	/* create in loop */
	for (size_t i = 0; i < NROF_PRODUCERS; i++) {
    	pthread_create(&pro_thread[i], NULL, producer, NULL);
	}
	pthread_create(&con_thread, NULL, consumer, NULL);

	/* join in loop */
	for (size_t i = 0; i < NROF_PRODUCERS; i++) {
    	pthread_join(pro_thread[i], NULL);
	}
	pthread_join(con_thread, NULL);

	return 0;
}

/* producer thread */
static void *producer(void *arg)
{
	/* keeps track of number of items produced */
	static uint16_t items_produced = 0;
	/* item variable can be used to bypass get_next_item(), to check if the fifo works */
	static ITEM item;
	/* is this considered busy waiting? */
	while (items_produced < NROF_ITEMS) {
		/* rsleep is required */
		items_produced++;
		rsleep(100);
		pthread_mutex_lock(&mutex);

		while (occupied >= BUFFER_SIZE)
			pthread_cond_wait(&less, &mutex);

		assert(occupied < BUFFER_SIZE);

		buffer[nextin++] = item++;

		nextin %= BUFFER_SIZE;
		occupied++;

		pthread_cond_signal(&more);
		pthread_mutex_unlock(&mutex);
	}
	return NULL;
}

/* consumer thread */
static void *consumer(void *arg)
{
	/* temporarely stores items */
	ITEM item;

	/* stops consuming when all items are consumed */
	for (size_t i = 0; i < NROF_ITEMS; i++) {
		/* rsleep is required */
		rsleep(100);
		pthread_mutex_lock(&mutex);
		while (occupied <= 0)
			pthread_cond_wait(&more, &mutex);

		assert(occupied > 0);

		item = buffer[nextout++];
		nextout %= BUFFER_SIZE;
		occupied--;

		pthread_cond_signal(&less);
		pthread_mutex_unlock(&mutex);

		printf("%d\n", item);
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
	static int counter = 0;						// seq.nr. of job to be handled
	ITEM found;									// item to be returned

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
