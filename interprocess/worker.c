/* 
 * Operating Systems {2INCO} Practical Assignment
 * Interprocess Communication
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

#include <time.h>
#include <complex.h>
#include "md5s.h"

#include "common.h"

static bool search_hash(uint128_t hash_inp, char assigned_letter, char *result);

static bool combination_util_entry(uint128_t hash, char arr[], int n, int r,
                                   char assigned_letter, char *data);

static bool combination_util_recursive(uint128_t hash, char arr[], char data[], int end,
                                       int index, int r);

static int get_mq_attr_nrof_messages(mqd_t mq_fd);

int main(int argc, char *argv[])
{
    static mqd_t mq_fd_request;
    static mqd_t mq_fd_response;
    static MQ_REQUEST_MESSAGE req;
    static MQ_RESPONSE_MESSAGE rsp;

    // Open queues, make read/write only. With argv you can name the queues in terminal
    mq_fd_request = mq_open(argv[1], O_RDONLY);
    mq_fd_response = mq_open(argv[2], O_WRONLY);

    // Generate a System V IPC key n a file common to worker and farmer
    key_t markers_key = ftok("common.h", 65);
    // Acquire a shared memory region
    int shmid = shmget(markers_key, sizeof(shared_memory_t), 0666 | IPC_CREAT);
    if (shmid < 0)
    {
        perror("farmer smget returned -1\n");
        exit(-1);
    }
    // Attach to a shared memory region
    shared_memory_t *sh_mem = (shared_memory_t *)shmat(shmid, NULL, 0);

    while (1)
    {
        rsleep(1000);
        mq_receive(mq_fd_request, (char *)&req, sizeof(req), NULL);

        rsp.hash_sequence_num = req.hash_sequence_num;

        if (req.stop)
        {
            exit(0);
        }
        else
        {
            while (sem_wait(&sh_mem->semaphore)) // Acquire the semaphore, reading a shared resource
                ;
            bool is_already_found = sh_mem->md5_list_markers[req.hash_sequence_num];
            sem_post(&sh_mem->semaphore); // Release the semaphore

            if (is_already_found)
            {
                rsp.is_found = false;
            }
            else
            {
                // Search for the md5 hash value
                char result[MAX_MESSAGE_LENGTH + 1];
                if (search_hash(req.hash, req.assigned_letter, result))
                {
                    rsp.is_found = true;
                    rsp.hash_sequence_num = req.hash_sequence_num;
                    strcpy(rsp.response, result);
                }
                else
                {
                    rsp.is_found = false;
                }
            }

            int msg = get_mq_attr_nrof_messages(mq_fd_response);
            if (msg >= MQ_MAX_MESSAGES)
            {
                msg = get_mq_attr_nrof_messages(mq_fd_response);
                rsleep(1000); // Sleep for at most 1 second
            }

            mq_send(mq_fd_response, (char *)&rsp, sizeof(rsp), 0);
        }
    }
    mq_close(mq_fd_request);
    mq_close(mq_fd_response);

    // Detach from shared memory region
    shmdt(sh_mem);
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

/**
 * @brief Sleep a random amount of time between 0 and t microseconds
 * 
 * @param t Higher threshold for generating a random pause in ms
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

/**
 * @brief This function traverses all possible combinations of strings starting from 'assigned_letter'
 *        to check if they match the provided hash
 * 
 * @param hash_inp Hash value to check combinations against
 * @param assigned_letter Assigned letter to check the strings
 * @param result Buffer to copy the found hash to
 * @return true Hash found with a given letter
 * @return false Hash not found with a given letter
 */
static bool search_hash(uint128_t hash_inp, char assigned_letter, char *result)
{
    char alph[ALPHABET_NROF_CHAR];
    // Fill an array with the given alphabet
    for (int i = 0; i < ALPHABET_NROF_CHAR; ++i)
    {
        alph[i] = ALPHABET_START_CHAR + i;
    }
    for (int current_length = 1; current_length <= MAX_MESSAGE_LENGTH; current_length++)
    {
        char data[current_length + 1];
        bool found = combination_util_entry(hash_inp, alph, ALPHABET_NROF_CHAR,
                                            current_length, assigned_letter, data);
        if (found)
        {
            strcpy(result, data);
            return true;
        }
    }
    return false;
}

/**
 * @brief 
 * 
 * @param hash 
 * @param arr Alphabet
 * @param n Alphabet Length
 * @param r Current string length
 * @param assigned_letter 
 * @param data Array with resulting string
 * @return true 
 * @return false 
 */
static bool combination_util_entry(uint128_t hash, char arr[], int n, int r,
                                   char assigned_letter, char *data)
{
    data[0] = assigned_letter;
    data[1] = '\0';

    return combination_util_recursive(hash, arr, data, n - 1, 1, r); // n - 1 -- alphabet
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
static bool combination_util_recursive(uint128_t hash, char arr[], char data[], int arr_size,
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
        if (combination_util_recursive(hash, arr, data, arr_size, index + 1, output_size))
        {
            return true;
        }
    }
    return false;
}
