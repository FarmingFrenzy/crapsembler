#include <stdio.h>
#include <stdlib.h>
#include "binaryoperations.h"

unsigned char appendToData(char** data, char newdata, int* DC) {
	char* temp;
	temp = (char*)realloc(*data, ((*DC) + 1)*sizeof(char));
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
	size = INDEXFROMIC(*IC);
	int* temp;
	temp = (int*)realloc(*instructions, sizeof(int)*(size+1));
	if(temp) {
		*instructions = temp;
		(*instructions)[size] = newinstruction;
		return 0;
	}
	printf("appendToInstructions: Memory allocation failed\n");
	return 1;
}
