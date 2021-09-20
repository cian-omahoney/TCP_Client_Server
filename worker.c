/*======================================================================

	University College Dublin
	COMP20200 - UNIX Programming

	Assignment 3: 	Compute the product of two nxn matrices.
	
	Project:	assign3_19351611		
	File Name:   	worker.c
	Description: 	Multi-threaded worker process acts as client,
			taking slices of matrix A and entire matrix B
			from master process, compute corresponding 
			slices of matrix C, and return slices to master.
	Author:      	Cian O'Mahoney
	Student Number:	19351611
	Email:		cian.omahoney@ucdconnect.ie
	Date:        	14/4/2021
	Version:     	1.0

======================================================================*/



/*======================================================================
Systems header files
======================================================================*/
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <pthread.h>

#include "MW_header.h"	// Header file shared by master and worker.




/*======================================================================
 * FUNCTION:	main()
 * ARGUMENTS:	IP address or hostname of computer where master process
 * 		is executed.	
 * RETURNS:	Exit success or failure.
 * DESCRIPTION:	Get number of worker threads required from master.
 * 		Create this number of threads.
 *====================================================================*/
int main(int argc, char *argv[])
{       	
	struct addrinfo hints;
    	struct addrinfo *result, *rp;
	int i = 0, j; 	// For loop iteration.
	int p;		// Number of worker processes required.
 	float f_p;	
	int retVal;
	int control_fd;
	

	// Initialise hints struct with required flags:
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_canonname = NULL; 
   	hints.ai_addr = NULL;
   	hints.ai_next = NULL; 
    	hints.ai_family = AF_INET; 
  	hints.ai_socktype = SOCK_STREAM;
    	hints.ai_flags = AI_NUMERICSERV;
	
	// Initialise thread attribute to allow joining of thread:
 	pthread_attr_t tattr;
    	pthread_attr_init(&tattr);
    	pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_JOINABLE);

	// Allocate memory for arrays to hold:
	pthread_t* tid = (pthread_t *)malloc(sizeof(pthread_t)); // > thread ids.
	int* cfd = (int*)malloc(sizeof(int));			 // > file descriptors passed to threads.


	status cleanup = NOT_YET;

	// This main while loop will only execute once.
	// This ensures that if any serious error occurs during the execution,
	// the program can skip directly to the cleanup section at the end and not
	// terminated suddenly.
	while(cleanup == NOT_YET)
	{	
		// Initialise address info structures:
		if((retVal = getaddrinfo(argv[1], PORTNUM, &hints, &result) != 0))
		{
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retVal));
			cleanup = ERROR;
       			continue;
		}
		
		printf("Connecting to server...\n");
			
		//First, read control information, ie number of threads required, from master:
    		for (rp = result; rp != NULL; rp = rp->ai_next) 
		{
         		control_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		
			if (control_fd == -1)		
	        		continue;  // On error, try next address.

	       		if (connect(control_fd, rp->ai_addr, rp->ai_addrlen) == 0)	
		        	break;

        		// If failed, close this socket, try next address
        		if(close(control_fd) == -1)
			{
				perror("close()");
				cleanup = ERROR;
				break;
			}
		}
		
		if(cleanup == ERROR)
			break;

        	if(read_complete(control_fd, 1, &f_p) == ERROR)
			break;

		p = f_p;
        	
		// Close connection:
		if(close(control_fd) == -1)
		{
			perror("close()");
			break;
		}

		rp = result;

		printf("Computing matrix product...\n");

		// Cycle through address structurs until a connection is established.
		// Then create threads until number of threads requested, p, is reached:
    		while(rp != NULL && i<p) 
		{
         		cfd[i] = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	
			if (cfd[i] == -1)
			{		
				rp = rp->ai_next;		
	        		continue;  // On error, try next address
			}

	       		if (connect(cfd[i], rp->ai_addr, rp->ai_addrlen) != -1)
			{	
		        	pthread_create((tid+i), &tattr, workerClient, (void*)(cfd+i));
				i++;
				cfd = (int*)realloc(cfd, (i+1)*sizeof(int));	
				tid = (pthread_t *)realloc(tid, (i+1)*sizeof(pthread_t));
				continue;
			}

        		// If failed, close this socket, try next address
     			if(close(cfd[i]) == -1)
			{
				perror("close()");
				cleanup = ERROR;
				break;
			}

			// Move to next node in linked list:
			rp = rp->ai_next;
		}
		
		if(cleanup == ERROR)
			break;
	
		// Join number of threads created:
		for(j=0; j<i; j++)
			pthread_join(tid[j], NULL);

 	   	if (rp == NULL)
			fprintf(stderr, "No socket to bind...\n");

		cleanup = HEALTHY;
	}
	
    	pthread_attr_destroy(&tattr); 	
	free(tid);
	free(cfd);
    	freeaddrinfo(result);

	if(cleanup == HEALTHY)
	{
		fflush(stdout);
		printf("Worker finished successfully.\n");
		return EXIT_SUCCESS;
	}
	else
	{
		fflush(stdout);
		printf("Worker failed.\n");
		return EXIT_FAILURE;
	}
} // End of 'main()'





/*======================================================================
 * FUNCTION:	workerClient()
 * ARGUMENTS:	File descriptor.
 * RETURNS:	Nothing.
 * DESCRIPTION:	Reads ID, n and p from master thread.
 * 		Reads A slice and all of matrix B from master thread.
 * 		Computes corresponding slice of C.
 * 		Returns this slice to master thread.
 *====================================================================*/
void* workerClient(void* argument) 
{
	int cfd = *((int *)argument);
	float f_n, f_p, f_id;
	float *A, *B, *C;

	status cleanup = NOT_YET;
	
	while(cleanup == NOT_YET)
	{
		// Read slice id from master:
		if(read_complete(cfd, 1, &f_id) == ERROR)
			break;
		
		// Read size of matrix n from master:
		if(read_complete(cfd, 1, &f_n) == ERROR)
			break;

		// Read number of worker threads, p, from master:
		if(read_complete(cfd, 1, &f_p) == ERROR)
			break;

		int n = f_n;
		int p = f_p;
		
		// If either n or p == 0, an error has occured:
		if(n <= 0 || p <= 0)
			break;
		
		// Calculate the expected size of each array:
		int size_A = (n*n)/p;
		int size_B = (n*n);
		int size_C = size_A;
	
		// Allocate correct size of array for each matrix:
		A = (float *)malloc(sizeof(float) * size_A);
		B = (float *)malloc(sizeof(float) * size_B);
		C = (float *)malloc(sizeof(float) * size_C);
	
		// Read slices of matrix A from master:
		if(read_complete(cfd, size_A, A) == ERROR)
			break;

		// Read all of matrix B from master:
		if(read_complete(cfd, size_B, B) == ERROR)
			break;

		// Calculate corresponding slices of matrix C:
		mult_matrix(n,p,A,B,C);

		// Send matrix C slices back to master:
		if(write_complete(cfd, size_C, C) == ERROR)
			break;
		
		// If no error has occured, terminate normally:
		cleanup = HEALTHY;
	}


    	if (close(cfd) == -1) // Close connection
		perror("close()");

 	free(A);
	free(B);
	free(C);
	
	if(cleanup == HEALTHY)
		pthread_exit(NULL);

	// If an error has occured and a worker thread has failed, exit in error:
	else
	{
		fflush(stderr);
		fprintf(stderr, "Worker process failed.\n");
		exit(EXIT_FAILURE);
	}

} // End of 'workerClient()':
