#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
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
void splitString(char*, char*, char***, int*);
void addICF();
int makeR(int, int, int, int, int);
int makeI(int, int, int, short);
int makeJ(int, int, int);
int getRegisterNumber(char*);
char* firstInstanceOf(char*, char);

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

void printBits(size_t const size, void const * const ptr)
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
    addICF();
    /*Second pass*/
    printf("*******SECOND PASS********\n");
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
    printf("done\n");
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
    printf("reading line %s", line);
    originalLine = (char*)calloc(strlen(line), sizeof(char));
    strcpy(originalLine, line);
    splitString(line, " ", &splitAtSpace, &splitAtSpaceLength);
    if(!(splitAtSpaceLength > 0)) {
        return 101;
    }
    if(strcmp(splitAtSpace[0], ".entry") == 0) {
        sym = getSymbol(symboltable, splitAtSpace[1]);
        if(!sym) {
            printf("Error on line %d, symbol %s doesn't exist\n", lineNumber, splitAtSpace[1]);
            return 1;
        }
        addAttribute(sym, ATTR_ENTRY);
    }
    else if((opCode = getOpcodeFromName(splitAtSpace[0])) != 64) {
        type = getTypeFromName(splitAtSpace[0]);
        state = STATE_CODE;
    }
    else if(splitAtSpaceLength > 2 && (opCode = getOpcodeFromName(splitAtSpace[1])) != 64) {
        type = getTypeFromName(splitAtSpace[1]);
        state = STATE_CODE;
    }
    if(state == STATE_CODE) {
        if(type == TYPEJ && opCode != 63 && !hasReg(instructionImage[INDEXFROMIC(IC)])) {
            if(splitAtSpaceLength > 2) {
                symbolname = splitAtSpace[2];
            }
            else {
                symbolname = splitAtSpace[1];
            }
            if(symbolname[strlen(symbolname)-1] == '\n'){
                char* name;
                name = (char*)calloc(strlen(symbolname)-1, sizeof(char));
                memcpy(name, symbolname, strlen(symbolname)-1);
                symbolname = name;
            }
            sym = getSymbol(symboltable, symbolname);
            if(sym == NULL) {
                printf("Error on line %d, symbol doesn't exist %s", lineNumber, symbolname);
                return 101;
            }
            if(hasAttribute(sym, ATTR_EXTERN)) {
                if(addExtern(&externalList, symbolname, IC) > 0) {
                    return 101;
                }
            }
            else {
                instructionImage[INDEXFROMIC(IC)] = instructionImage[INDEXFROMIC(IC)] | sym->address;
                printf("new:\t  ");
                printBits(sizeof(int), &instructionImage[INDEXFROMIC(IC)]);
            }
        }
        else if(opCode >= 15 && opCode <= 18) {
            if(splitAtSpaceLength > 2) {
                symbolname = splitAtSpace[2];
            }
            else {
                symbolname = splitAtSpace[1];
            }
            if(symbolname[strlen(symbolname)-1] == '\n'){
                char* name;
                name = (char*)calloc(strlen(symbolname)-1, sizeof(char));
                memcpy(name, symbolname, strlen(symbolname)-1);
                symbolname = name;
            }
            splitString(symbolname, ",", &splitAtComma, &splitAtCommaLength);
            symbolname = splitAtComma[2];
            sym = getSymbol(symboltable, symbolname);
            if(!sym) {
                printf("Error on line %d, symbol doesn't exist %s", lineNumber, symbolname);
                return 101;
            }
            diff = sym->address - IC;
            instructionImage[INDEXFROMIC(IC)] = instructionImage[INDEXFROMIC(IC)] | (diff & 0x0000FFFF);
            printf("new:\t  ");
            printBits(sizeof(int), &instructionImage[INDEXFROMIC(IC)]);
        }
        IC+=4;
        state = STATE_READING;
    }
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
    originalLine = (char*)calloc(strlen(line), sizeof(char));
    strcpy(originalLine, line);
	printf("reading line %s", line);
	splitString(line, " ", &splitAtSpace, &splitAtSpaceLength);
	if(state == STATE_READING) {
		/*TODO: trim input*/
		if(splitAtSpaceLength > 0) {
            if(strcmp(".extern", splitAtSpace[0]) == 0) {
                if(addExtern(&symboltable, splitAtSpace[1], 0) > 0) {
                    return 101;
                }
            }
			else if(isDataAllocation(splitAtSpace[0])) {
				dataType = isDataAllocation(splitAtSpace[0]);
				state =  STATE_DATA;
			}
            else if(getOpcodeFromName(splitAtSpace[0]) != 64) {
                opCode = getOpcodeFromName(splitAtSpace[0]);
                state = STATE_CODE;
            }
			else if(splitAtSpace[0][strlen(splitAtSpace[0])-1] == ':') {
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
				opCode = getOpcodeFromName(splitAtSpace[1]);
				if(dataType) {
                    if(getSymbol(symboltable, labelName) == NULL) {
                        Symboltable* sym;
					    initSymbol(&sym);
                        setName(sym, labelName);
                        setAddress(sym, DC);
                        addAttribute(sym, ATTR_DATA);
                        appendSymbol(&symboltable, sym);
                    }
                    else {
                        Symboltable* sym = getSymbol(symboltable, labelName);
                        addAttribute(sym, ATTR_DATA);
                    }
					state = STATE_DATA;
				}
				else if(opCode != 64) {
                    if(getSymbol(symboltable, labelName) == NULL) {
                        Symboltable* sym;
					    initSymbol(&sym);
					    setName(sym, labelName);
					    setAddress(sym, IC);
					    addAttribute(sym, ATTR_CODE);
					    appendSymbol(&symboltable, sym);
                    }
                    else {
                        Symboltable* sym = getSymbol(symboltable, labelName);
                        addAttribute(sym, ATTR_CODE);
                    }
					state = STATE_CODE;
				}
			}
		}
		if(state == STATE_DATA) {
			if(labelName) {
                if(dataType != 4) {
                    splitString(splitAtSpace[2], ",", &splitAtComma, &splitAtCommaLength);
    				allocateData(dataType, splitAtComma, splitAtCommaLength);
                }
                else {
                    char* firstQuote;
                    firstQuote = firstInstanceOf(originalLine, '\"');
                    allocateData(dataType, &firstQuote, 1);
                }
			}
			else {
                if(dataType != 4) {
                    splitString(splitAtSpace[1], ",", &splitAtComma, &splitAtCommaLength);
    				allocateData(dataType, splitAtComma, splitAtCommaLength);
                }
                else {
                    char* firstQuote;
                    firstQuote = firstInstanceOf(originalLine, '\"');
                    allocateData(dataType, &firstQuote, 1);
                }
			}
			state = STATE_READING;
			return 2;
		}
		if(state == STATE_CODE) {
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
            writeInstruction(splitAtComma, opCode, type, funct);
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

/*
*31-26: opcode
*25-21: rs
*20-16: rt
*15-11: rd
*10-6:  funct
*5-0:   N/A
*/
int makeR(int opcode, int rs, int rt, int rd, int funct) {
	int res = 0;
	if((rs < 0 || rs > 31) || (rt < 0 || rt > 31) || (rd < 0 || rd > 31)) {
		printf("Error in line %d, register number invalid\n", lineNumber);
		return 0xFFFFFFFF;
	}
	res = res | (funct << 6);
	res = res | (rd << 11);
	res = res | (rt << 16);
	res = res | (rs << 21);
	res = res | (opcode << 26);
	return res;
}

int makeI(int opcode, int rs, int rt, short immed) {
	int res = 0;
	if((rs < 0 || rs > 31) || (rt < 0 || rt > 31)) {
		printf("Error in line %d, register number invalid\n", lineNumber);
		return 0xFFFFFFFF;
	}
	if(immed > SHRT_MAX || immed < SHRT_MIN) {
		printf("Error in line %d\n, value too large or too small\n", lineNumber);
	}
	res = res | immed;
	res = res & 0x0000FFFF;
	res = res | (rt << 16);
	res = res | (rs << 21);
	res = res | (opcode << 26);
	return res;
}

int makeJ(int opcode, int reg, int address) {
	int res = 0;
	if(address < 0 || address > pow(2,25)-1) {
		printf("Error in line %d, address value too large or too small\n", lineNumber);
	}
	res = res | address;
	res = res | (reg << 25);
	res = res | (opcode << 26);
	return res;
}

int getRegisterNumber(char* str) {
	char buffer[2];
	int index = 0;
	int i = 0;
	int strlength = strlen(str);
	for(i = 0; i < strlength; i++) {
		if(str[i] >= '0' && str[i] <= '9') {
			buffer[index++] = str[i];
		}
	}
	return atoi(buffer);
}

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

unsigned char writeInstruction(char** splitAtComma, int opCode, int type, int funct) {
    int rs;
    int rd;
    int rt;
    int immed;
    if(type == TYPER) {
        if(opCode == 0) {
            rs = getRegisterNumber(splitAtComma[0]);
            rt = getRegisterNumber(splitAtComma[1]);
            rd = getRegisterNumber(splitAtComma[2]);
            appendToInstructions(&instructionImage, makeR(opCode, rs, rt, rd, funct), &IC);
        }
        else {
            rs = getRegisterNumber(splitAtComma[1]);
            rd = getRegisterNumber(splitAtComma[0]);
            appendToInstructions(&instructionImage, makeR(opCode, rs, 0, rd, funct), &IC);
        }
    }
    /*int makeI(int opcode, int rs, int rt, int immed)*/
    else if(type == TYPEI) {
        if(opCode >= 10 && opCode <= 14) {
            if(!isNumber(splitAtComma[1])) {
                printf("Error at line %d, immed not a number\n", lineNumber);
                return 101;
            }
            rs = getRegisterNumber(splitAtComma[0]);
            immed = (short)atoi(splitAtComma[1]);
            rt = getRegisterNumber(splitAtComma[2]);
            appendToInstructions(&instructionImage, makeI(opCode, rs, rt, immed), &IC);
        }
        else if(opCode >= 15 && opCode <= 18) {
            rs = getRegisterNumber(splitAtComma[0]);
            immed = 0;
            rt = getRegisterNumber(splitAtComma[1]);
            appendToInstructions(&instructionImage, makeI(opCode, rs, rt, immed), &IC);
        }
        else {
            if(!isNumber(splitAtComma[1])) {
                printf("Error at line %d, immed not a number\n", lineNumber);
                return 101;
            }
            rs = getRegisterNumber(splitAtComma[0]);
            immed = (short)atoi(splitAtComma[1]);
            rt = getRegisterNumber(splitAtComma[2]);
            appendToInstructions(&instructionImage, makeI(opCode, rs, rt, immed), &IC);
        }
    }
    /*int makeJ(int opcode, int reg, int address)*/
    else if(type == TYPEJ) {
        if(opCode == 30) {
            if(splitAtComma[0][0] == '$') {
                appendToInstructions(&instructionImage, makeJ(opCode, 1, getRegisterNumber(splitAtComma[0])), &IC);
            }
            else {
                appendToInstructions(&instructionImage, makeJ(opCode, 0, 0), &IC);
            }
        }
        else if(opCode == 31 || opCode == 32 || opCode == 63) {
            appendToInstructions(&instructionImage, makeJ(opCode, 0, 0), &IC);
        }
    }
    return 0;
}

unsigned char addExtern(Symboltable** table, char* labelname, int addr) {
    Symboltable* sym;
    if(labelname[strlen(labelname)-1] == '\n'){
        char* name;
        name = (char*)calloc(strlen(labelname)-1, sizeof(char));
        memcpy(name, labelname, strlen(labelname)-1);
        labelname = name;
    }
    if(labelExists(*table, labelname) && addr == 0) {
        printf("Error in line %d, label %s already exists\n", lineNumber, labelname);
        return 1;
    }
    initSymbol(&sym);
    setName(sym, labelname);
    setAddress(sym, addr);
    if(addr == 0) {
        addAttribute(sym, ATTR_EXTERN);
    }
    appendSymbol(table, sym);
    return 0;
}

void addICF() {
    Symboltable* c = symboltable;
    while(c != NULL) {
        if(c->attributes[0] == ATTR_DATA || c->attributes[1] == ATTR_DATA) {
            c->address += IC;
        }
        c = c->next;
    }
}
