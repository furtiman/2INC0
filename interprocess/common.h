/* 
 * Operating Systems {2INCO} Practical Assignment
 * Interprocess Communication
 *
 * Contains definitions which are commonly used by the farmer and the workers
 *
 */

#ifndef COMMON_H
#define COMMON_H

#include "settings.h"

// maximum size for any message in the tests
#define MAX_MESSAGE_LENGTH	6
 
// A data structure with 3 members, represents a request to a worker
typedef struct
{
    uint128_t hash;
    uint16_t hash_sequence_num;
    char assigned_letter;
} MQ_REQUEST_MESSAGE;

// A data structure with 3 members, represents a response from a worker
typedef struct
{
    uint16_t hash_sequence_num;
    bool is_found;
} MQ_RESPONSE_MESSAGE;

#endif

