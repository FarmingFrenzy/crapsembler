#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "commands.h"

/*These functions are used to identify commands, their opcode, type and funct*/

unsigned char getOpcodeFromName(char* name) {
	if(strcmp("add", name) == 0) {
		return 0;
	}
	if(strcmp("sub", name) == 0) {
		return 0;
	}
	if(strcmp("and", name) == 0) {
		return 0;
	}
	if(strcmp("or", name) == 0) {
		return 0;
	}
	if(strcmp("nor", name) == 0) {
		return 0;
	}
	if(strcmp("move", name) == 0) {
		return 1;
	}
	if(strcmp("mvhi", name) == 0) {
		return 1;
	}
	if(strcmp("mvlo", name) == 0) {
		return 1;
	}
	if(strcmp("addi", name) == 0) {
		return 10;
	}
	if(strcmp("subi", name) == 0) {
		return 11;
	}
	if(strcmp("andi", name) == 0) {
		return 12;
	}
	if(strcmp("ori", name) == 0) {
		return 13;
	}
	if(strcmp("nori", name) == 0) {
		return 14;
	}
	if(strcmp("bne", name) == 0) {
		return 15;
	}
	if(strcmp("beq", name) == 0) {
		return 16;
	}
	if(strcmp("blt", name) == 0) {
		return 17;
	}
	if(strcmp("bgt", name) == 0) {
		return 18;
	}
	if(strcmp("lb", name) == 0) {
		return 19;
	}
	if(strcmp("sb", name) == 0) {
		return 20;
	}
	if(strcmp("lw", name) == 0) {
		return 21;
	}
	if(strcmp("sw", name) == 0) {
		return 22;
	}
	if(strcmp("lh", name) == 0) {
		return 23;
	}
	if(strcmp("sh", name) == 0) {
		return 24;
	}
	if(strcmp("jmp", name) == 0) {
		return 30;
	}
	if(strcmp("la", name) == 0) {
		return 31;
	}
	if(strcmp("call", name) == 0) {
		return 32;
	}
	if(strcmp("stop", name) == 0 || strcmp("stop\n", name) == 0) {
		return 63;
	}
	return 64;
}

unsigned char getTypeFromOpcode(int opcode) {
	if(opcode == 1 || opcode == 0) {
		return TYPER;
	}
	if(opcode >= 10 || opcode <= 24) {
		return TYPEI;
	}
	if(opcode == 30 || opcode == 31 || opcode == 32 || opcode == 63) {
		return TYPEJ;
	}
	return 3;
}

unsigned char getTypeFromName(char* name) {
	/*Type R*/
	if(strcmp("add", name) == 0) {
		return TYPER;
	}
	if(strcmp("sub", name) == 0) {
		return TYPER;
	}
	if(strcmp("and", name) == 0) {
		return TYPER;
	}
	if(strcmp("or", name) == 0) {
		return TYPER;
	}
	if(strcmp("nor", name) == 0) {
		return TYPER;
	}
	if(strcmp("move", name) == 0) {
		return TYPER;
	}
	if(strcmp("mvhi", name) == 0) {
		return TYPER;
	}
	if(strcmp("mvlo", name) == 0) {
		return TYPER;
	}

	/*Type I*/
	if(strcmp("addi", name) == 0) {
		return TYPEI;
	}
	if(strcmp("subi", name) == 0) {
		return TYPEI;
	}
	if(strcmp("andi", name) == 0) {
		return TYPEI;
	}
	if(strcmp("ori", name) == 0) {
		return TYPEI;
	}
	if(strcmp("nori", name) == 0) {
		return TYPEI;
	}
	if(strcmp("bne", name) == 0) {
		return TYPEI;
	}
	if(strcmp("beq", name) == 0) {
		return TYPEI;
	}
	if(strcmp("blt", name) == 0) {
		return TYPEI;
	}
	if(strcmp("bgt", name) == 0) {
		return TYPEI;
	}
	if(strcmp("lb", name) == 0) {
		return TYPEI;
	}
	if(strcmp("sb", name) == 0) {
		return TYPEI;
	}
	if(strcmp("lw", name) == 0) {
		return TYPEI;
	}
	if(strcmp("sw", name) == 0) {
		return TYPEI;
	}
	if(strcmp("lh", name) == 0) {
		return TYPEI;
	}
	if(strcmp("sh", name) == 0) {
		return TYPEI;
	}

	/*Type J*/
	if(strcmp("jmp", name) == 0) {
		return TYPEJ;
	}
	if(strcmp("la", name) == 0) {
		return TYPEJ;
	}
	if(strcmp("call", name) == 0) {
		return TYPEJ;
	}
	if(strcmp("stop", name) == 0 || strcmp("stop\n", name) == 0) {
		return TYPEJ;
	}
	return TYPEERROR;
}

unsigned char getFunctFromName(char* name) {
	if(strcmp("add", name) == 0) {
		return 1;
	}
	if(strcmp("sub", name) == 0) {
		return 2;
	}
	if(strcmp("and", name) == 0) {
		return 3;
	}
	if(strcmp("or", name) == 0) {
		return 4;
	}
	if(strcmp("nor", name) == 0) {
		return 5;
	}
	if(strcmp("move", name) == 0) {
		return 1;
	}
	if(strcmp("mvhi", name) == 0) {
		return 2;
	}
	if(strcmp("mvlo", name) == 0) {
		return 3;
	}
	return 10;
}
