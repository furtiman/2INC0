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

#include "flip.h"

#define BUFFER_SIZE ((NROF_PIECES - 1) / 128) + 1

#define BITS_IN_UINT128 128

#define BOARD_BIT_POSITION(buffer_cell, position_in_cell) (buffer_cell * BITS_IN_UINT128 + position_in_cell)

// Create a bitmask where bit at position n is set
#define BITMASK(n) (((uint128_t)1) << (n))
// Check if bit n in v is set
#define BIT_IS_SET(v, n) (((v)&BITMASK(n)) == BITMASK(n))
// Set bit n in v
#define BIT_SET(v, n) ((v) = (v) | BITMASK(n))
// Clear bit n in v
#define BIT_CLEAR(v, n) ((v) = (v) & ~BITMASK(n))

static void *create_mask(void *flip_number);
static void control_threads(void);

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(void)
{
    control_threads();

    for (uint16_t i = 0; i < BUFFER_SIZE; ++i)
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

static void *create_mask(void *flip_number)
{
    uint32_t *arg_number;
    uint32_t number;

    arg_number = (uint32_t *)flip_number;
    number = *arg_number;
    free(flip_number);

    for (uint32_t num = 0; num <= NROF_PIECES; num = num + number)
    {
        int buffer_position = num / BITS_IN_UINT128;
        int element_position = num - buffer_position * BITS_IN_UINT128;
        {
            pthread_mutex_lock(&mutex);
            BIT_IS_SET(buffer[buffer_position], element_position) ? BIT_CLEAR(buffer[buffer_position], element_position) : BIT_SET(buffer[buffer_position], element_position);
            pthread_mutex_unlock(&mutex);
        }
    }
    return NULL;
}

static void control_threads(void)
{
    static uint32_t *flip_number;
    /* array of threads */
    static pthread_t flippers[NROF_THREADS];
    static int active_threads = 0;
    for (uint32_t i = 1; i <= NROF_PIECES; i++)
    {
        flip_number = malloc(sizeof(uint32_t));
        *flip_number = i;

        /* created NROF_THREADS threads and gives them the current value of i */
        pthread_create(&flippers[i % NROF_THREADS], NULL, create_mask, flip_number);
        active_threads++;

        /* join all threads after NROF_THREADS - 1 is created */
        if ((active_threads % NROF_THREADS) == 0 || i == NROF_PIECES)
        {
            for (uint16_t j = 0; j < active_threads; ++j)
            {
                /* no return value needed */
                pthread_join(flippers[j], NULL);
                active_threads--;
            }
        }
    }
}
