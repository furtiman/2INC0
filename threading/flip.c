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

int main(void) {
    // TODO: start threads to flip the pieces and output the results
    // (see thread_test() and thread_mutex_test() how to use threads and mutexes,
    //  see bit_test() how to manipulate bits in a large integer)
    
    printf("\nThe number of pieces is %d.", NROF_PIECES);

    uint16_t number = 0;
    char continu = 'n';

    while(1) { // for(number = 1; number < NROF_PIECES; number++) { /* for-loop can be used to do flip the pieces automaticly */

        printf("\nWhich pieces have to be flipped?\nEnter a number: ");
        scanf("%d", &number);
        printf("\n");

        flip_pieces(number);

        uint64_t low[(NROF_PIECES/128) + 1], high[(NROF_PIECES/128) + 1];

        for (int16_t i = 0; i <= NROF_PIECES/128; i++) {
            low[i] = LO(buffer[i]);
            high[i] = HI(buffer[i]);

            /* prints the first 64 bits */
            for (int16_t j = 0; j <= 63; j++) {
                if (low[i] & 1) {
                    printf("%d", 1);
                } else {
                    printf("%d", 0);
                }
                low[i] >>= 1;
            }

            /* prints the last 64 bits */
            for (int16_t j = 0; j <= 63; j++) {
                if (high[i] & 1) {
                    printf("%d", 1);
                } else {
                    printf("%d", 0);
                }
                high[i] >>= 1;
            }
            printf("\n");
        }
        
        printf("\n");
        
        printf("Continue? [Y/N]\n");
        scanf("%s", &continu);
        if (continu == 'n' || continu == 'N')
            exit(0);
    }
    return (0);
}

void create_mask(uint128_t mask[], uint16_t flip_number) {
    const uint16_t buffer_size = NROF_PIECES/128 + 1;

    /* inintialize mask array*/
    for (uint16_t i = 0; i < buffer_size; i++) {
        mask[i] = 0;
    }

    for (uint16_t i = NROF_PIECES; i > 0; i--) {
        /* i counts backwards, piece_number counts upwards */
        uint16_t piece_number = i - 1;
        /* becomes 1 if piece_number i-1 should be flipperd, 0 if piece_number i-1 should not be flipped */
        uint16_t one_or_zero = (!(piece_number % flip_number)) && piece_number ? 1 : 0; 

         for (uint16_t j = buffer_size - 1; j > 0; j--) {
             /* shifts all bits left by 1 and puts the msb of arr[i - 1] in the lsb of arr[i] */
             mask[j] = (mask[j] << 1) | ((mask[j - 1] & ((uint128_t)1 << 127)) >> 127);
         }
        /* shifts all bits left by 1 and puts a 1 or 0 at the lsb of the mask */
        mask[0] = (mask[0] << 1) | one_or_zero;
    }
}

void flip_pieces(uint128_t flip_number) {
     uint128_t mask[(NROF_PIECES/128) + 1];
     create_mask(mask, flip_number);
     for (uint16_t i = 0; i <= NROF_PIECES/128; i++) {
         /* buffer XOR mask works like flipping the pieces */
         buffer[i] ^= mask[i];
     }
}
