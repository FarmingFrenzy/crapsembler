#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symboltable.h"
#include "commands.h"
#include "binaryoperations.h"
#define INDEXFROMIC(x) ((x - 100)/4)
#define TYPER 0
#define TYPEI 1
#define TYPEJ 2
#define TYPEERROR 3
#define STATE_READING 0
#define STATE_LABEL 1
#define STATE_DATA 2
#define STATE_CODE 3


unsigned char onlyWhitespace(char*);
unsigned char interpertLine(char*);
unsigned char isDataAllocation(char*);
unsigned char isNumber(char*);
unsigned char allocateData(int, char**, int);
void splitString(char*, char*, char***, int*);

int state = STATE_READING;
Symboltable* symboltable = NULL;
char* dataImage = NULL;
int* instructionImage = NULL;
int IC = 100;
int DC = 0;
int lineNumber = 0;

int main(int argc, char* argv[]) {
	FILE* inputFile;
	char* line_buffer = NULL;
	size_t line_buffer_size = 0;
	printf("reading file: %s\n", argv[1]);
	inputFile = fopen(argv[1], "r");
	while((line_buffer_size = getline(&line_buffer, &line_buffer_size, inputFile)) != -1) {
		lineNumber++;
		if(interpertLine(line_buffer) == 101) {
			return 101;
		}
	}
	fclose(inputFile);
	printSymboltable(symboltable);
	return 0;
}

/*
* Return 0 for comment
* Return 1 for whitespace
* Return 2 for data allocated
* Return 101 for error
*/
unsigned char interpertLine(char* line) {
	char** splitAtSpace = NULL;
	char** splitAtComma = NULL;
	int splitAtCommaLength = 0;
	int splitAtSpaceLength = 0;
	char* labelName = NULL;
	int dataType = 0;
	/*Check if line is a comment*/
	if(line[0] == ';') {
		return 0;
	}
	/*Check if line is only whitespace*/
	if(onlyWhitespace(line)) {
		return 1;
	}
	splitString(line, " ", &splitAtSpace, &splitAtSpaceLength);
	if(state == STATE_READING) {
		/*TODO: trim input*/
		if(splitAtSpaceLength > 0) {
			if(isDataAllocation(splitAtSpace[0])) {
				dataType = isDataAllocation(splitAtSpace[0]);
				state =  STATE_DATA;
			}
			if(splitAtSpace[0][strlen(splitAtSpace[0])-1] == ':') {
				/*TODO: make a function of this*/
				labelName = (char*)calloc(strlen(splitAtSpace[0])-1, sizeof(char));
				/*TODO: check for errors*/
				memcpy(labelName, splitAtSpace[0], strlen(splitAtSpace[0])-1);
				if(splitAtSpaceLength < 2) {
					printf("Label %s at line %d undefined\n", labelName, lineNumber);
					return 101;
				}
				if(onlyWhitespace(splitAtSpace[1])) {
					printf("Label %s at line %d undefined\n", labelName, lineNumber);
					return 101;
				}
				if(!labelIsValid(labelName)) {
					printf("Invalid label name at line %d: %s\n", lineNumber, labelName);
					return 101;
				}
				if(labelExists(symboltable, labelName)) {
					printf("Label %s at line %d already exists.\n", labelName, lineNumber);
					return 101;
				}
				dataType = isDataAllocation(splitAtSpace[1]);
				if(dataType) {
					Symboltable* sym;
					initSymbol(&sym);
					setName(sym, labelName);
					setAddress(sym, DC);
					addAttribute(sym, ATTR_DATA);
					appendSymbol(&symboltable, sym);
					state = STATE_DATA;
				}
				if(getOpcodeFromName(splitAtSpace[1]) != 64) {
					Symboltable* sym;
					initSymbol(&sym);
					setName(sym, labelName);
					setAddress(sym, IC);
					addAttribute(sym, ATTR_CODE);
					appendSymbol(&symboltable, sym);
					state = STATE_CODE;
				}
			}
		}
		if(state == STATE_DATA) {
			if(labelName) {
				splitString(splitAtSpace[2], ",", &splitAtComma, &splitAtCommaLength);
				allocateData(dataType, splitAtComma, splitAtCommaLength);
			}
			else {
				splitString(splitAtSpace[1], ",", &splitAtComma, &splitAtCommaLength);
				allocateData(dataType, splitAtComma, splitAtCommaLength);
			}
			state = STATE_READING;
			return 2;
		}
		if(state == STATE_CODE) {
			state = STATE_READING;
		}
	}
	return 0;
}

