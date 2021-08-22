#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "symboltable.h"

void initSymbol(Symboltable** symbol) {
    (*symbol) = (Symboltable*)calloc(1, sizeof(Symboltable));
    (*symbol)->name = NULL;
    (*symbol)->attributes = NULL;
    (*symbol)->attributeCount = 0;
    (*symbol)->address = 0;
}

Symboltable** getLastSymbol(Symboltable** head) {
    Symboltable** current = head;
	while((*current) != NULL) {
		current = &((*current)->next);
	}
	return current;
}

unsigned char setAddress(Symboltable* symbol, int address) {
    if(address <= pow(2, 25)-1 && address >= 0) {
        symbol->address = address;
        return 0;
    }
    printf("setAddress: address %d too large or too small\n", address);
    return 2;
}

void setName(Symboltable* symbol, char* name) {
    symbol->name = name;
}

unsigned char addAttribute(Symboltable* symbol, int attribute) {
    int* temp = realloc(symbol->attributes, ++(symbol->attributeCount)*sizeof(int));
    if(temp) {
        symbol->attributes = temp;
        symbol->attributes[symbol->attributeCount-1] = attribute;
        return 0;
    }
    printf("addAttribute: Memory allocation falied\n");
    return 1;
}

unsigned char hasAttribute(Symboltable* symbol, int attribute) {
    int i;
    for(i=0; i < symbol->attributeCount; i++) {
        if(symbol->attributes[i] == attribute) {
            return 1;
        }
    }
    return 0;
}

unsigned char labelExists(Symboltable* symbol, char* label) {
    Symboltable* current = symbol;
    while(current != NULL) {
        if(strcmp(current->name, label) == 0) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}

void appendSymbol(Symboltable** head, Symboltable* new) {
    Symboltable** last = getLastSymbol(head);
    *last = new;
    (*last)->next = NULL;
}

unsigned char labelIsValid(char* label) {
    int i;
    int labelLength;
    labelLength = strlen(label);
    for(i = 0; i < labelLength; i++) {
        char current = label[i];
        if(!((current >= 'A' && current <= 'Z') || (current >= 'a' && current <= 'z') || (current >= '0' && current <= '9'))) {
            return 0;
        }
    }
    return 1;
}

void printSymboltable(Symboltable* head) {
    if(head != NULL) {
        printf("Symbol %s:\n", head->name);
        printf("\taddress: %d\n", head->address);
        printf("\tattribute %d\n", head->attributes[0]);
        printSymboltable(head->next);
    }
}
