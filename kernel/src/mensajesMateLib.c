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

// TODO manejar error de crear un semaforo con el mismo nombre que uno ya existente
void init_sem(PCB* pcb) {
	t_semaforo_mate* nuevo_semaforo = malloc(sizeof(t_semaforo_mate));

	int size;
	int size_nombre_semaforo;

	recv(pcb->conexion, &size, sizeof(int), MSG_WAITALL);

	recv(pcb->conexion, &size_nombre_semaforo, sizeof(int), MSG_WAITALL);

	nuevo_semaforo->nombre = malloc(size_nombre_semaforo);

	recv(pcb->conexion, nuevo_semaforo->nombre, size_nombre_semaforo, MSG_WAITALL);

	recv(pcb->conexion, &nuevo_semaforo->value, sizeof(int), MSG_WAITALL);

	list_create(nuevo_semaforo->cola_bloqueados);

	pthread_mutex_lock(&mutex_lista_semaforos_mate);
	list_add(LISTA_SEMAFOROS_MATE, nuevo_semaforo);
	pthread_mutex_unlock(&mutex_lista_semaforos_mate);

	dar_permiso_para_continuar(pcb->conexion);
}


void wait_sem(t_procesador* estructura_procesador) {
	char* nombre;
	int size;
	int size_nombre_semaforo;

	recv(estructura_procesador->lugar_PCB->conexion, &size, sizeof(int), MSG_WAITALL);

	recv(estructura_procesador->lugar_PCB->conexion, &size_nombre_semaforo, sizeof(int), MSG_WAITALL);

	nombre = malloc(size_nombre_semaforo);

	recv(estructura_procesador->lugar_PCB->conexion, nombre, size_nombre_semaforo, MSG_WAITALL);

	bool _criterio_busqueda_semaforo(void* elemento) {
				return (strcmp(((t_semaforo_mate*)elemento)->nombre, nombre) == 0);
			}

		pthread_mutex_lock(&mutex_lista_semaforos_mate);
		t_semaforo_mate* semaforo = list_find(LISTA_SEMAFOROS_MATE, _criterio_busqueda_semaforo);
		if(semaforo->value > 0) {
			semaforo->value -= 1;
			dar_permiso_para_continuar(estructura_procesador->lugar_PCB->conexion);
		} else {
			list_add(semaforo->cola_bloqueados, estructura_procesador->lugar_PCB);
			pthread_mutex_lock(&mutex_lista_blocked);
			list_add(LISTA_BLOCKED, estructura_procesador->lugar_PCB);
			pthread_mutex_unlock(&mutex_lista_blocked);
			pthread_mutex_lock(&mutex_lista_procesadores); // TENER CUIDADO (si se bloquea todo ver aqui)
			sem_wait(&estructura_procesador->sem_exec);
			estructura_procesador->bit_de_ocupado = 0;
			pthread_mutex_unlock(&mutex_lista_procesadores);
			sem_post(&sem_grado_multiprocesamiento);
		}
		pthread_mutex_unlock(&mutex_lista_semaforos_mate);
}

void post_sem(PCB* pcb) {
	// receive del nombre
	// buscarlo en la lista de sems
	// una vez encontrado, incrementarle en 1 el valor
	// desbloquear el proceso q tiene en la cola en primer lugar (FIFO)
}

void destroy_sem(PCB* pcb) {
	// receive del nombre
	// buscarlo en la lista de sems
	// eliminarlo de la lista list.remove(sem)
	// liberar memoria
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


