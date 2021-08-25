#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>
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
unsigned char interpertLineSecond(char*);
unsigned char interpertLine(char*);
unsigned char isDataAllocation(char*);
unsigned char isNumber(char*);
unsigned char allocateData(int, char**, int);
unsigned char writeInstruction(char**, int, int, int);
unsigned char addExtern(Symboltable**, char* , int);
unsigned char hasDotas(char*);
void splitString(char*, char*, char***, int*);
void addICF();
void toUpperStr(char*);
void writeObjectFile(FILE*);
void writeEntryFile(FILE*);
void writeExtFile(FILE*);
int makeR(int, int, int, int, int);
int makeI(int, int, int, short);
int makeJ(int, int, int);
int getRegisterNumber(char*);
int compileFile(char*);
char* firstInstanceOf(char*, char);
char* getObName(char*);
char* getEntName(char*);
char* getExtName(char*);
char* trimLine(char*);

Symboltable* symboltable = NULL;
Symboltable* externalList = NULL;
char* dataImage = NULL;
int* instructionImage = NULL;
int state = STATE_READING;
int IC = 100;
int DC = 0;
int ICF = 0;
int DCF = 0;
int lineNumber = 0;

int main(int argc, char* argv[]) {
    int i;
    /*Loop over all the arguments starting at one*/
    for(i = 1; i < argc; i++) {
        /*Check if the file exists*/
        if(access(argv[i], F_OK) != 0) {
            printf("file %s doesn't exist\n", argv[i]);
            return 2;
        }
        /*Check if the file has a .as extention*/
        if(!hasDotas(argv[i])) {
            printf("file does not have .as extention\n");
            return 2;
        }
        /*try to compile the file, if it fails at any point, return 101*/
        if(compileFile(argv[i]) == 101) {
            return 101;
        }
    }
    return 0;
}

int compileFile(char* fileName) {
	FILE* inputFile;
    FILE* outputFile;
	char* line_buffer = NULL;
	size_t line_buffer_size = 0;
	printf("reading file: %s\n", fileName);
	inputFile = fopen(fileName, "r");
    /*Go through the file line by line, incrementing line number each time*/
	while((line_buffer_size = getline(&line_buffer, &line_buffer_size, inputFile)) != -1) {
		lineNumber++;
        /*interpert the currentl line. If there's an error, exit the program*/
		if(interpertLine(line_buffer) == 101) {
			return 101;
		}
	}
    /*Add IC's final value to all things in the symboltable marked with data*/
    addICF();
    /*Second pass*/
    lineNumber = 0;
    ICF = IC;
    DCF = DC;
    IC = 100;
    DC = 0;
    state = STATE_READING;
    rewind(inputFile);
    while((line_buffer_size = getline(&line_buffer, &line_buffer_size, inputFile)) != -1) {
		lineNumber++;
		if(interpertLineSecond(line_buffer) == 101) {
			return 101;
		}
	}
    /*DONEZO*/
	fclose(inputFile);
	printSymboltable(symboltable);
    printf("Externals:\n");
    printSymboltable(externalList);
    /*WRITE OUTPUT*/
    outputFile = fopen(getObName(fileName), "w");
    IC = 100;
    writeObjectFile(outputFile);
    fclose(outputFile);
    outputFile = fopen(getEntName(fileName), "w");
    writeEntryFile(outputFile);
    fclose(outputFile);
    outputFile = fopen(getExtName(fileName), "w");
    writeExtFile(outputFile);
    fclose(outputFile);
    return 0;
}

