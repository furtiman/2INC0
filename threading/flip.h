/* 
 * Operating Systems {2INC0} Practical Assignment
 * Threaded application
 *
 * Joris Geurts
 * 
 */

#ifndef _FLIP_H_
#define _FLIP_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h> // for perror()
#include <pthread.h>

#include "uint128.h"

/**
 * NROF_PIECES: size of the board; number of pieces to be flipped
 */
#define NROF_PIECES 300000

/**
 * NROF_THREADS: number of threads that can be run in parallel
 * (value must be between 1 and ... (until you run out of system resources))
 */
#define NROF_THREADS 100

#define BUFFER_SIZE ((NROF_PIECES - 1) / 128) + 1

#define BITS_IN_UINT128 128

#define BOARD_BIT_POSITION(buffer_cell, position_in_cell) (buffer_cell * BITS_IN_UINT128 + position_in_cell)

/**
 * buffer[]: datastructure of the pieces; each piece is represented by one bit
 */
static uint128_t buffer[BUFFER_SIZE];

// Create a bitmask where bit at position n is set
#define BITMASK(n) (((uint128_t)1) << (n))
// Check if bit n in v is set
#define BIT_IS_SET(v, n) (((v)&BITMASK(n)) == BITMASK(n))
// Set bit n in v
#define BIT_SET(v, n) ((v) = (v) | BITMASK(n))
// Clear bit n in v
#define BIT_CLEAR(v, n) ((v) = (v) & ~BITMASK(n))

#endif