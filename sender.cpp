//Albert, Ben, Joseph, Jyo
#include <sys/shm.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"    /* For the message struct */

/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* Id for shared memory segment and message queue */
int shmid, msqid;

/* Pointer to shared memory */
void* sharedMemPtr;

/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory
 * @param msqid - the id of the shared memory
 */

void init(int& shmid, int& msqid, void*& sharedMemPtr)
{
	/* TODO:
1. Create a file called keyfile.txt containing string "Hello world" (you may do so manually or from the code).
	   2. Use ftok("keyfile.txt", 'a') in order to generate the key.
	   3. Use the key in the TODO's below. Use the same key for the queue
and the shared memory segment. This also serves to illustrate the difference between the key and the id used in message queues and shared memory. The id for any System V object (i.e. message queues, shared memory, and sempahores)is unique system-wide among all SYstem V objects. Two objects, on the other hand, may have the same key.
	 */

	/* TODO: Get the id of the shared memory segment. The size of the segment must be SHARED_MEMORY_CHUNK_SIZE */
	/* TODO: Attach to the shared memory */
	/* TODO: Attach to the message queue */
	/* Store the IDs and the pointer to the shared memory region in the corresponding parameters */

	// Gets key
	key_t key = ftok("keyfile.txt", 'a');

	// Get the id of the shared memory segment
	if ( (shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0644 | IPC_CREAT)) == -1 ) {
		perror("(shmget) Error getting shared memory seg_id");
		exit(1);
	}

	// Attach to shared memory
	//maps shared block and gives pointer to it
	sharedMemPtr = shmat(shmid, (void *) 0, 0);
	if (sharedMemPtr == (char *)(-1)) {
		perror("(shmat) Error getting a pointer to shared memory");
		exit(1);
	}

	// Attach to message queue
	if ( (msqid = msgget(key, 0644 | IPC_CREAT)) == -1) {
		perror("(msgget) Error getting message from queue");
		exit(1);
	}
}
/**
 * Performs the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */

void send(const char* fileName)
{
	/* Open the file for reading */
	FILE* fp = fopen(fileName, "r");

	/* A buffer to store message we will send to the receiver. */
	message sndMsg;

	/* A buffer to store message received from the receiver. */
	message rcvMsg;

	/* Was the file open? */
	if(!fp)
	{
		perror("fopen");
		exit(-1);
	}

	/* Read the whole file */
	while(!feof(fp))
	{
		/* Read at most SHARED_MEMORY_CHUNK_SIZE from the file and store them in shared memory.
 		 * fread will return how many bytes it has actually read (since the last chunk may be less
 		 * than SHARED_MEMORY_CHUNK_SIZE).
 		 */
		 if((sndMsg.size = fread(sharedMemPtr, sizeof(char), SHARED_MEMORY_CHUNK_SIZE, fp)) < 0)
 		{
 			perror("error: fread");
 			exit(-1);
 		}

		/* Send a message to the receiver reporting that the data is ready
 		 * (message of type SENDER_DATA_TYPE)
 		 */
		sndMsg.mtype = SENDER_DATA_TYPE;
		if(msgsnd(msqid, &sndMsg , sizeof(struct message) - sizeof(long), 0) == -1)
		{
			perror("error: msgsnd");
			exit(1);
		}

		/* Wait until the receiver sends us a message of type RECV_DONE_TYPE
 		 * indicating the memory chunk is saved.
 		 */
		 if(msgrcv(msqid, &rcvMsg, sizeof(struct message) - sizeof(long), RECV_DONE_TYPE, 0) == -1)
 		{
 			perror("error: msgrcv");
 			exit(1);
 		}
	}
	/** Let the receiver know that we have nothing more to send after we have
 	  * finished sending the file. We will do this by sending a message of type
	  *	SENDER_DATA_TYPE with size field set to 0.
	  */
		sndMsg.size = 0;
		if(msgsnd(msqid, &sndMsg , sizeof(struct message) - sizeof(long), 0) == -1)
		{
			perror("error: msgsnd");
			exit(-1);
		}
	/* Close the file */
	fclose(fp);
}

void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr)
{
	//detach from shared memory
	if (shmdt(sharedMemPtr) == -1) {
		perror("(shmdt) Error while trying to detach from shared memory");
		exit(1);
	}
}
/**
 * The main send function
 * @param fileName - the name of the file
 */
int main(int argc, char** argv)
{
	/* Check the command line arguments */
	if(argc < 2)
	{
		fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
		exit(-1);
	}

	/* Connect to shared memory and the message queue */
	init(shmid, msqid, sharedMemPtr);

	/* Send the file */
	send(argv[1]);

	/* Cleanup */
	cleanUp(shmid, msqid, sharedMemPtr);

	return 0;
}