unsigned char interpertLineSecond(char* line) {
    char* originalLine;
    char* symbolname;
    char** splitAtSpace;
    char** splitAtComma;
    int splitAtSpaceLength;
    int splitAtCommaLength;
    int opCode;
    int type;
    short diff;
    Symboltable* sym;
    /*Check if line is a comment*/
    if(line[0] == ';') {
        return 0;
    }
    /*Check if line is only whitespace*/
    if(onlyWhitespace(line)) {
        return 1;
    }
    originalLine = trimLine(line);
    line = trimLine(originalLine);
    splitString(line, " ", &splitAtSpace, &splitAtSpaceLength);
    if(!(splitAtSpaceLength > 0)) {
        return 101;
    }
    /*If we are at an entry, find a symbol with the same name in the symboltable, and add the entry attribute to it*/
    if(strcmp(splitAtSpace[0], ".entry") == 0) {
        sym = getSymbol(symboltable, splitAtSpace[1]);
        if(!sym) {
            printf("Error on line %d, symbol %s doesn't exist\n", lineNumber, splitAtSpace[1]);
            return 1;
        }
        addAttribute(sym, ATTR_ENTRY);
    }
    /*If we are at a code line, get the type, and set the state to code*/
    else if((opCode = getOpcodeFromName(splitAtSpace[0])) != 64) {
        type = getTypeFromName(splitAtSpace[0]);
        state = STATE_CODE;
    }
    /*If we are at a label with a code line, set the type and state*/
    else if(splitAtSpaceLength > 2 && (opCode = getOpcodeFromName(splitAtSpace[1])) != 64) {
        type = getTypeFromName(splitAtSpace[1]);
        state = STATE_CODE;
    }
    if(state == STATE_CODE) {
        /*If we are at an instruction of type J, which doesn't use a register and is not stop, we need to add the address of the symbol it's using*/
        if(type == TYPEJ && opCode != 63 && !hasReg(instructionImage[INDEXFROMIC(IC)])) {
            /*Get the name of symbol, check if it's a line with a label or not*/
            if(splitAtSpaceLength > 2) {
                symbolname = splitAtSpace[2];
            }
            else {
                symbolname = splitAtSpace[1];
            }
            /*Remove the newline from the symbolname*/
            if(symbolname[strlen(symbolname)-1] == '\n'){
                char* name;
                name = (char*)calloc(strlen(symbolname)-1, sizeof(char));
                memcpy(name, symbolname, strlen(symbolname)-1);
                symbolname = name;
            }
            /*Get the symbol form the symboltable. If it doesn't exist, Error out*/
            sym = getSymbol(symboltable, symbolname);
            if(sym == NULL) {
                printf("Error on line %d, symbol doesn't exist %s", lineNumber, symbolname);
                return 101;
            }
            /*If it is an external symbol, add it to the external list with an address of IC, if there's a problem adding it, Error out*/
            if(hasAttribute(sym, ATTR_EXTERN)) {
                if(addExtern(&externalList, symbolname, IC) > 0) {
                    return 101;
                }
            }
            /*If the symbol exists in this file, add it's address to the matching instruction*/
            else {
                instructionImage[INDEXFROMIC(IC)] = instructionImage[INDEXFROMIC(IC)] | sym->address;
            }
        }
        /*If it is an instruction of type I that uses a label as an argument*/
        else if(opCode >= 15 && opCode <= 18) {
            /*Get the argument list, store it in symbolname*/
            if(splitAtSpaceLength > 2) {
                symbolname = splitAtSpace[2];
            }
            else {
                symbolname = splitAtSpace[1];
            }
            /*Remove the newline*/
            if(symbolname[strlen(symbolname)-1] == '\n'){
                char* name;
                name = (char*)calloc(strlen(symbolname)-1, sizeof(char));
                memcpy(name, symbolname, strlen(symbolname)-1);
                symbolname = name;
            }
            /*Split the string at every comma*/
            splitString(symbolname, ",", &splitAtComma, &splitAtCommaLength);
            /*Get the actual symbol name*/
            symbolname = splitAtComma[2];
            sym = getSymbol(symboltable, symbolname);
            /*If the symbol doesn't exist, error out*/
            if(!sym) {
                printf("Error on line %d, symbol doesn't exist %s\n", lineNumber, symbolname);
                return 101;
            }
            /*If the symbol is external, error out*/
            if(hasAttribute(sym, ATTR_EXTERN)) {
                printf("Error on line %d, symbol %s is external\n", lineNumber, symbolname);
            }
            /*Calculate the difference and add it to the matching instruction*/
            diff = sym->address - IC;
            instructionImage[INDEXFROMIC(IC)] = instructionImage[INDEXFROMIC(IC)] | (diff & 0x0000FFFF);
        }
        /*Increment IC*/
        IC+=4;
        state = STATE_READING;
    }
    return 0;
}


