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

#include <time.h>
#include <complex.h>
#include "md5s.h"

#include "common.h"

bool printCombination(uint128_t hash, char arr[], int n, int r, char assigned_letter, char *data);

bool combinationUtil(uint128_t hash, char arr[], char data[], int end,
                     int index, int r);

void printAllKLengthRec(char *prefix,
                        int k);

static int get_mq_attr_nrof_messages(mqd_t mq_fd);

// generate strings, hash values and compare hash values  char *found_string,
bool search_hash(uint128_t hash_inp, bool stop, char assigned_letter, char *result);

int main(int argc, char *argv[])
{
    static mqd_t mq_fd_request;
    static mqd_t mq_fd_response;
    static MQ_REQUEST_MESSAGE req;
    static MQ_RESPONSE_MESSAGE rsp;

    // Open queues, make read/write only. With argv you can name the queues in terminal
    mq_fd_request = mq_open(argv[1], O_RDONLY);
    mq_fd_response = mq_open(argv[2], O_WRONLY);

    while (1)
    {
        rsleep(1000);
        // printf("Worker Receiving message!\n");
        mq_receive(mq_fd_request, (char *)&req, sizeof(req), NULL);

        rsp.hash_sequence_num = req.hash_sequence_num;

        if (req.stop)
        {
            // printf("Received kill!\n");
            rsleep(10000);
            exit(0);
        }
        else if (!(md5_list_marker[req.hash_sequence_num]))
        {
            // printf("Worker received letter %c, hash n %d\r\n", req.assigned_letter, req.hash_sequence_num);
            // Sleep for 1 second at maximum
            rsleep(1000);
            // Search for the md5 hash value
            char result[MAX_MESSAGE_LENGTH];
            if (search_hash(req.hash, false, req.assigned_letter, result))
            {
                rsp.is_found = true;
                rsp.hash_sequence_num = req.hash_sequence_num;
                strcpy(rsp.response, result);
            }
            else
            {
                rsp.is_found = false;
            }

            int msg = get_mq_attr_nrof_messages(mq_fd_response);
            if (msg >= MQ_MAX_MESSAGES)// while (msg >= MQ_MAX_MESSAGES)
            {
                msg = get_mq_attr_nrof_messages(mq_fd_response);
                rsleep(1000);
            }

            mq_send(mq_fd_response, (char *)&rsp, sizeof(rsp), 0);
            // printf("Worker sent response with letter %c, hash n %d\r\n", req.assigned_letter, req.hash_sequence_num);
        }
    }

    mq_close(mq_fd_request);
    mq_close(mq_fd_response);
    return (0);
}

static int get_mq_attr_nrof_messages(mqd_t mq_fd)
{
    struct mq_attr attr;
    int rtnval;

    rtnval = mq_getattr(mq_fd, &attr);
    if (rtnval == -1)
    {
        perror("mq_getattr() failed in worker");
        exit(1);
    }

    return attr.mq_curmsgs;
}

/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time
 * between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
extern void rsleep(int t)
{
    static bool first_call = true;

    if (first_call == true)
    {
        srandom(time(NULL) % getpid());
        first_call = false;
    }
    usleep(random() % t);
}

/*
 * search_hash(uint128_t hash_inp, char found_string[])
 *
 * Generates strings of 1, 2, 3, ..., and MAX_MESSAGE_LENGTH characters
 * For each length, it has to generate current_length^ALPHABET_NROF_CHAR characters (calculations from the lecture)
 *  char *found_string,
 */
bool search_hash(uint128_t hash_inp, bool stop, char assigned_letter, char *result)
{
    char alph[ALPHABET_NROF_CHAR];
    for (int i = 0; i < ALPHABET_NROF_CHAR; ++i)
    {
        alph[i] = ALPHABET_START_CHAR + i;
    }
    for (int current_length = 1; (current_length <= MAX_MESSAGE_LENGTH) && !stop; current_length++)
    {
        char data[current_length + 1];
        bool found = printCombination(hash_inp, alph, ALPHABET_NROF_CHAR, current_length, assigned_letter, data);
        if (found)
        {
            strcpy(result, data);
            return true;
        }
    }
    return false;
}

// The main function that prints all combinations of size r
// in arr[] of size n. This function mainly uses combinationUtil()
bool printCombination(uint128_t hash, char arr[], int n, int r, char assigned_letter, char *data)
{
    data[0] = assigned_letter;
    data[1] = '\0';

    return combinationUtil(hash, arr, data, n - 1, 1, r);
}

/**
 * @brief 
 * 
 * @param arr Input alphabet
 * @param data Temporary array
 * @param arr_size Size of the alphabet
 * @param index Current element being replaced
 * @param output_size Length of the strings to generate
 */
bool combinationUtil(uint128_t hash, char arr[], char data[], int arr_size,
                     int index, int output_size)
{
    uint128_t found_hash;
    // Current combination is ready to be printed, print it
    if (index == output_size)
    {
        found_hash = md5s(data, output_size);
        return (found_hash == hash);
    }

    for (int i = 0; i <= arr_size; i++)
    {
        data[index] = arr[i];
        data[index + 1] = '\0';
        if (combinationUtil(hash, arr, data, arr_size, index + 1, output_size))
        {
            return true;
        }
    }
    return false;
}
