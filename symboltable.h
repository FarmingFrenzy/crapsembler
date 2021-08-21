#define ATTR_CODE    0
#define ATTR_DATA    1
#define ATTR_ENTRY   2
#define ATTR_EXTERN  3

typedef struct symboltable {
    char* name;
    int address;
    int* attributes;
    int attributeCount;
    struct symboltable* next;
} Symboltable;

unsigned char labelExists(Symboltable*, char*);
unsigned char addAttribute(Symboltable*, int);
unsigned char setAddress(Symboltable*, int);
unsigned char hasAttribute(Symboltable*, int);
unsigned char labelIsValid(char*);
Symboltable** getLastSymbol(Symboltable**);
void setName(Symboltable*, char*);
void initSymbol(Symboltable**);
void appendSymbol(Symboltable**, Symboltable*);
void printSymboltable(Symboltable*);