unsigned char interpertLine(char* line) {
	char** splitAtSpace = NULL;
	char** splitAtComma = NULL;
	int splitAtCommaLength = 0;
	int splitAtSpaceLength = 0;
	char* labelName = NULL;
    char* originalLine;
	int dataType = 0;
	int opCode = 64;
    int type = 0;
    int funct;
	/*Check if line is a comment*/
	if(line[0] == ';') {
		return 0;
	}
	/*Check if line is only whitespace*/
	if(onlyWhitespace(line)) {
		return 1;
	}
    /*Trim the line, and save a copy of the orignal, because strtok (which is used in split string), breaks the string it's used on*/
    originalLine = trimLine(line);
    line = trimLine(originalLine);
    /*Split the line at every space*/
	splitString(line, " ", &splitAtSpace, &splitAtSpaceLength);
	if(state == STATE_READING) {
		if(splitAtSpaceLength > 0) {
            /*If we are at an extern instrction, try to add an extern. If it fails, exit the program*/
            if(strcmp(".extern", splitAtSpace[0]) == 0) {
                if(addExtern(&symboltable, splitAtSpace[1], 0) > 0) {
                    return 101;
                }
            }
            /*If the first word is a keyword for allocating data, get the data type, and set the state to DATA*/
			else if(isDataAllocation(splitAtSpace[0])) {
				dataType = isDataAllocation(splitAtSpace[0]);
				state =  STATE_DATA;
			}
            /*If the first word is a command, get the opCode, and set the state to CODE*/
            else if(getOpcodeFromName(splitAtSpace[0]) != 64) {
                opCode = getOpcodeFromName(splitAtSpace[0]);
                state = STATE_CODE;
            }
            /*If we are at an entry instruction, skip it*/
            else if(strcmp(".entry", splitAtSpace[0]) == 0) {
                return 0;
            }
            /*If the last character of the first word is ':', we are at a label*/
			else if(splitAtSpace[0][strlen(splitAtSpace[0])-1] == ':') {
                /*Get the label name without the ':' at the end*/
				labelName = (char*)calloc(strlen(splitAtSpace[0])-1, sizeof(char));
				memcpy(labelName, splitAtSpace[0], strlen(splitAtSpace[0])-1);
                /*Error out if there are not enough arguments*/
				if(splitAtSpaceLength < 2) {
					printf("Label %s at line %d undefined\n", labelName, lineNumber);
					return 101;
				}
                /*Error out if the label name is invalid*/
				if(!labelIsValid(labelName)) {
					printf("Invalid label name at line %d: %s\n", lineNumber, labelName);
					return 101;
				}
                /*Error out if the label already exists*/
				if(labelExists(symboltable, labelName)) {
					printf("Label %s at line %d already exists.\n", labelName, lineNumber);
					return 101;
				}
                /*Try to get the data type and opCode*/
				dataType = isDataAllocation(splitAtSpace[1]);
				opCode = getOpcodeFromName(splitAtSpace[1]);
                /*If the second word is a keyword for data allocation*/
				if(dataType) {
                    /*If the symbol doesn't exist in the symbol table, add it*/
                    if(getSymbol(symboltable, labelName) == NULL) {
                        Symboltable* sym;
					    initSymbol(&sym);
                        setName(sym, labelName);
                        setAddress(sym, DC);
                        addAttribute(sym, ATTR_DATA);
                        appendSymbol(&symboltable, sym);
                    }
                    /*If it does, add to it the attribute data*/
                    else {
                        Symboltable* sym = getSymbol(symboltable, labelName);
                        addAttribute(sym, ATTR_DATA);
                    }
                    /*set the state to data*/
					state = STATE_DATA;
				}
                /*If the first word returns a valid opCode*/
				else if(opCode != 64) {
                    /*If the symbol doesn't exist, add it to the symboltable*/
                    if(getSymbol(symboltable, labelName) == NULL) {
                        Symboltable* sym;
					    initSymbol(&sym);
					    setName(sym, labelName);
					    setAddress(sym, IC);
					    addAttribute(sym, ATTR_CODE);
					    appendSymbol(&symboltable, sym);
                    }
                    /*If it does, add to it the attibute code*/
                    else {
                        Symboltable* sym = getSymbol(symboltable, labelName);
                        addAttribute(sym, ATTR_CODE);
                    }
					state = STATE_CODE;
				}
			}
            /*If the line doesn't match any known pattern, error out*/
            else {
                printf("Error, line %d not understood\n", lineNumber);
                return 101;
            }
		}
		if(state == STATE_DATA) {
            /*If we are at a line with a label, all the indexs in splitAtSpace go up by one, so we need to check if labelName is defined*/
			if(labelName) {
                /*If the datatype is not .asciz, split the string at every comman, put it in an arry, and append it to the data image*/
                if(dataType != 4) {
                    splitString(splitAtSpace[2], ",", &splitAtComma, &splitAtCommaLength);
                    /*If there's a problem appending the data, Error out*/
    				if(allocateData(dataType, splitAtComma, splitAtCommaLength) != 0) {
                        return 101;
                    }
                }
                /*If it is, allocate whatever's in the quotes*/
                else {
                    /*If we are at asciz, send whatever's in the quotes to allocate data*/
                    char* firstQuote;
                    firstQuote = firstInstanceOf(originalLine, '\"');
                    if(allocateData(dataType, &firstQuote, 1) != 0) {
                        return 101;
                    }
                }
			}
            /*Do the same, but on splitAtSpace[1] instead, if labelName is not defined*/
			else {
                if(dataType != 4) {
                    splitString(splitAtSpace[1], ",", &splitAtComma, &splitAtCommaLength);
    				if(allocateData(dataType, splitAtComma, splitAtCommaLength) != 0) {
                        return 101;
                    }
                }
                else {
                    char* firstQuote;
                    firstQuote = firstInstanceOf(originalLine, '\"');
                    if(allocateData(dataType, &firstQuote, 1) != 0) {
                        return 101;
                    }
                }
			}
			state = STATE_READING;
			return 2;
		}
		if(state == STATE_CODE) {
            /*Based on if the label is defined or not, get the type and funct from the right index, and split at commas at the right index*/
			if(labelName) {
				type = getTypeFromName(splitAtSpace[1]);
				funct = getFunctFromName(splitAtSpace[1]);
				splitString(splitAtSpace[2], ",", &splitAtComma, &splitAtCommaLength);
			}
            else {
                type = getTypeFromName(splitAtSpace[0]);
                funct = getFunctFromName(splitAtSpace[0]);
                splitString(splitAtSpace[1], ",", &splitAtComma, &splitAtCommaLength);
            }
            /*Write the instruction to the instruction image, if there's an error, error out*/
            if(writeInstruction(splitAtComma, opCode, type, funct) == 101) {
                return 101;
            }
			state = STATE_READING;
		}
	}
	return 0;
}


