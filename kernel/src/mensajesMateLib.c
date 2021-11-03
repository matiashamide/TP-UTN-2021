/*
 * mensajesMateLib.c
 *
 *  Created on: 3 nov. 2021
 *      Author: utnso
 */

#include "mensajesMateLib.h"


void dar_permiso_para_continuar(int conexion) {

	char* mensaje = malloc(sizeof(9));
	mensaje = "Continua";

	enviar_mensaje(mensaje, conexion);
}


void init_sem(PCB* pcb) {
	t_semaforo_mate* nuevo_semaforo = malloc(sizeof(t_semaforo_mate));

	int size;
	int size_nombre_semaforo;

	recv(pcb->conexion, &size, sizeof(int), MSG_WAITALL);

	recv(pcb->conexion, &size_nombre_semaforo, sizeof(int), MSG_WAITALL);

	nuevo_semaforo->nombre = malloc(size_nombre_semaforo);

	recv(pcb->conexion, nuevo_semaforo->nombre, size_nombre_semaforo, MSG_WAITALL);

	recv(pcb->conexion, &nuevo_semaforo->value, sizeof(int), MSG_WAITALL);

	pthread_mutex_lock(&mutex_lista_semaforos_mate);
	list_add(LISTA_SEMAFOROS_MATE, nuevo_semaforo);
	pthread_mutex_unlock(&mutex_lista_semaforos_mate);

	dar_permiso_para_continuar(pcb->conexion);
}


void wait_sem(PCB* pcb) {

}

void post_sem(PCB* pcb) {

}

void destroy_sem(PCB* pcb) {

}

void call_IO(PCB* pcb) {

}

void memalloc(PCB* pcb) {

}

void memread(PCB* pcb) {

}

void memfree(PCB* pcb) {

}

void memwrite(PCB* pcb) {

}