unsigned char allocateData(int dataType, char** dataList, int dataListLength) {
	int value;
	char splitValue[4];
	int i;
	int j;
	int ret;
	if(dataType == 1) {
		for(i = 0; i < dataListLength; i++) {
			if((ret = isNumber(dataList[i])) == 0) {
				printf("Not a number %s in line %d\n", dataList[i], lineNumber);
				return 1;
			}
 			value = atoi(dataList[i]);
			if((ret = appendToData(&dataImage, value, &DC)) == 1) {
				printf("Memory allocation failure %s in line %d\n", dataList[i], lineNumber);
				return 1;
			}
		}
	}
	else if(dataType == 2) {
		for(i = 0; i < dataListLength; i++) {
			if((ret = isNumber(dataList[i])) == 0) {
				printf("Not a number %s in line %d\n", dataList[i], lineNumber);
				return 1;
			}
			value = atoi(dataList[i]);
			printf("got value %d from %s\n", value, dataList[i]);
			splitValue[0] =  value & 0x000000FF;
			splitValue[1] = (value & 0x0000FF00) >> 8;
			splitValue[2] = (value & 0x00FF0000) >> 16;
			splitValue[3] = (value & 0xFF000000) >> 24;
			for(j = 0; j < 4; j++) {
				/*printf("writing %d from dh \n", splitValue[j]);*/
				if((ret = appendToData(&dataImage, splitValue[j], &DC)) == 1) {
					printf("Memory allocation failure %s in line %d\n", dataList[i], lineNumber);
					return 1;
				}
			}
		}
	}
	else if(dataType == 3) {
		for(i = 0; i < dataListLength; i++) {
			if((ret = isNumber(dataList[i])) == 0) {
				printf("Not a number %s in line %d\n", dataList[i], lineNumber);
				return 1;
			}
			value = atoi(dataList[i]);
			printf("got value %d from %s\n", value, dataList[i]);
			splitValue[0] =  value & 0x000000FF;
			splitValue[1] = (value & 0x0000FF00) >> 8;
			for(j = 0; j < 2; j++) {
				/*printf("writing %d from dh \n", splitValue[j]);*/
				if((ret = appendToData(&dataImage, splitValue[j], &DC)) == 1) {
					printf("Memory allocation failure %s in line %d\n", dataList[i], lineNumber);
					return 1;
				}
			}
		}
	}
	else if(dataType == 4) {
		if(dataList[0][strlen(dataList[0])-2] != '"' || dataList[0][0] != '"') {
			printf("Error in line %d, missing quotation marks\n", lineNumber);
			return 1;
		}
		j = strlen(dataList[0]);
		for(i = 1; i < j-2; i++) {
			if((ret = appendToData(&dataImage, dataList[0][i], &DC)) == 1) {
				printf("Memory allocation failure %s in line %d\n", dataList[0], lineNumber);
				return 1;
			}
		}
		if((ret = appendToData(&dataImage, 0, &DC)) == 1) {
			printf("Memory allocation failure %s in line %d\n", dataList[0], lineNumber);
			return 1;
		}
	}
	return 0;
}

unsigned char isNumber(char* str) {
	int i;
	int stringlength = strlen(str);
	for(i = 0; i < stringlength; i++) {
		if(i == stringlength - 1 && str[i] == '\n') {
			continue;
		}
		if(i == 0) {
			if((str[i] >= '0' && str[i] <= '9') || (str[i] == '-')) {
				continue;
			}
			printf("here\n");
			return 0;
		}
		if((str[i] >= '0' && str[i] <= '9')) {
			continue;
		}
		return 0;
	}
	return 1;
}

void splitString(char* str, char* split, char*** res, int* size) {
	char* token;
	int resSize;
	*res = (char**)calloc(0, sizeof(char*));
	token = strtok(str, split);
	resSize = 0;
	while(token != NULL) {
		*res = (char**)realloc(*res, ++resSize*sizeof(char*));
		(*res)[resSize-1] = (char*)calloc(strlen(token), sizeof(char));
		(*res)[resSize-1] = token;
		token = strtok(NULL, split);
	}
	*size = resSize;
}


unsigned char onlyWhitespace(char* line) {
	int length = strlen(line);
	int i;
	for(i = 0; i < length; i++) {
		if(!(line[i] == '\t' || line[i] == ' ' || line[i] == '\n' || line[i] == 0x00)) {
			return 0;
		}
	}
	return 1;
}

/*A function that checks if a word is a keyword for data allocation.
* Returns 0 if not
* Returns 1 for db
* Returns 2 for dw
* Returns 3 for dh
* Returns 4 for asciz
*/
unsigned char isDataAllocation(char* word) {
	if(strcmp(word, ".db") == 0) {
		return 1;
	}
	if(strcmp(word, ".dw") == 0) {
		return 2;
	}
	if(strcmp(word, ".dh") == 0) {
		return 3;
	}
	if(strcmp(word, ".asciz") == 0) {
		return 4;
	}
	return 0;
}
