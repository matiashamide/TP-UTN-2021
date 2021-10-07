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
		//TODO esto es algo solamente para testear que pasen los procesos necesarios a Ready
		PCB* pcb_test = malloc(sizeof(PCB));
		pcb_test = (PCB*) list_get(LISTA_NEW, 0);
		printf("PID carpincho: %d\n", pcb_test->PID);
		free(pcb_test);
		//ACA termina el testeo
		pthread_mutex_unlock(&mutex_lista_ready);

		sem_post(&sem_cola_ready);

		free(pcb);
	}


}
