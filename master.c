/*======================================================================

	University College Dublin
	COMP20200 - UNIX Programming

	Assignment 3: 	Compute the product of two nxn matrices.
	
	Project:	assign3_19351611		
	File Name:   	master.c
	Description: 	Multi-threaded master process acts as server,
			sending slices of matrix A and entire matrix B
			to worker process.  Received slices back from
			worker process and recombines these to form 
			matrix C.
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

#include "MW_header.h"


/* Argument:
*	Global structure to hold data shared by main thread and
*	other threads.
*		n:	Size of matrices.
*		p: 	Number of worker processes.
*		cfd:	Array to hold array file descriptors.
*		A:	Array A.
*		B:	Array B.
*		offset:	Array to hold offset of A used be threads.
*/
typedef struct
{
	int n;
	int p;
	int* cfd;
	float* A;
	float* B;
	long int* offset;
}Argument;

// Global struct shared by main and other threads.
Argument request;


/*======================================================================
 * FUNCTION:	main()
 * ARGUMENTS:	n: size of matrices.
 * 		p: number of worker processes.	
 * RETURNS:	Exit success or failure.
 * DESCRIPTION:	Creates TCP connection with worker process.
 * 		Creates p threads and pass these threads slices of A.
 * 		Collect slices of C back from threads.
 *====================================================================*/
