/* 
 * Operating Systems {2INCO} Practical Assignment
 * Interprocess Communication
 *
 * Ivan Turasov
 * Roy Meijer
 *
 * Grading:
 * Students who hand in clean code that fully satisfies the minimum requirements will get an 8. 
 * Extra steps can lead to higher marks because we want students to take the initiative. 
 * Extra steps can be, for example, in the form of measurements added to your code, a formal 
 * analysis of deadlock freeness etc.
 */

#include <time.h>    // for time()
#include <complex.h> //
#include "md5s.h"    //

#include "common.h"

/* defines */
#define ALPHABET_LENGTH (ALPHABET_END_CHAR - ALPHABET_START_CHAR)

/* local functions*/

/// sleep random amount of time
static void rsleep(int t);

/// power integer
int16_t ipow(int16_t base, int16_t exp);

/// generate strings, hash values and compare hash values
bool search_hash(uint128_t hash_inp, char found_string[], bool stop);


int main(int argc, char *argv[]) {
    // TODO:
    // (see message_queue_test() in interprocess_basic.c)
    //  * open the two message queues (whose names are provided in the arguments)
    //  * repeatingly:s
    //      - read from a message queue the new job to do
    //      - wait a random amount of time (e.g. rsleep(10000);)
    //      - do that job
    //      - write the results to a message queue
    //    until there are no more tasks to do
    //  * close the message queues

    mqd_t mq_fd_request;
    mqd_t mq_fd_response;
    MQ_REQUEST_MESSAGE req;
    MQ_RESPONSE_MESSAGE rsp;

    char *gen_string; /// <-- define size?

    /// opens queue's, make read/write only. with argv you can name the queues in terminal
    mq_fd_request = mq_open(argv[1], O_RDONLY);
    mq_fd_response = mq_open(argv[2], O_WRONLY);

    /// kept the printfs for testing purposes, will remove in the final version
    // read the message queue and store it in the request message
    printf("child: receiving...\n");

    ///store request queue
    mq_receive(mq_fd_request, (char *)&req, sizeof(req), NULL);

    printf("child: received: %d, %d, '%c', '%d'\n", req.hash, req.hash_sequence_num, req.assigned_letter, req.stop);

    if (!(req.stop)) {

        /// sleep for 1 second at maximum
        rsleep(1000);

        /// store first letter
        gen_string[0] = req.assigned_letter;

        /// search for the md5 hash value
        if (search_hash(req.hash, gen_string, req.stop)) {
            /// print generated string, put generated string and true in response message
            printf("%s\n", gen_string);
            rsp.hash_sequence_num = gen_string;
            rsp.is_found = true;
        } else {
            ///put hash number, first letter and false in response message
            sprintf(rsp.hash_sequence_num, "ha%d_%c", req.hash_sequence_num, req.assigned_letter);
            rsp.is_found = false;
        }

        // send the response
        printf("child: sending...\n");
        mq_send(mq_fd_response, (char *)&rsp, sizeof(rsp), 0);
    }
    mq_close(mq_fd_response);
    mq_close(mq_fd_request);

    return (0);
}

/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time
 * between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
static void rsleep(int t) {
    static bool first_call = true;

    if (first_call == true) {
        srandom(time(NULL) % getpid());
        first_call = false;
    }
    usleep(random() % t);
}

/*
 * int16_t ipow(int16_t base, int16_t exp)
 *
 * Powers an int
 * This is more efficient than pow because it uses ints, no floats
 * Returns base^exp
 */
int16_t ipow(int16_t base, int16_t exp) {
    int16_t result = 1;
    while (1) {
        if (exp & 1) {
            result *= base;
        }
        exp >>= 1;

        if (!exp) {
            break;
        }
        base *= base;
    }
    return result;
}

/*
 * search_hash(uint128_t hash_inp, char found_string[])
 *
 * Generates strings of 1, 2, 3, ..., and MAX_MESSAGE_LENGTH characters
 * For each length, it has to generate current_length^ALPHABET_LENGTH characters (calculations from the lecture)
 * 
 */
bool search_hash(uint128_t hash_inp, char found_string[], bool stop) {
    int counter, current_length, array_pos, number;
    uint128_t found_hash;

    // generate 1 character, 2 character, 3 character, ..., MAX_MESSAGE_LENGTH character strings of letters
    for (current_length = 1; (current_length < MAX_MESSAGE_LENGTH) && !stop; current_length++) {
        // there are alphabet_lenght different letters, so ALPHABET_LENGTH^current_length different strings have to be generated
        for (counter = 0; (counter <= ipow(ALPHABET_LENGTH, current_length) && !stop); counter++) {
            // store the generated number
            number = counter;

            /// it has to start with only the assigned letter
            if (number == 0) {
                found_string[2] = '\0';
                /// then it looks for all other possible combinations
            } else {
                /// array_pos determanes the number of letters in the string and if number becomes 0, every letter is handeled
                /// array_pos starts with 1 because found_string[0] is defined.
                for (array_pos = 1; (number > 0) && !stop; array_pos++) {
                    found_string[array_pos] = number % ALPHABET_LENGTH + 'a';
                    found_string[array_pos + 1] = '\0';
                    number /= ALPHABET_LENGTH;
                }
            }
            //generates hash value based on the generated string
            found_hash = md5s(found_string, MAX_MESSAGE_LENGTH);
            //returns true if this is equal to the input hash value
            if (found_hash == hash_inp) {
                return true;
            }
        }
    }
    return false;
}
