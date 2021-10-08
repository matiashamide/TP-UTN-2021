#include "planificacion.h"

void algoritmo_planificador_largo_plazo() {

	while(1) {
		sem_wait(&sem_cola_new);
		sem_wait(&sem_grado_multiprogramacion);

		PCB* pcb = malloc(sizeof(PCB));

		pthread_mutex_lock(&mutex_lista_new);
		pcb = (PCB*) list_remove(LISTA_NEW, 0);
		pthread_mutex_unlock(&mutex_lista_new);

		pthread_mutex_lock(&mutex_lista_ready);
		list_add(LISTA_READY, pcb);
		pthread_mutex_unlock(&mutex_lista_ready);

		sem_post(&sem_cola_ready);

		free(pcb);
	}


}