/*
* A function that takes a data type, data list, and it's length, and allocates the data byte by byte.
* Note that the incrementing of DC is done by appendToData every time it's called
*/
unsigned char allocateData(int dataType, char** dataList, int dataListLength) {
	int value;
	char splitValue[4];
	int i;
	int j;
	int ret;
    /*If we are at a .db line*/
	if(dataType == 1) {
        /*Loop over the data list*/
		for(i = 0; i < dataListLength; i++) {
            /*If dataList[i] is not a number, return an error*/
			if((ret = isNumber(dataList[i])) == 0) {
				printf("Not a number %s in line %d\n", dataList[i], lineNumber);
				return 1;
			}
 			value = atoi(dataList[i]);
            /*Append value to data. If there's a problem doing that, return an error.*/
			if((ret = appendToData(&dataImage, value, &DC)) == 1) {
				printf("Memory allocation failure %s in line %d\n", dataList[i], lineNumber);
				return 1;
			}
		}
	}
	else if(dataType == 2) {
        /*If we are at a .dw*/
		for(i = 0; i < dataListLength; i++) {
            /*Check if it's a number*/
			if((ret = isNumber(dataList[i])) == 0) {
				printf("Not a number %s in line %d\n", dataList[i], lineNumber);
				return 1;
			}
            /*Get the value, and split it into four bytes*/
			value = atoi(dataList[i]);
			splitValue[0] =  value & 0x000000FF;
			splitValue[1] = (value & 0x0000FF00) >> 8;
			splitValue[2] = (value & 0x00FF0000) >> 16;
			splitValue[3] = (value & 0xFF000000) >> 24;
            /*Write the bytes to the data image, starting with the least significant*/
			for(j = 0; j < 4; j++) {
				if((ret = appendToData(&dataImage, splitValue[j], &DC)) == 1) {
					printf("Memory allocation failure %s in line %d\n", dataList[i], lineNumber);
					return 1;
				}
			}
		}
	}
	else if(dataType == 3) {
        /*If we are at a .dh, split the data into two bytes, and write them oen by one*/
		for(i = 0; i < dataListLength; i++) {
			if((ret = isNumber(dataList[i])) == 0) {
				printf("Not a number %s in line %d\n", dataList[i], lineNumber);
				return 1;
			}
			value = atoi(dataList[i]);
			splitValue[0] =  value & 0x000000FF;
			splitValue[1] = (value & 0x0000FF00) >> 8;
			for(j = 0; j < 2; j++) {
				if((ret = appendToData(&dataImage, splitValue[j], &DC)) == 1) {
					printf("Memory allocation failure %s in line %d\n", dataList[i], lineNumber);
					return 1;
				}
			}
		}
	}
	else if(dataType == 4) {
        /*If we are at a .asciz, make sure we have quotation marks around the data*/
		if(dataList[0][strlen(dataList[0])-2] != '"' || dataList[0][0] != '"') {
			printf("Error in line %d, missing quotation marks\n", lineNumber);
			return 1;
		}
        /*Store the length in j, loop over the string, and append each char to memory*/
		j = strlen(dataList[0]);
		for(i = 1; i < j-2; i++) {
			if((ret = appendToData(&dataImage, dataList[0][i], &DC)) == 1) {
				printf("Memory allocation failure %s in line %d\n", dataList[0], lineNumber);
				return 1;
			}
		}
        /*Append a null byte to memory*/
		if((ret = appendToData(&dataImage, 0, &DC)) == 1) {
			printf("Memory allocation failure %s in line %d\n", dataList[0], lineNumber);
			return 1;
		}
	}
	return 0;
}

