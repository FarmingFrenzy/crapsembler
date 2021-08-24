#include <stdio.h>
#include <stdlib.h>
#include "binaryoperations.h"

void printbits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i = size-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("");
}


unsigned char appendToData(char** data, char newdata, int* DC) {
	char* temp;
	temp = (char*)realloc(*data, ((*DC) + 1)*sizeof(char));
    printf("appending:");
    printbits(sizeof(char), &newdata);
	if(temp) {
		*data = temp;
		(*data)[*DC] = newdata;
		(*DC)++;
		return 0;
	}
	printf("appendToData: Memory allocation failed\n");
	return 1;
}

unsigned char appendToInstructions(int** instructions, int newinstruction, int* IC) {
	int size;
    int* temp;
    printf("appending:");
    printbits(sizeof(int), &newinstruction);
	size = INDEXFROMIC(*IC);
	temp = (int*)realloc(*instructions, sizeof(int)*(size+1));
	if(temp) {
		*instructions = temp;
		(*instructions)[size] = newinstruction;
		(*IC)+=4;
		return 0;
	}
	printf("appendToInstructions: Memory allocation failed\n");
	return 1;
}


int hasReg(int instruction) {
    return instruction & (1 << 25);
}
