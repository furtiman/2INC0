/* 
 * Operating Systems {2INCO} Practical Assignment
 * Interprocess Communication
 * 
 * Ivan Turasov (0988297)
 * Roy Meijer (1522329)
 * 
 * Contains definitions which are commonly used by the farmer and the workers
 */

#ifndef COMMON_H
#define COMMON_H

// Common includes for farmer and worker
#include <errno.h>
#include <unistd.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "settings.h"

// Maximum size for any message in the tests
#define MAX_MESSAGE_LENGTH 6

#define STUDENT_NAME_1 "Ivan Turasov"
#define STUDENT_NAME_2 "Roy Meijer"

// A data structure with 4 members, represents a request to a worker
typedef struct
{
    uint128_t hash;
    uint16_t hash_sequence_num;
    char assigned_letter;
    bool stop;
} MQ_REQUEST_MESSAGE;

// A data structure with 3 members, represents a response from a worker
typedef struct
{
    uint16_t hash_sequence_num;
    bool is_found;
    char response[MAX_MESSAGE_LENGTH + 1];
} MQ_RESPONSE_MESSAGE;

bool md5_list_marker[MD5_LIST_NROF];

/**
 * @brief Sleep a random amount of time between 0 and t microseconds
 * 
 * @param t Higher threshold for generating a random pause in ms
 */
extern void rsleep(int t);

#endif