/*A function that checks if a given string is a valid number*/
unsigned char isNumber(char* str) {
	int i;
	int stringlength = strlen(str);
	for(i = 0; i < stringlength; i++) {
        /*If we are at the end of the string and we have a new line character, ignore it*/
		if(i == stringlength - 1 && str[i] == '\n') {
			continue;
		}
        /*If we are at the start of the string, and we have either a number or a negative sign, continue, return 0 if it's anything else*/
		if(i == 0) {
			if((str[i] >= '0' && str[i] <= '9') || (str[i] == '-')) {
				continue;
			}
			return 0;
		}
        /*at any other point in the string, only accent numbers*/
		if((str[i] >= '0' && str[i] <= '9')) {
			continue;
		}
		return 0;
	}
	return 1;
}

/*A function that splits a string using some token 'split', and stores it in a string array, and it's size in an int*/
void splitString(char* str, char* split, char*** res, int* size) {
	char* token;
	int resSize;
	*res = (char**)calloc(0, sizeof(char*));
    /*get the first token*/
	token = strtok(str, split);
	resSize = 0;
    /*Loop until there are no more tokens*/
	while(token != NULL) {
        /*Increase the size of res each time*/
		*res = (char**)realloc(*res, ++resSize*sizeof(char*));
        /*Set the new allocated element to be the token*/
		(*res)[resSize-1] = token;
        /*get the next token*/
		token = strtok(NULL, split);
	}
    /*Set the size*/
	*size = resSize;
}

/*A function that checks if a line is purly white space, returns 1 if it is, zero otherwise*/
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

/*
*A function that encodes an instruction of type R
*/
int makeR(int opcode, int rs, int rt, int rd, int funct) {
	int res = 0;
    /*Make sure the register numbers are valid, return an invalid operation if not*/
	if((rs < 0 || rs > 31) || (rt < 0 || rt > 31) || (rd < 0 || rd > 31)) {
		printf("Error in line %d, register number invalid\n", lineNumber);
		return 0xFFFFFFFF;
	}
    /*Encode everything in it's right spot*/
	res = res | (funct << 6);
	res = res | (rd << 11);
	res = res | (rt << 16);
	res = res | (rs << 21);
	res = res | (opcode << 26);
	return res;
}

/*
*A function that encodes an instruction of type I
*/
int makeI(int opcode, int rs, int rt, short immed) {
	int res = 0;
    /*Make sure the register numbers are valid, return an invalid operation if not*/
	if((rs < 0 || rs > 31) || (rt < 0 || rt > 31)) {
		printf("Error in line %d, register number invalid\n", lineNumber);
		return 0xFFFFFFFF;
	}
    /*Encode everything in it's right spot, getting rid of the higher 16 bytes in immed incase it's a negative number*/
	res = res | immed;
	res = res & 0x0000FFFF;
	res = res | (rt << 16);
	res = res | (rs << 21);
	res = res | (opcode << 26);
	return res;
}

