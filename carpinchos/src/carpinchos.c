/*
 ============================================================================
 Name        : carpinchos.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <matelib/matelib.h>

int main(void) {
	mate_instance* lib = malloc(sizeof(mate_instance));
	mate_init(lib, CONFIG_PATH);
	mate_printf();


	return EXIT_SUCCESS;
}
