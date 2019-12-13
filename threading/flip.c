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

#include "uint128.h"
#include "flip.h"


void create_mask(uint128_t mask[], uint32_t flip_number);
static void* flip_thread(void* flip_number);
static void control_threads(void);

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(void) {
    /* start threads, flip pieces, stop threads */
    control_threads();

    for (uint16_t i = 0; i <= NROF_PIECES/128; i++) {
        for (uint32_t j = 0; j < 127; j++) {
            if (buffer[i] & 1) {
                printf("%d", i*128 + j);
                printf("\n");
            }
            buffer[i] >>= 1;
        }
    }
    return (0);
}

void create_mask(uint128_t mask[], uint32_t flip_number) {
    const uint16_t buffer_size = NROF_PIECES/128 + 1;

    /* inintialize mask array*/
    for (uint16_t i = 0; i < buffer_size; i++) {
        mask[i] = 0;
    }

    for (uint32_t i = NROF_PIECES; i > 0; i--) {
        /* i counts backwards, piece_number counts upwards */
        uint32_t piece_number = i - 1;
        /* becomes 1 if piece_number i-1 should be flipperd, 0 if piece_number i-1 should not be flipped */
        bool one_or_zero = !(piece_number % flip_number) && piece_number ? 1 : 0; 

         for (uint16_t j = buffer_size - 1; j > 0; j--) {
             /* shifts all bits left by 1 and puts the msb of arr[i - 1] in the lsb of arr[i] */
             mask[j] = (mask[j] << 1) | ((mask[j - 1] & ((uint128_t)1 << 127)) >> 127);
         }
        /* shifts all bits left by 1 and puts a 1 or 0 at the lsb of the mask */

        mask[0] = (mask[0] << 1) | one_or_zero;
    }
}

static void* flip_thread(void* flip_number) {
    uint32_t *arg_number;
    uint32_t number;

    arg_number = (uint32_t*)flip_number;
    number = *arg_number;
    free(flip_number);

    uint128_t mask[(NROF_PIECES/128) + 1];
    create_mask(mask, number);

    //printf("\nlocked\n");

    pthread_mutex_lock(&mutex);
    for (uint16_t i = 0; i <= NROF_PIECES/128; i++) {
        /* buffer XOR mask works like flipping the pieces */
        buffer[i] ^= mask[i];
    }
    pthread_mutex_unlock(&mutex);

    //printf("\nunlock\n");
    return (NULL);
}

static void control_threads(void) {
    uint32_t* flip_number;
    /* array of threads */
    pthread_t flippers[NROF_THREADS];

    for (uint32_t i = 1; i <= NROF_PIECES; i++) {
        flip_number = malloc(sizeof(uint32_t));
        *flip_number = i;

        /* created NROF_THREADS threads and gives them the current value of i */
        //printf("\nThread %d flips %d\n", (int)(i % NROF_THREADS), (int)i);
        pthread_create(&flippers[i % NROF_THREADS], NULL, flip_thread, flip_number);

        /* join all threads after NROF_THREADS - 1 is created */
        if (!(i % NROF_THREADS)) {
            for (uint16_t j = NROF_THREADS - 1; j != 0; j--) {
                /* no return value needed */
                pthread_join(flippers[j], NULL);
            }
        }
    }
    //printf("\ndone\n");
}