/*A function to make an instruction of type J*/
int makeJ(int opcode, int reg, int address) {
	int res = 0;
    /*Make sure the address is in rage*/
	if(address < 0 || address > pow(2,25)-1) {
		printf("Error in line %d, address value too large or too small\n", lineNumber);
	}
	res = res | address;
	res = res | (reg << 25);
	res = res | (opcode << 26);
	return res;
}

/*A function that takes in a string of the format "${N},", where {N} is some number, and returns the number as an int*/
int getRegisterNumber(char* str) {
	char buffer[2] = {0};
	int index = 0;
	int i = 0;
	int strlength = strlen(str);
    /*Loop over the string*/
	for(i = 0; i < strlength; i++) {

        /*If the current character is a number, add it to the buffer and increment index*/
		if(str[i] >= '0' && str[i] <= '9') {
			buffer[index++] = str[i];
		}
        /*If index becomes too large, error out*/
        if(index > 2) {
            return -1;
        }
	}
    /*If the string is emtpy, meaning there were no numbers, error out*/
    if(buffer[0] == '\0' && buffer[1] == '\0') {
        return -1;
    }
	return atoi(buffer);
}

/*A function that returns a pointer to the first instance of a char in a string*/
char* firstInstanceOf(char* str, char c) {
    int i = 0;
    int strlength = strlen(str);
    for(i = 0; i < strlength; i++) {
        if(str[i] == c) {
            return &str[i];
        }
    }
    return NULL;
}

/* A function that writes an instruction to the instruction image. It takes an array of arguments, the opcode, the type and the funct
* Note that appendToInstructions increments IC
*/
unsigned char writeInstruction(char** splitAtComma, int opCode, int type, int funct) {
    int rs;
    int rd;
    int rt;
    int immed;
    if(type == TYPER) {
        if(opCode == 0) {
            /*CGet all the needed values*/
            int inst;
            rs = getRegisterNumber(splitAtComma[0]);
            rt = getRegisterNumber(splitAtComma[1]);
            rd = getRegisterNumber(splitAtComma[2]);
            /*Put them in makeR, if there's an error, error out*/
            inst = makeR(opCode, rs, rt, rd, funct);
            if(inst == 0xFFFFFFFF) {
                return 101;
            }
            /*If there's no error, append the instruction to the image*/
            appendToInstructions(&instructionImage, inst, &IC);
        }
        else {
            int inst;
            /*If the opcode is one (it can only be one or zero, since the type is R)*/

            /* Note:
            * rs and rd are supposed to be splitAtComma[1] and splitAtComma[0], respectivly, based on the spesification in the pdf
            * but doing that gives a different result when encoding the line move $20,$4 in the example file from the pdf, "ps.as"
            * I've opted to follow the example
            */
            rt = 0;
            rs = getRegisterNumber(splitAtComma[0]);
            rd = getRegisterNumber(splitAtComma[1]);
            inst = makeR(opCode, rs, rt, rd, funct);
            if(inst == 0xFFFFFFFF) {
                return 101;
            }
            appendToInstructions(&instructionImage, inst, &IC);
        }
    }
    else if(type == TYPEI) {
        /*If we are at type I*/
        if(opCode >= 10 && opCode <= 14) {
            int inst;
            /*For instruction where the immed is a number and not a label, check that it's a number*/
            if(!isNumber(splitAtComma[1])) {
                printf("Error at line %d, immed not a number\n", lineNumber);
                return 101;
            }
            /*Get the values*/
            rs = getRegisterNumber(splitAtComma[0]);
            immed = (short)atoi(splitAtComma[1]);
            rt = getRegisterNumber(splitAtComma[2]);
            /*And append the instruction*/
            inst = makeI(opCode, rs, rt, immed);
            if(inst == 0xFFFFFFFF) {
                return 101;
            }
            appendToInstructions(&instructionImage, inst, &IC);
        }
        else if(opCode >= 15 && opCode <= 18) {
            int inst;
            /*In the case where immed is a label, leave it at zero, the proper value will be added in the sceond pass*/
            rs = getRegisterNumber(splitAtComma[0]);
            immed = 0;
            rt = getRegisterNumber(splitAtComma[1]);
            inst = makeI(opCode, rs, rt, immed);
            if(inst == 0xFFFFFFFF) {
                return 101;
            }
            appendToInstructions(&instructionImage, inst, &IC);
        }
        else {

            int inst;
            /*Exactly same as the first case, but I am afraid to touch this*/
            if(!isNumber(splitAtComma[1])) {
                printf("Error at line %d, immed not a number\n", lineNumber);
                return 101;
            }
            rs = getRegisterNumber(splitAtComma[0]);

            immed = (short)atoi(splitAtComma[1]);
            rt = getRegisterNumber(splitAtComma[2]);
            inst = makeI(opCode, rs, rt, immed);
            if(inst == 0xFFFFFFFF) {
                return 101;
            }
            appendToInstructions(&instructionImage, inst, &IC);
        }
    }
    else if(type == TYPEJ) {
        if(opCode == 30) {
            /*If it's a jmp instruction, check if it's using a register. If it is, set reg to one and put the register number in the address*/
            if(splitAtComma[0][0] == '$') {
                appendToInstructions(&instructionImage, makeJ(opCode, 1, getRegisterNumber(splitAtComma[0])), &IC);
            }
            /*Set reg to zero and leave the address blank, otherwise*/
            else {
                appendToInstructions(&instructionImage, makeJ(opCode, 0, 0), &IC);
            }
        }
        else if(opCode == 31 || opCode == 32 || opCode == 63) {
            /*All the other instructions of type J have a reg of 0, and need their address filled in the second pass*/
            appendToInstructions(&instructionImage, makeJ(opCode, 0, 0), &IC);
        }
    }
    return 0;
}

