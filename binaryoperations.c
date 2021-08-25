#include <stdio.h>
#include <stdlib.h>
#include "binaryoperations.h"

/*A function that takes a pointer to a byte array, a new byte to append to it, and a pointer to DC.
* It allocates a new byte in the array, puts the new data in it, and increments DC
*/
unsigned char appendToData(char** data, char newdata, int* DC) {
	char* temp;
    /*Reallocate the array*/
	temp = (char*)realloc(*data, ((*DC) + 1)*sizeof(char));
	if(temp) {
        /*If the allocate was successful, put newdata as the last element, and increment DC*/
		*data = temp;
		(*data)[*DC] = newdata;
		(*DC)++;
		return 0;
	}
	printf("appendToData: Memory allocation failed\n");
	return 1;
}


/*A function that takes a pointer to an int array, a new int to append to it, and a pointer to IC.
* It allocates a new int in the array, puts the new instruction in it, and increments IC by four
*/
unsigned char appendToInstructions(int** instructions, int newinstruction, int* IC) {
	int size;
    int* temp;
    /*Get the size using the INDEXFROMIC macro*/
	size = INDEXFROMIC(*IC);
    /*Reallocate more space*/
	temp = (int*)realloc(*instructions, sizeof(int)*(size+1));
	if(temp) {
        /*If the allocation was successful, add the new instruction and increment IC by four*/
		*instructions = temp;
		(*instructions)[size] = newinstruction;
		(*IC)+=4;
		return 0;
	}
	printf("appendToInstructions: Memory allocation failed\n");
	return 1;
}

/*A function that checks if the reg bit is set on a given instruction*/
int hasReg(int instruction) {
    return instruction & (1 << 25);
}
