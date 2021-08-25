#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "symboltable.h"

/*A linked list that will be used for the symbol table and the list of external labels*/

/*A function that takes a symbol table pointer pointer, and initilizes it*/
void initSymbol(Symboltable** symbol) {
    (*symbol) = (Symboltable*)calloc(1, sizeof(Symboltable));
    (*symbol)->name = NULL;
    (*symbol)->attributes = NULL;
    (*symbol)->attributeCount = 0;
    (*symbol)->address = 0;
}

/*A function that takes a symboltable pointer pointer, and interates over until it reaches the last one*/
Symboltable** getLastSymbol(Symboltable** head) {
    Symboltable** current = head;
	while((*current) != NULL) {
		current = &((*current)->next);
	}
	return current;
}

/*A function that stes the address of a symbol, making sure the given address is valid*/
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

/*A function that adds an attribute to the array of attributes of a symbol*/
unsigned char addAttribute(Symboltable* symbol, int attribute) {
    /*Increase the size of the attribute array*/
    int* temp = realloc(symbol->attributes, ++(symbol->attributeCount)*sizeof(int));
    /*If the allocation was successful, add the new attribute*/
    if(temp) {
        symbol->attributes = temp;
        symbol->attributes[symbol->attributeCount-1] = attribute;
        return 0;
    }
    printf("addAttribute: Memory allocation falied\n");
    return 1;
}

/*A function that takes a symbol and an attribute, and checks if the symbol has that attribute*/
unsigned char hasAttribute(Symboltable* sym, int attr) {
    int i;
    for(i = 0; i < sym->attributeCount; i++) {
        if(attr == sym->attributes[i]) {
            return 1;
        }
    }
    return 0;
}

/*A function that loops over a symbol table, and checks if a given label already exists in it*/
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

/*A function that takes a symbol table, and returns a symbol with a given label*/
Symboltable* getSymbol(Symboltable* symbol, char* label) {
    Symboltable* current = symbol;
    char* name;
    /*Get rid of a newline character, if there is one*/
    if(label[strlen(label)-1] == '\n'){
        name = (char*)calloc(strlen(label)-1, sizeof(char));
        memcpy(name, label, strlen(label)-1);
        label = name;
    }
    /*Loop over and search for the label in the table*/
    while(current != NULL) {
        if(strcmp(current->name, label) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/*A function that appends a symbol to a symbol table*/
void appendSymbol(Symboltable** head, Symboltable* new) {
    Symboltable** last = getLastSymbol(head);
    *last = new;
    (*last)->next = NULL;
}

/*A function that ensures a label is valid (has only numbers and letters)*/
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
        if(head->attributeCount > 0) {
            printf("\tattribute %d\n", head->attributes[0]);
        }
        if(head->attributeCount > 1) {
            printf("\tattribute %d\n", head->attributes[1]);
        }
        printSymboltable(head->next);
    }
}