/*A function that adds extern to any given symbol table (to be used on either the externalList, or symboltable)*/
unsigned char addExtern(Symboltable** table, char* labelname, int addr) {
    Symboltable* sym;
    /*Trim away the newline character, if there is one*/
    if(labelname[strlen(labelname)-1] == '\n'){
        char* name;
        name = (char*)calloc(strlen(labelname)-1, sizeof(char));
        memcpy(name, labelname, strlen(labelname)-1);
        labelname = name;
    }
    /*
    * If the label exists, and the supplied address was zero (which means we are appending to the symboltable and not the externalList), error out
    */
    if(labelExists(*table, labelname) && addr == 0) {
        printf("Error in line %d, label %s already exists\n", lineNumber, labelname);
        return 1;
    }
    /*Append the symbol to the given symbol table, adding the the external attribute if it's in the global symboltable*/
    initSymbol(&sym);
    setName(sym, labelname);
    setAddress(sym, addr);
    if(addr == 0) {
        addAttribute(sym, ATTR_EXTERN);
    }
    appendSymbol(table, sym);
    return 0;
}

/*A function that adds the final value of IC to all things in the symboltable marked as data*/
void addICF() {
    Symboltable* c = symboltable;
    while(c != NULL) {
        if(hasAttribute(c, ATTR_DATA)) {
            c->address += IC;
        }
        c = c->next;
    }
}

/*A function that takes a file pointer, and writes to it the contents for the entry file*/
void writeEntryFile(FILE* output) {
    Symboltable* current = symboltable;
    char buffer[10];
    /*Loop over the symbol table*/
    while(current != NULL) {
        /*If the current symbol is an entry*/
        if(hasAttribute(current, ATTR_ENTRY)) {
            /*Write the symbol's name, a space, and then its address, aligned to be four characters in length with a zero if needed*/
            fwrite(current->name, sizeof(char), strlen(current->name), output);
            fputc(' ', output);
            sprintf(buffer, "%04d", current->address);
            fwrite(buffer, sizeof(char), strlen(buffer), output);
            fputc('\n', output);
        }
        current = current->next;
    }
}

/*A function that takes a file pointer, and writes to it the contents for the external file*/
void writeExtFile(FILE* output) {
    Symboltable* current = externalList;
    char buffer[10];
    /*Loop over the external table, this function operates like writeEntryFile*/
    while(current != NULL) {
        fwrite(current->name, sizeof(char), strlen(current->name), output);
        fputc(' ', output);
        sprintf(buffer, "%04d", current->address);
        fwrite(buffer, sizeof(char), strlen(buffer), output);
        fputc('\n', output);
        current = current->next;
    }
}

