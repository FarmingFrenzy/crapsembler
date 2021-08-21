#define TYPER 0
#define TYPEI 1
#define TYPEJ 2
#define TYPEERROR 3

unsigned char getOpcodeFromName(char*);
unsigned char getFunctFromName(char*);
unsigned char getTypeFromName(char*);
unsigned char getTypeFromOpcode(int);
