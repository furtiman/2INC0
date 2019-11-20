/* 
 * Operating Systems {2INCO} Practical Assignment
 * Interprocess Communication
 *
 * STUDENT_NAME_1 (STUDENT_NR_1)
 * STUDENT_NAME_2 (STUDENT_NR_2)
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

static void getattr(mqd_t mq_fd);

int main (int argc, char * argv[])
{
    if (argc != 1)
    {
        fprintf (stderr, "%s: invalid arguments\n", argv[0]);
    }


    pid_t processID; /* Process ID from fork() */
    mqd_t mq_fd_request;
    mqd_t mq_fd_response;
    MQ_REQUEST_MESSAGE req;
    MQ_RESPONSE_MESSAGE rsp;
    struct mq_attr attr;

    sprintf(mq_name1, "/mq_request_%s_%d", STUDENT_NAME_1, getpid());
    sprintf(mq_name2, "/mq_response_%s_%d", STUDENT_NAME_2, getpid());

    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(MQ_REQUEST_MESSAGE);
    mq_fd_request = mq_open(mq_name1, O_WRONLY | O_CREAT | O_EXCL, 0600, &attr);

    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(MQ_RESPONSE_MESSAGE);
    mq_fd_response = mq_open(mq_name2, O_RDONLY | O_CREAT | O_EXCL, 0600, &attr);

    getattr(mq_fd_request);
    getattr(mq_fd_response);

    processID = fork();
    if (processID < 0)
    {
        perror("fork() failed");
        exit(1);
    }
    else
    {
        if (processID == 0)
        {
            printf("Forked successfully");
        }
    }
   
    // TODO:
    //  * create the message queues (see message_queue_test() in interprocess_basic.c)
    //  * create the child processes (see process_test() and message_queue_test())
    //  * do the farming
    //  * wait until the chilren have been stopped (see process_test())
    //  * clean up the message queues (see message_queue_test())

    // Important notice: make sure that the names of the message queues contain your
    // student name and the process id (to ensure uniqueness during testing)
    }

static void message_queue_test(void)
{
    pid_t processID; /* Process ID from fork() */
    mqd_t mq_fd_request;
    mqd_t mq_fd_response;
    MQ_REQUEST_MESSAGE req;
    MQ_RESPONSE_MESSAGE rsp;
    struct mq_attr attr;

    sprintf(mq_name1, "/mq_request_%s_%d", STUDENT_NAME_1, getpid());
    sprintf(mq_name2, "/mq_response_%s_%d", STUDENT_NAME_2, getpid());

    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(MQ_REQUEST_MESSAGE);
    mq_fd_request = mq_open(mq_name1, O_WRONLY | O_CREAT | O_EXCL, 0600, &attr);

    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(MQ_RESPONSE_MESSAGE);
    mq_fd_response = mq_open(mq_name2, O_RDONLY | O_CREAT | O_EXCL, 0600, &attr);

    getattr(mq_fd_request);
    getattr(mq_fd_response);

    processID = fork();
    if (processID < 0)
    {
        perror("fork() failed");
        exit(1);
    }
    else
    {
        if (processID == 0)
        {
            // child-stuff
            message_queue_child();
            exit(0);
        }
        else
        {
            // remaining of the parent stuff

            // fill request message
            req.a = 73;
            req.b = 42;
            req.c = 'z';

            sleep(3);
            // send the request
            printf("parent: sending...\n");
            mq_send(mq_fd_request, (char *)&req, sizeof(req), 0);

            sleep(3);
            // read the result and store it in the response message
            printf("parent: receiving...\n");
            mq_receive(mq_fd_response, (char *)&rsp, sizeof(rsp), NULL);

            printf("parent: received: %d, '", rsp.e);
            // printing characters of f[] separately:
            for (int i = 0; i < rsp.e; i++)
            {
                printf("%c", rsp.f[i]);
            }
            // printing g[] in one step (because it has the \0-terminator):
            printf("', '%s'\n", rsp.g);

            sleep(1);

            waitpid(processID, NULL, 0); // wait for the child

            mq_close(mq_fd_response);
            mq_close(mq_fd_request);
            mq_unlink(mq_name1);
            mq_unlink(mq_name2);
        }
    }

    return (0);
}

static void getattr(mqd_t mq_fd)
{
    struct mq_attr attr;
    int rtnval;

    rtnval = mq_getattr(mq_fd, &attr);
    if (rtnval == -1)
    {
        perror("mq_getattr() failed");
        exit(1);
    }
    fprintf(stderr, "%d: mqdes=%d max=%ld size=%ld nrof=%ld\n",
            getpid(),
            mq_fd, attr.mq_maxmsg, attr.mq_msgsize, attr.mq_curmsgs);
}