/*A function that takes a file pointer and writes the contents of the object file to it*/
void writeObjectFile(FILE* output) {
    int i;
    int j;
    char buffer[20];
    IC = 100;
    /*Write ICF-100 and DCF to the output file*/
    sprintf(buffer, "%d", ICF-100);
    fwrite(&buffer, sizeof(char), strlen(buffer), output);
    fputc(' ', output);
    sprintf(buffer, "%d", DCF);
    fwrite(&buffer, sizeof(char), strlen(buffer), output);
    fputc('\n', output);
    /*Loop over the instructions*/
    for(i = 0; i < INDEXFROMIC(ICF); i++) {
        /*Write the approprite IC value*/
        sprintf(buffer, "%04d", IC);
        fwrite(&buffer, sizeof(char), strlen(buffer), output);
        fputc(' ', output);
        /*Then, write the instruction byte by byte, starting with the least significant byte*/
        sprintf(buffer, "%02x", instructionImage[INDEXFROMIC(IC)] & 0x000000FF);
        toUpperStr(buffer);
        fwrite(&buffer, sizeof(char), strlen(buffer), output);
        fputc(' ', output);
        sprintf(buffer, "%02x", (instructionImage[INDEXFROMIC(IC)] & 0x0000FF00) >> 8);
        toUpperStr(buffer);
        fwrite(&buffer, sizeof(char), strlen(buffer), output);
        fputc(' ', output);
        sprintf(buffer, "%02x", (instructionImage[INDEXFROMIC(IC)] & 0x00FF0000) >> 16);
        toUpperStr(buffer);
        fwrite(&buffer, sizeof(char), strlen(buffer), output);
        fputc(' ', output);
        sprintf(buffer, "%02x", (instructionImage[INDEXFROMIC(IC)] & 0xFF000000) >> 24);
        toUpperStr(buffer);
        fwrite(&buffer, sizeof(char), strlen(buffer), output);
        fputc('\n', output);
        /*Increment IC*/
        IC += 4;
    }
    i = 0;
    /*Loop over the data image*/
    while(i < DCF) {
        /*Write IC*/
        sprintf(buffer, "%04d", IC);
        fwrite(&buffer, sizeof(char), strlen(buffer), output);
        fputc(' ', output);
        /*Then write four bytes to the file*/
        for(j = 0; j < 4 && i < DCF; j++) {
            sprintf(buffer, "%02x", (unsigned char)dataImage[i++]);
            toUpperStr(buffer);
            fwrite(&buffer, sizeof(char), strlen(buffer), output);
            fputc(' ', output);
        }
        /*Start a new line*/
        fputc('\n', output);
        IC += 4;
    }
}

/*A function that loops over a string and makes every character upper case*/
void toUpperStr(char* str) {
    int i;
    int strlength = strlen(str);
    for(i = 0; i < strlength; i++) {
        str[i] = toupper(str[i]);
    }
}

/*A function that takes a file name ending in .as, and returns a file with the same name ending in .ob*/
char* getObName(char* inputName) {
    char* ret = (char*)calloc(strlen(inputName), sizeof(char));
    memcpy(ret, inputName, strlen(inputName)-2);
    strcat(ret, "ob");
    return ret;
}

/*A function that takes a file name ending in .as, and returns a file with the same name ending in .ent*/
char* getEntName(char* inputName) {
    char* ret = (char*)calloc(strlen(inputName)+1, sizeof(char));
    memcpy(ret, inputName, strlen(inputName)-2);
    strcat(ret, "ent");
    return ret;
}

/*A function that takes a file name ending in .as, and returns a file with the same name ending in .ext*/
char* getExtName(char* inputName) {
    char* ret = (char*)calloc(strlen(inputName)+1, sizeof(char));
    memcpy(ret, inputName, strlen(inputName)-2);
    strcat(ret, "ext");
    return ret;
}

/*A function that checks if a file ends in .as*/
unsigned char hasDotas(char* name) {
    int len = strlen(name);
    return (name[len-3] == '.' && name[len-2] == 'a' && name[len-1] == 's');
}

/*A function that gets rid of white space at the start of a line, as well as white space between arguments seperated by commans*/
char* trimLine(char* line) {
    int i;
    int resIndex = 0;
    int len = strlen(line);
    char start = 1;
    char inComma = 0;
    char* res = (char*)calloc(len, sizeof(char));
    for(i = 0; i < len; i++) {
        if((start || inComma) && (line[i] == ' ' || line[i] == '\t')) {
            continue;
        }
        else if(start && !(line[i] == ' ' || line[i] == '\t')) {
            start = 0;
        }
        else if(line[i] == ',') {
            inComma = 1;
        }
        else if(inComma && (line[i] != ' ' && line[i] != '\t')) {
            inComma = 0;
        }
        res[resIndex++] = line[i];
    }

    return res;
}
