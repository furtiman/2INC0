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

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "common.h"

static char mq_name1[80];
static char mq_name2[80];

int sent = 0;     // Incremented when a message is sent
int received = 0; // Incremented when a message is received
int msg = 0;      // Used to store the current amount of messages in a mq
int found_hashes = 0;

static int get_mq_attr_nrof_messages(mqd_t mq_fd);

int main(int argc, char *argv[])
{
    if (argc != 1)
    {
        fprintf(stderr, "%s: invalid arguments\n", argv[0]);
    }

    static mqd_t mq_fd_request;
    static mqd_t mq_fd_response;
    static MQ_REQUEST_MESSAGE req;
    static MQ_RESPONSE_MESSAGE rsp;
    struct mq_attr attr;

    char responses[MD5_LIST_NROF][MAX_MESSAGE_LENGTH + 1];

    sprintf(mq_name1, "/mq_request_%s_%d", STUDENT_NAME_1, getpid());
    sprintf(mq_name2, "/mq_response_%s_%d", STUDENT_NAME_2, getpid());

    attr.mq_maxmsg = MQ_MAX_MESSAGES;
    attr.mq_msgsize = sizeof(MQ_REQUEST_MESSAGE);
    mq_fd_request = mq_open(mq_name1, O_WRONLY | O_CREAT | O_EXCL, 0600, &attr);

    attr.mq_maxmsg = MQ_MAX_MESSAGES;
    attr.mq_msgsize = sizeof(MQ_RESPONSE_MESSAGE);
    mq_fd_response = mq_open(mq_name2, O_RDONLY | O_CREAT | O_EXCL, 0600, &attr);

    get_mq_attr_nrof_messages(mq_fd_request);
    get_mq_attr_nrof_messages(mq_fd_response);

    pid_t processes[NROF_WORKERS];

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
    // Initialise shared semaphore
    sem_init(&sh_mem->semaphore, 1, 1);

    // Fill the markers buffer
    for (int i = 0; i < MD5_LIST_NROF; ++i)
    {
        sh_mem->md5_list_markers[i] = false;
    }

    // Create workers
    for (size_t i = 0; i < NROF_WORKERS; ++i)
    {
        processes[i] = fork();
        if (processes[i] < 0)
        {
            perror("fork() of process failed");
            exit(1);
        }
        else if (processes[i] == 0)
        {
            execlp("./worker", "worker", mq_name1, mq_name2, NULL);
            exit(0);
        }
    }

    for (int hash_num = 0; hash_num < MD5_LIST_NROF; ++hash_num)
    {
        for (char letter = ALPHABET_START_CHAR; letter <= ALPHABET_END_CHAR; ++letter)
        {
            // Fill request message
            req.hash = md5_list[hash_num];
            req.hash_sequence_num = hash_num;
            req.assigned_letter = letter;
            req.stop = false;

            mq_send(mq_fd_request, (char *)&req, sizeof(req), 0);
            sent++;
            msg = get_mq_attr_nrof_messages(mq_fd_response);
            while (msg)
            {
                mq_receive(mq_fd_response, (char *)&rsp, sizeof(rsp), NULL);
                received++;

                if (rsp.is_found)
                {
                    while (sem_wait(&sh_mem->semaphore)) // Acquire the semaphore, writing to a shared resource
                        ;
                    sh_mem->md5_list_markers[rsp.hash_sequence_num] = true; // Indicate that the given hash is found
                    sem_post(&sh_mem->semaphore);                           // Release the semaphore

                    strcpy(responses[rsp.hash_sequence_num], rsp.response);
                    found_hashes++;
                }
                msg = get_mq_attr_nrof_messages(mq_fd_response);
            }
        }
    }

    // All the messages are sent, wait until all the messages are received
    msg = get_mq_attr_nrof_messages(mq_fd_response);
    while (sent != received || msg || found_hashes != MD5_LIST_NROF)
    {
        mq_receive(mq_fd_response, (char *)&rsp, sizeof(rsp), NULL);
        received++;
        if (rsp.is_found)
        {
            while (sem_wait(&sh_mem->semaphore)) // Acquire the semaphore, writing to a shared resource
                ;
            sh_mem->md5_list_markers[rsp.hash_sequence_num] = true; // Indicate that the given hash is found
            sem_post(&sh_mem->semaphore);                           // Release the semaphore

            strcpy(responses[rsp.hash_sequence_num], rsp.response);
            found_hashes++;
        }
        msg = get_mq_attr_nrof_messages(mq_fd_response);
    }

    // Send mesasges to terminate all the workers
    for (size_t i = 0; i < NROF_WORKERS; ++i)
    {
        req.stop = true;
        mq_send(mq_fd_request, (char *)&req, sizeof(req), 0);
    }

    // Wait for all the workers to terminate
    for (size_t i = 0; i < NROF_WORKERS; ++i)
    {
        waitpid(processes[i], NULL, 0);
    }

    // Print the collected strings
    for (int i = 0; i < MD5_LIST_NROF; ++i)
    {
        printf("'%s'\n", responses[i]);
    }

    mq_close(mq_fd_response);
    mq_close(mq_fd_request);
    mq_unlink(mq_name1);
    mq_unlink(mq_name2);

    // Detach from shared memory region
    shmdt(sh_mem);

    // Clean up the shared memory location
    if (shmctl(shmid, IPC_RMID, 0) == -1)
    {
        printf("Error deleting shared memory\n");
    }
    return 0;
}

/**
 * @brief Get the attributes of the message queue and return a number of messages that it contains
 * 
 * @param mq_fd Message queue
 * @return int Number of messages in the queue
 */
static int get_mq_attr_nrof_messages(mqd_t mq_fd)
{
    struct mq_attr attr;
    int rtnval;

    rtnval = mq_getattr(mq_fd, &attr);
    if (rtnval == -1)
    {
        perror("mq_getattr() failed in farmer");
        exit(1);
    }

    return attr.mq_curmsgs;
}
