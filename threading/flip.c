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
#include "flip.h"

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
                printf("%d", BOARD_BIT_POSITION(i, j));
                printf("\n");
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

    for (uint16_t i = 0; i < BUFFER_SIZE; i++)
    {
        for (uint32_t j = 0; j < 128; ++j)
        {
            if (BOARD_BIT_POSITION(i, j) > NROF_PIECES)
            {
                break;
            }
            if (BOARD_BIT_POSITION(i, j) % number == 0)
            {
                pthread_mutex_lock(&mutex);
                BIT_IS_SET(buffer[i], j) ? BIT_CLEAR(buffer[i], j) : BIT_SET(buffer[i], j);
                pthread_mutex_unlock(&mutex);
            }
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
