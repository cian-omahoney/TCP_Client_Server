/*======================================================================

	University College Dublin
	COMP20200 - UNIX Programming

	Assignment 3: 	Compute product of nxn matrix.
	
	Project:	assign3_19351611		
	File Name:   	MW_header.h
	Description: 	Header file shared by worker and master.
	Author:      	Cian O'Mahoney
	Student Number:	19351611
	Email:		cian.omahoney@ucdconnect.ie
	Date:        	14/4/2021
	Version:     	1.0

======================================================================*/


// Include guards for 'MW_header.h'
#ifndef MW_HEADER_H_INCLUDED
#define MW_HEADER_H_INCLUDED


/*======================================================================
 CONSTANT DEFINITIONS
======================================================================*/
#define MATRIX_INIT 2		// Initialise all matrices with this constant.
#define BACKLOG 20		// Maxium allows backlog.
#define MAX_LINE 135		// Maximum lenght of decorative line.
#define PORTNUM "49999"    	// Port number for server.


/*======================================================================
 TYPE DEFINITIONS
======================================================================*/
typedef enum{ERROR, HEALTHY, NOT_YET} status;


/*======================================================================
 FUNCTION PROTOTYPES
======================================================================*/

// Master only:
void print_array(float*, int);
void printTwoDimArray(float**, int);
void* masterServer(void* input);
void array_converter(float**, float*, int);
void stitch_array(int, int, float**, float**);

// Shared:
status write_complete(int, int, float*);
status read_complete(int, int, float*);

// Worker only:
void* workerClient(void* input);
void mult_matrix(int n, int p, float* A, float* B, float* C);

#endif