int main(int argc, char *argv[])
{
	status cleanup = NOT_YET;


	/**************************************************************/
	/* CHECK USER INPUT IS VALID:	 			      */

	int n,p;	// n: size of matrices. p: number of workers.
	int i,j;	// To increment loops.

	if(argc < 3)
	{
		printf("Too few arguments!\n");
		printf("Usage: %s [matrix size] [worker processes]\n", argv[0]);
		return EXIT_FAILURE;
	}

	// Convert n input to integer.
	// If n is not a positive integer, return error.
        if(((n = atoi(argv[1])) == 0) || n<0) 
	{
		fprintf(stdout, "Incorrect matrix size provided. Must be positive integer.\n");
		return EXIT_FAILURE;
	}
        
	// Comnvert p input to integer.
	// If p is not a positive integer, or is not a divisor of the matrix size, return error.
	if( ((p = atoi(argv[2])) == 0) || n%p != 0 || p>n || p<=0)
	{
		fprintf(stdout, "Incorrect number of worker processes provided. Must be positive integer divisor of matrix size.\n");
		return EXIT_FAILURE;
	}

	// If n and p are suitable, assign to global variables to be accessible to all threads.
	request.n = n;
	request.p = p;
	/*							      */	
	/**************************************************************/
	



	/*************************************************************/
	/* INITIALISE MATRIX A, B AND C:			     */
	
	// A, B and C are two-dimensional arrays of floats.
	// Allocate memory for A,B and C.
	float** A = (float **)malloc(n*sizeof(float*));
	float** B = (float **)malloc(n*sizeof(float*));
	float** C = (float **)malloc(n*sizeof(float*));

	for(i=0; i<n; i++)
	{
		A[i] = (float *)malloc(n*sizeof(float));
		B[i] = (float *)malloc(n*sizeof(float));
		C[i] = (float *)malloc(n*sizeof(float));
	}
	
	// Initialise A and B elements to constant value.
	for(i=0; i<n; i++)
		for(j = 0; j<n; j++)
		{	
			A[i][j] = MATRIX_INIT;
			B[i][j] = MATRIX_INIT;
		}
	
	// Print array A and B to stdout.
	fprintf(stdout, "Matrix A:\n");
	printTwoDimArray(A, n);

	fprintf(stdout, "\nMatrix B:\n");
	printTwoDimArray(B, n);
	
	// A, B and C Matrices:
	// 	> A, B and C are two-dimensional arrays of floats.
	// 	> A one-dimensional copy of A and B is made to the global structure 'request'.
	// 	> Work is done with these one-dimensional copies and a one-dimensional array of C is received.
	// 	> This is then converted to the two-dimensional array C.
	// This was done as were more features to be added to this program, it would be more
	// convenient to have all the arrays as two-dimensional, although it is not neccesarily required.
	
	// Here A and B are converted to one-dimensional arrays
	request.A = (float *)malloc(n*n*sizeof(float));
	request.B = (float *)malloc(n*n*sizeof(float));
	
	array_converter(A, request.A, n);
	array_converter(B, request.B, n);
	/*							      */	
	/**************************************************************/



	/*************************************************************/
	/* INITIALISE DATA				 	     */
	
	struct addrinfo hints;
    	struct addrinfo *result, *rp;
	int retVal;
	int lfd, optval  = 1;
	int num_threads;
	int control_fd;
	float f_p = p;
	
	// Initialise hints struct with required parameters.
  	memset(&hints, 0, sizeof(struct addrinfo));
    	hints.ai_canonname = NULL; 
    	hints.ai_addr = NULL;
    	hints.ai_next = NULL; 
    	hints.ai_family = AF_INET; 		// Using IP4 addresses only.
    	hints.ai_socktype = SOCK_STREAM;	// Using socket stream.
    	hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;


	// Initialise thread attribute. Threads must be joinable. 
	pthread_attr_t tattr;
    	pthread_attr_init(&tattr);
    	pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_JOINABLE);
	

	// Allocate memory for arrays to hold:
	pthread_t* tid = (pthread_t *) malloc(n * sizeof(pthread_t));	// > Thread ids.
	int* identifier = (int*) malloc(p * sizeof(int));		// > Thread matrix A slice identifiers.
	float** returned = (float**) malloc(p * sizeof(float*));	// > Slices of C returned from threads.
	request.offset = (long int*)malloc(p * sizeof(long int));	// > Matrix A offset values.
	request.cfd = (int *)malloc(n * sizeof(int));   		// > File descriptors passed to threads.
	/*							      */	
	/**************************************************************/




	// This main while loop will only execute once.
	// This ensures that if any serious error occurs during the execution,
	// the program can skip directly to the cleanup section at the end and not
	// terminated suddenly.
	while(cleanup == NOT_YET)
	{
		/******************************************************************/
		/* INITIALISE SOCKET:						  */

		// Initialise address info structures:
    		if((retVal = getaddrinfo(NULL, PORTNUM, &hints, &result)) != 0)
		{
       			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retVal));
			cleanup = ERROR;
			continue;
		}

		// Attempt to create socket with address info structures:
    		for (rp = result; rp != NULL; rp = rp->ai_next)
		{
         		lfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

         		if (lfd == -1) // On error, try next address.
            			continue;


         		if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
			{
            			cleanup = ERROR;
				break;
			}
			
			// Attempt to bind to socket created:
         		if (bind(lfd, rp->ai_addr, rp->ai_addrlen) == 0) // Success.
            			break;

         		// If bind() failed, close this socket, try next address.
         		if(close(lfd) == -1)
			{
				perror("close");
				cleanup = ERROR;
				break;
			}
    		}

		if(cleanup != NOT_YET)
			break;
		
    		if (rp == NULL)
		{
       			cleanup = HEALTHY;
			break;
		}
		
		// Display host and service to user:
		{	
       			char host[NI_MAXHOST];
       			char service[NI_MAXSERV];
       			if (getnameinfo((struct sockaddr *)rp->ai_addr, rp->ai_addrlen, host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
          			fprintf(stdout, "\nListening on (%s, %s)...\n\n", host, service);
       			else
			{
          			cleanup = ERROR;
				break;
			}
    
		}
		
		// Listen for connections to socket:
    		if (listen(lfd, BACKLOG) == -1)
		{
			cleanup = ERROR;
			break;
		}
		/*								  */
		/******************************************************************/
		


		/******************************************************************/
		/* SEND CONTROL INFORMATION TO WORKER:				  */
		
		// Accept a connection to the socket.
		// This connection will be used to send initial control information to the worker.
		if((control_fd = accept(lfd, NULL, NULL)) == -1)
		{
			perror("accept()");
			cleanup = ERROR;
			break;
		}
		
		// Write control information, this this case the number of worker threads required, to the socket.
		if((cleanup = write_complete(control_fd, 1, &f_p)) != NOT_YET)
			break;
		
		// Close file descriptor:
		if(close(control_fd) == -1)
		{
			perror("close()");
			cleanup = ERROR;
			break;
		}
		/*								  */
		/******************************************************************/
		


		/******************************************************************/
		/* ACCEPT CONNECTIONS AND CREATE THREADS			  */

		// Accept connections from worker threads and create server threads to
		// send and receive data with worker threads:
    		for (num_threads = 0; num_threads < p;)
    		{
        		if((*(request.cfd + num_threads) = accept(lfd, NULL, NULL)) == -1)
			{
				perror("accept()");
				continue;
			}

			// Fill 'identifier' array with values which inform a certain thread
			// what slice of A-matrix it has received:
			identifier[num_threads] = num_threads;

			// Offset array informs each thread what offset to use with the 
			// A-matrix to ensure they get the correct slices.
			request.offset[num_threads] = ((n*n)/p)*(num_threads);
			
			// Create threads and pass them unique ID values:
        		if((retVal = pthread_create(&tid[num_threads], &tattr, masterServer,(void*)(identifier + num_threads))) != 0)
			{
				errno = retVal;
				perror("pthread_create()");
				cleanup = ERROR;
				break;
			}
			
			num_threads++;
    		}

		// Join how ever many threads were created:
		for(i=0; i<num_threads; i++)
		{
			if((retVal = pthread_join(tid[i], (void*)(returned+i))) != 0)
			{
				errno =  retVal;
				perror("pthread_join()");
				cleanup = ERROR;
				break;
			}
			// If threads return a null pointer, an error has occured:
			if((returned+i) == NULL)
				cleanup = ERROR;
		}
		/*								  */
		/******************************************************************/
	
		if(cleanup != NOT_YET)
			break;

		
		//Stitch array C together from slices received from threads:
		stitch_array(n, p, returned, C);
		
		// Print array C to standard out:
		fprintf(stdout, "Matrix C:\n");	
		printTwoDimArray(C, n);
		
		// If no error has occured by this point, exit succussfully:
		cleanup = HEALTHY;

	} // 'while-loop' end.
	/*							     */	
	/*************************************************************/



	/*************************************************************/
	/* Clean up and finish.					     */
	
	freeaddrinfo(result);
   	pthread_attr_destroy(&tattr); 

	for(i=0; i<num_threads; i++)
		free(returned[i]);

	free(returned);
	free(tid);
	free(request.cfd);
	free(request.A);
	free(request.B);
	free(request.offset);
	free(identifier);


	for(i=0; i<n; i++)
	{	
		free(A[i]);
		free(B[i]);
		free(C[i]);
	}

	free(A);
	free(B);
	free(C);

	if(cleanup == HEALTHY)
	{
		printf("\nMaster process finished successfully.\n\n");
		return EXIT_SUCCESS;
	}
	else
	{
		fflush(stdout);
		fprintf(stdout, "\nMaster process failed.\n\n");
		return EXIT_SUCCESS;
	}
	/*							     */
	/*************************************************************/

} // End of 'main()'




	
	
