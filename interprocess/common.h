/* 
 * Operating Systems {2INCO} Practical Assignment
 * Interprocess Communication
 *
 * Contains definitions which are commonly used by the farmer and the workers
 *
 */

#ifndef COMMON_H
#define COMMON_H

// Common includes for farmer and worker
#include <errno.h>
#include <unistd.h> // for ex
#include <mqueue.h> // for mq
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "settings.h"

// Maximum size for any message in the tests
#define MAX_MESSAGE_LENGTH 6

#define STUDENT_NAME_1 "Ivan Turasov"
#define STUDENT_NAME_2 "Roy Meijer"

// A data structure with 3 members, represents a request to a worker
typedef struct
{
    uint128_t hash;
    uint16_t hash_sequence_num;
    char assigned_letter;
    bool stop; /// Wadded this to stop a worker if the hash value has already been found by another worker. not sure if needed
} MQ_REQUEST_MESSAGE;

// A data structure with 2 members, represents a response from a worker
typedef struct
{
    uint16_t hash_sequence_num;
    bool is_found;
} MQ_RESPONSE_MESSAGE;

static bool md5_list_marker[MD5_LIST_NROF] = {false};
#endif
