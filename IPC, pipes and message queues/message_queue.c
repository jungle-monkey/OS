#include "message_queue.h"
#include <fcntl.h>           /* For O_* constants ->file control options */
#include <sys/stat.h>        /* For mode constants */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

/*
 * The commands supported by the server
 */
typedef enum _Command {
    CmdAdd = 0x00,     // Adds the two message parameters
    CmdSubtract,       // Subtracts the two message parameters
    CmdExit            // Stops the server
} Command;

/*
 * The message format to be sent to the server.
 */
typedef struct _Message {
    /*
     * One of the command constants.
     */
    Command command;
    /*
     * Used as operand 1 (if required)
     */
    int parameter1;
    /*
     * Used as operand 2 (if required)
     */
    int parameter2;
} Message;

#define QUEUE_NAME "/simple_calculator"
#define FORMAT_STRING_ADD      "Calc: %d + %d = %d\n"
#define FORMAT_STRING_SUBTRACT "Calc: %d - %d = %d\n"

mqd_t startClient(void)
{
    // Open the message queue previously created by the server
    mqd_t q_client = mq_open(QUEUE_NAME, O_WRONLY);
    return q_client;
}

int sendExitTask(mqd_t client) // client ~ filedes
{
    // Send the exit command to the server.
    Message msg;
    msg.command = CmdExit;
        //int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio);
    int sending_result = mq_send(client, (char*) &msg, sizeof(msg), 0); //?
    return sending_result;
}

int sendAddTask(mqd_t client, int operand1, int operand2)
{
    // Send the add command with the operands
    Message msg;
    msg.command = CmdAdd;
    msg.parameter1 = operand1;
    msg.parameter2 = operand2;
    int adding_result = mq_send(client, (char*) &msg, sizeof(msg), 0); 
    return adding_result;
}

int sendSubtractTask(mqd_t client, int operand1, int operand2)
{
    // Send the sub command with the operands
    Message msg;
    msg.command = CmdSubtract;
    msg.parameter1 = operand1;
    msg.parameter2 = operand2;
    int subt_result = mq_send(client, (char*) &msg, sizeof(msg), 0); 
    return subt_result;
}

int stopClient(mqd_t client)
{
    (void)client;
    // Clean up anything on the client-side
    int close_result = mq_close(client);
    return close_result;
}

int runServer(void)
{
    int didExit = 0, hadError = 0; // flags
    Message msg;
    // Flags for options when creating the queue
    int mess_q_flags = O_CREAT | O_RDONLY; //O_RDWR; // 
    //printf("O_CREATE: %d \n", O_CREAT); // output: O_CREATE: 64 -> 00.... 0100 0000
    //printf("O_RDWR: %d \n", O_RDWR); // output: O_RDWR: 2 -> 00.... 0000 0010
    //printf("Message queue flags: %d \n", mess_q_flags); // -> both create + rdwr -> 00... 0100 0010
    
    // Set the mode foe the mq
    mode_t mode =  S_IRWXO | S_IRWXU | S_IRWXG;

    // A mq_attr structure will have at least the following fields: 
    struct mq_attr attr; 
    attr.mq_flags   = 0; //| O_RDONLY; // Message queue flags. Read only flag has to be in the message queue attributes
    attr.mq_maxmsg  = 10;           // Maximum number of messages in the queue
    attr.mq_msgsize = sizeof(msg);  // Maximum message size
    attr.mq_curmsgs = 0;            // Number of messages currently queued
    (void) attr;

    // Create and open the message queue. Server only needs to read from it.
    // Clients only need to write to it, allow for all users.
    mqd_t server = mq_open(QUEUE_NAME, mess_q_flags, mode, &attr); // mqd_t mq_open(const char *name, int oflag, ...); open() creates the queue as far as flag O_CREAT is included
    int x = mq_getattr(server, &attr);
    printf("x: %d\n", x);

    if(server == -1) {
	return -1;
    }


    // This is the implementation of the server
    do {
        // Attempt to receive a message from the queue.
        ssize_t received = mq_receive(server, (char*)&msg, sizeof(msg), NULL);
        if (received != sizeof(msg)) {
            // This implicitly also checks for error (i.e., -1)
            hadError = 1;
            continue;
        }

        switch (msg.command)
        {
            case CmdExit:
                // End this loop.
                didExit = 1;
                break;

            case CmdAdd:
                // Print the required output.
                printf(FORMAT_STRING_ADD,
                       msg.parameter1,
                       msg.parameter2,
                       msg.parameter1 + msg.parameter2);
                break;

            case CmdSubtract:
                // Print the required output.
                printf(FORMAT_STRING_SUBTRACT,
                       msg.parameter1,
                       msg.parameter2,
                       msg.parameter1 - msg.parameter2);
                break;

            default:
                break;
        }
    } while (!didExit); //  do {...} while (condition) -> run it at least once before checking the condition

    // Close the message queue on exit and unlink it
    mq_close(server);
    mq_unlink(QUEUE_NAME);

    return hadError ? -1 : 0;
}