/*======================================================================
 * FUNCTION:	masterServer()
 * ARGUMENTS:	identifier: variable to identify which slices of matrix
 * 		A a certain thread should operate one.
 * RETURNS:	Pointer to slice of C on success.
 * DESCRIPTION: Create a frame to send to worker thread:
 * 		HEADER: > id number
 * 			> n
 * 			> p
 * 		PAYLOAD:> slices of A.
 * 			> All of matrix B.
 * 		Received computed frame back from worker.
 *====================================================================*/
void* masterServer(void* identifier)
{
	int id = *(int*)identifier;
	int cfd = request.cfd[id];
	int n = request.n;
	int p = request.p;
	int offset = request.offset[id];
	int i;			// To increment loops
	status wret = NOT_YET; 	//Return value for write_complete
	status rret =  NOT_YET; // Return value for read_complete


	// Calcualte size of slices of C array to be received from worker:
	int size_C = (n*n)/p;
	
	// Calculate size of request to send:
	int request_size = 3 + (n*n) + ((n*n)/p);
	
	// Allocate memory for request message 'buf'
	// and received array.
	float* buf = (float *) malloc (sizeof(float) * request_size);
	float* received = (float *) malloc(sizeof(float)*(size_C + 1));

	// Build frame around data payload to be sent to worker thread:
	buf[0] = id;
   	buf[1] = n;
	buf[2] = p;

	for(i=3; i<(request_size - (n*n)); i++)
		buf[i] = request.A[offset + i-3];

	for(; i<request_size; i++)
		buf[i] = request.B[i-(request_size - (n*n))]; 

	// Send request frame to worker thread:
	wret = write_complete(cfd, request_size, buf);
	
	// Received computed frame from worker thread:
	received[0] = id;
	rret = read_complete(cfd, size_C, received+1);

	free(buf);

	if(close(cfd) == -1)
		perror("close()");
	
	// If an error occured, exit with error:
	if(wret == ERROR || rret == ERROR)
	{	
		fprintf(stderr, "Error occured in thread %d.\n", id);
		fflush(stdout);
		fprintf(stderr, "Master process failed.\n");
		exit(EXIT_FAILURE);
	}
	// If not error occurect return the received data:
    	pthread_exit((void*)received);
    	return NULL;

} // End of 'masterServer()'.
