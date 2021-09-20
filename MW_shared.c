/*======================================================================

	University College Dublin
	COMP20200 - UNIX Programming

	Assignment 3: 	Compute the product of two nxn matrices.
	
	Project:	assign3_19351611		
	File Name:   	master.c
	Description:    This file contains a number of functions 
			required to implement both master and worker.
	Author:      	Cian O'Mahoney
	Student Number:	19351611
	Email:		cian.omahoney@ucdconnect.ie
	Date:        	14/4/2021
	Version:     	1.0

======================================================================*/




/*======================================================================
Systems header files
======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>

#include "MW_header.h"





/*======================================================================
 * FUNCTION:	printTwoDimArray()
 * ARGUMENTS:	Square, 2d array to print.
 * 		length of one side.
 * RETURNS:	nothing.
 * DESCRIPTION:	Prints formated 2D array with decorative lines.
 *====================================================================*/
void printTwoDimArray(float** array, int size)
{
	int i,j;
	int line_len = size*9;

	if(line_len > MAX_LINE)
		line_len = MAX_LINE;

	// Print line above:
	for(i=0; i<(line_len); i++)
		printf("=");
	printf("\n");	

	// Print array:
	for(i=0; i<size; i++)
	{
		for(j=0; j<size; j++)
			fprintf(stdout, "%8.2f ", array[i][j]);
		fprintf(stdout, "\n");
	}

	// Print line below:
	for(i=0; i<(line_len); i++)
		printf("=");
	printf("\n");

} // End of 'printTwoDimArray()'.




/*======================================================================
 * FUNCTION:	array_converter()
 * ARGUMENTS:	2D array.
 * 		Array to save 1D array into
 * 		Size of array side.
 * RETURNS:	Nothing.
 * DESCRIPTION:	Converts 2D square array of side 'size' to sting.
 *====================================================================*/
void array_converter(float** twoD, float* oneD, int size)
{	
	int i,j;

	for(i=0; i<size; i++)
		for(j=0; j<size; j++)
			oneD[(size*i)+j] = twoD[i][j];

} // End of 'array_converter()'.





/*======================================================================
 * FUNCTION:	stich_array()
 * ARGUMENTS:	n: size of matrices.
 * 		p: number of worker processes.
 * 		sliceArray: array of array pointers holding slices of C.
 * 		C: array to hold combined matrix.	
 * RETURNS:	nothing.
 * DESCRIPTION:	Combines slices of an array into a single 2D array.
 * 		Ensures slices are in the correct order.
 *====================================================================*/
void stitch_array(int n, int p, float** sliceArray, float** C)
{
	int i, j, k, l, id;
	for(i=0; i<p; i++)
	{	
		// The ID number of that particular set of slices:
		id = (int)sliceArray[i][0];

		l=0;
		for(k=(id*(n/p)); k<((n/p)*(id+1)); k++,l++)
			for(j=0; j<n; j++)
				C[k][j] = sliceArray[i][(l*n) + j + 1];
	}

} // End of 'stich_array()'.






/*======================================================================
 * FUNCTION:	write_complete()
 * ARGUMENTS:   cfd: file descriptor to write to.
 * 		size: size of array to write.
 * 		buf: array to write to file.
 * RETURNS:	status variable.
 * DESCRIPTION:	Write buffer to file descriptor cfd using send().
 * 		Ensured that all data is written, even if interupt
 * 		signal is received.
 *====================================================================*/
status write_complete(int cfd, int size, float* buf)
{
	int totWritten;
	float* bufr = buf;
	ssize_t numWritten;

	for (totWritten = 0; totWritten < sizeof(float)*size; )
	{
        	numWritten = send(cfd, bufr, (sizeof(float)*size) - totWritten, 0);
        	if (numWritten < 0)
		{
           		if (numWritten == -1 && errno == EINTR)
              			continue;
           		else
			{
           			perror("write_complete()");
				return ERROR;
           		}
        	}
        	totWritten += numWritten;
		bufr += numWritten / sizeof(float);
    	}

	return NOT_YET;

}  // End of 'write_complete()'.





/*====================================================================
 * FUNCTION:	read_complete()
 * ARGUMENTS:   cfd: file descriptor to read from.
 * 		size: size of array to read.
 * 		buf: array to save what is read.
 * RETURNS:	status variable.
 * DESCRIPTION:	Read buffer from file descriptor cfd using recv().
 * 		Ensured that all data is read, even if interupt
 * 		signal is received.
 *====================================================================*/
status read_complete(int cfd, int size, float* buf)
{
	int totRead;
	float* bufr = buf;
	ssize_t numRead;

	for (totRead = 0; totRead < sizeof(float)*size; )
	{
        	numRead = recv(cfd, bufr, (sizeof(float)*size) - totRead, 0);
        	if (numRead < 0)
		{
           		if (numRead == -1 && errno == EINTR)
              			continue;
           		else
			{
           			perror("read_complete()");
				return ERROR;
           		}
        	}
		if (numRead == 0)
			return ERROR;

        	totRead += numRead;
		bufr += numRead / sizeof(float);
    	}

	return NOT_YET;

}  // End of 'read_complete()'. 





/*======================================================================
 * FUNCTION:	mult_matrix()
 * ARGUMENTS:	n: size of matrices.
 * 		p: number of worker processes.	
 * 		A: array A
 * 		B: array B
 * 		C: array C
 * RETURNS:	nothing.
 * DESCRIPTION:	Calculate slice of matrix C from slices of matrix A
 * 		and matrix B, where AxB=C.
 *====================================================================*/
void mult_matrix(int n, int p, float* A, float* B, float* C)
{	
	int i, j, k, c_inc = 0;
	float sum = 0;

	for(i=0; i<n/p; i++)
	{
		for(k=0; k<n; k++, c_inc++)
		{	
			// Calculate a single element of C:
			sum = 0;
			for(j=0; j<n; j++)
			{
				sum += A[(i*n) + j]*B[(j*n) +k];
			}
			C[c_inc] = sum;
		}
	}

}  // End of 'mult_matrix()'.
