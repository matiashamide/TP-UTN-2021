/*
 ============================================================================
 Name        : carpinchoJose.c
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

	mate_instance mate_ref;
	mate_init(&mate_ref, "/home/utnso/workspace/tp-2021-2c-DesacatadOS/matelib/matelib.config");
	mate_sem_init(&mate_ref, "SEM", 1);
	//mate_sem_wait(&mate_ref, "SEM");
	mate_printf();
	mate_close(&mate_ref);

	return EXIT_SUCCESS;
}
