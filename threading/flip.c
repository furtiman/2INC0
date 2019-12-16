/* 
 * Operating Systems {2INCO} Practical Assignment
 * Threaded Application
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
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h> // for perror()
#include <pthread.h>

#include <stdbool.h>
#include <stddef.h>

#include "flip.h"

#define BITS_IN_UINT128 128

#define BOARD_BIT_POSITION(buffer_cell, position_in_cell) (buffer_cell * BITS_IN_UINT128 + position_in_cell)

#define BUFFER_SIZE ((NROF_PIECES - 1) / BITS_IN_UINT128) + 1

// Create a bitmask where bit at position n is set
#define BITMASK(n) (((uint128_t)1) << (n))
// Check if bit n in v is set
#define BIT_IS_SET(v, n) (((v)&BITMASK(n)) == BITMASK(n))
// Set bit n in v
#define BIT_SET(v, n) ((v) = (v) | BITMASK(n))
// Clear bit n in v
#define BIT_CLEAR(v, n) ((v) = (v) & ~BITMASK(n))

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void create_mask(void *flip_number);

#if NROF_THREADS > 1 // If there is only one thread, there is no point in creating a thread pool
typedef void (*thread_func_t)(void *arg);

// Thread Pool definitions
// Data type for a thread pool job, element of a linked list
typedef struct tpool_work
{
    thread_func_t func;
    void *arg;
    struct tpool_work *next;
} tpool_work_t;

// Thread pool data type
typedef struct tpool
{
    tpool_work_t *work_first;
    tpool_work_t *work_last;
    pthread_mutex_t work_mutex;
    pthread_cond_t work_cond;
    pthread_cond_t working_cond;
    size_t working_cnt; // How many threads are processing at the moment
    size_t thread_cnt;  // How many threads are alive
    bool stop;
} tpool_t;

// Thread pool functions
static tpool_t *tpool_create(size_t num);
static void tpool_wait(tpool_t *pool);
static void tpool_destroy(tpool_t *pool);

static bool tpool_add_work(tpool_t *pool, thread_func_t func, void *arg);

static void *tpool_worker(void *arg);

static tpool_work_t *tpool_work_create(thread_func_t func, void *arg);
static void tpool_work_destroy(tpool_work_t *work);

static tpool_work_t *tpool_work_get(tpool_t *thread_pool);
#endif

int main(void)
{
#if NROF_THREADS > 1 // If there is only one thread, there is no point in creating a thread pool
    uint32_t *flip_number;
    tpool_t *pool = tpool_create(NROF_THREADS);

    for (uint32_t i = 0; i < NROF_PIECES; i++)
    {
        flip_number = malloc(sizeof(uint32_t));
        *flip_number = i + 1;
        tpool_add_work(pool, create_mask, flip_number);
    }
    tpool_wait(pool);
    tpool_destroy(pool);
#else
    pthread_t thread;
    uint32_t *flip_number = malloc(sizeof(uint32_t));
    *flip_number = 1;

    pthread_create(&thread, NULL, (void *)create_mask, flip_number);
    pthread_join(thread, NULL);
#endif
    for (uint32_t i = 0; i < BUFFER_SIZE; ++i)
    {
        for (uint32_t j = 0; j < BITS_IN_UINT128; ++j)
        {
            if (BIT_IS_SET(buffer[i], j))
            {
                printf("%d\n", BOARD_BIT_POSITION(i, j));
            }
        }
    }
    return (0);
}

static void create_mask(void *flip_number)
{
    uint32_t *arg_number;
    uint32_t number;
    arg_number = (uint32_t *)flip_number;
    number = *arg_number;

#if NROF_THREADS == 1 // If there is only one thread, there is no point in creating a thread pool
    free(flip_number);
    for (; number <= NROF_PIECES; ++number)
    {
#endif

        for (uint32_t num = 0; num <= NROF_PIECES; num = num + number)
        {
            int buffer_position = num / BITS_IN_UINT128;
            int element_position = num - buffer_position * BITS_IN_UINT128;
            pthread_mutex_lock(&mutex);
            BIT_IS_SET(buffer[buffer_position], element_position) ? BIT_CLEAR(buffer[buffer_position], element_position) : BIT_SET(buffer[buffer_position], element_position);
            pthread_mutex_unlock(&mutex);
        }

#if NROF_THREADS == 1
    }
#endif

}

#if NROF_THREADS > 1
// Thread pool functions
tpool_t *
tpool_create(size_t num)
{
    tpool_t *pool;
    pthread_t thread;

    if (num == 0)
        num = 2;

    pool = calloc(1, sizeof(*pool));
    pool->thread_cnt = num;

    pthread_mutex_init(&(pool->work_mutex), NULL);
    pthread_cond_init(&(pool->work_cond), NULL);
    pthread_cond_init(&(pool->working_cond), NULL);

    pool->work_first = NULL;
    pool->work_last = NULL;

    for (size_t i = 0; i < num; i++)
    {
        pthread_create(&thread, NULL, tpool_worker, pool);
        pthread_detach(thread);
    }

    return pool;
}

static void tpool_wait(tpool_t *pool)
{
    if (pool == NULL)
        return;

    pthread_mutex_lock(&(pool->work_mutex));
    while (1)
    {
        if ((!pool->stop && pool->working_cnt != 0) || (pool->stop && pool->thread_cnt != 0))
        {
            pthread_cond_wait(&(pool->working_cond), &(pool->work_mutex));
        }
        else
        {
            break;
        }
    }
    pthread_mutex_unlock(&(pool->work_mutex));
}

static void tpool_destroy(tpool_t *pool)
{
    tpool_work_t *work;
    tpool_work_t *work2;

    if (pool == NULL)
        return;

    pthread_mutex_lock(&(pool->work_mutex));
    work = pool->work_first;
    while (work != NULL)
    {
        work2 = work->next;
        tpool_work_destroy(work);
        work = work2;
    }
    pool->stop = true;
    pthread_cond_broadcast(&(pool->work_cond));
    pthread_mutex_unlock(&(pool->work_mutex));

    tpool_wait(pool);

    pthread_mutex_destroy(&(pool->work_mutex));
    pthread_cond_destroy(&(pool->work_cond));
    pthread_cond_destroy(&(pool->working_cond));

    free(pool);
}

static void *tpool_worker(void *arg)
{
    tpool_t *pool = arg;
    tpool_work_t *work;

    while (1)
    {
        pthread_mutex_lock(&(pool->work_mutex));
        if (pool->stop)
            break;

        if (pool->work_first == NULL)
            pthread_cond_wait(&(pool->work_cond), &(pool->work_mutex));

        work = tpool_work_get(pool);
        pool->working_cnt++;
        pthread_mutex_unlock(&(pool->work_mutex));

        if (work != NULL)
        {
            work->func(work->arg);
            tpool_work_destroy(work);
        }

        pthread_mutex_lock(&(pool->work_mutex));
        pool->working_cnt--;
        if (!pool->stop && pool->working_cnt == 0 && pool->work_first == NULL)
            pthread_cond_signal(&(pool->working_cond));
        pthread_mutex_unlock(&(pool->work_mutex));
    }

    pool->thread_cnt--;
    pthread_cond_signal(&(pool->working_cond));
    pthread_mutex_unlock(&(pool->work_mutex));
    return NULL;
}

static bool tpool_add_work(tpool_t *pool, thread_func_t func, void *arg)
{
    tpool_work_t *work;

    if (pool == NULL)
        return false;

    work = tpool_work_create(func, arg);
    if (work == NULL)
        return false;

    pthread_mutex_lock(&(pool->work_mutex));
    if (pool->work_first == NULL)
    {
        pool->work_first = work;
        pool->work_last = pool->work_first;
    }
    else
    {
        pool->work_last->next = work;
        pool->work_last = pool->work_last->next;
    }

    pthread_cond_broadcast(&(pool->work_cond));
    pthread_mutex_unlock(&(pool->work_mutex));

    return true;
}

/**
 * @brief This function gets a work from the top of the queue (linked list)
 * 
 * @param thread_pool Thread pool
 * @return tpool_work_t* 
 */
static tpool_work_t *tpool_work_get(tpool_t *thread_pool)
{
    tpool_work_t *work;

    if (thread_pool == NULL)
        return NULL;

    work = thread_pool->work_first;
    if (work == NULL)
        return NULL;
    if (work->next == NULL) // If the pulled work is the last in the list
    {
        thread_pool->work_first = NULL;
        thread_pool->work_last = NULL;
    }
    else
    {
        thread_pool->work_first = thread_pool->work_first->next;
    }

    return work;
}

/**
 * @brief This function fills a work object with parameters ot pass to the thread
 * 
 * @param func Function to start a thread with
 * @param arg Arguments to pass to the function
 * @return tpool_work_t* 
 */
static tpool_work_t *tpool_work_create(thread_func_t func, void *arg)
{
    tpool_work_t *work;

    if (func == NULL)
        return NULL;

    work = malloc(sizeof(*work));
    work->func = func;
    work->arg = arg;
    work->next = NULL;
    return work;
}

/**
 * @brief This function destroys a work object and frees the memory
 * 
 * @param work Work object to destroy
 */
static void tpool_work_destroy(tpool_work_t *work)
{
    if (work == NULL)
        return;
    free(work);
}
#endif
