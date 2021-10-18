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
		//PCB* pcb_test = malloc(sizeof(PCB));
		//pcb_test = (PCB*) list_get(LISTA_READY, 0);
		//printf("PID carpincho: %d\n", pcb_test->PID);
		//free(pcb_test);
		//ACA termina el testeo
		pthread_mutex_unlock(&mutex_lista_ready);

		sem_post(&sem_cola_ready);

		free(pcb);
	}

}

void algoritmo_planificador_corto_plazo() {

	while(1) {
		sem_wait(&sem_cola_ready);
		sem_wait(&sem_grado_multiprocesamiento);

		if(strcmp(CONFIG_KERNEL.alg_plani, "SJF") == 0) {
			algoritmo_SJF();
		} else if (strcmp(CONFIG_KERNEL.alg_plani, "HRRN") == 0){
			algoritmo_HRRN();
		} else {
			log_info(LOGGER, "El algoritmo de planificacion ingresado no existe\n");
		}
	}

}

void algoritmo_SJF() {

	PCB* pcb = malloc(sizeof(PCB));

	pcb = (PCB*) list_get_minimum(LISTA_READY, criterio_comparacion_SJF());

	pthread_mutex_lock(&mutex_lista_ready);
	list_remove_by_condition(LISTA_READY, criterio_remocion_lista(pcb));
	pthread_mutex_unlock(&mutex_lista_ready);

	pthread_mutex_lock(&mutex_lista_exec);
	list_add(LISTA_EXEC, pcb);
	//TODO esto es algo solamente para testear que pasen los procesos necesarios a Exec
	//PCB* pcb_test = malloc(sizeof(PCB));
	//pcb_test = (PCB*) list_get(LISTA_READY, 0);
	//printf("PID carpincho: %d\n", pcb_test->PID);
	//free(pcb_test);
	//ACA termina el testeo
	pthread_mutex_unlock(&mutex_lista_exec);

	//TODO aca deberian ponerse a ejecutar los procesos en los hilos de multiprocesamiento

}

void algoritmo_HRRN() {


}

void* criterio_comparacion_SJF(void* pcbA, void* pcbB){
	PCB* pcb1 = malloc(sizeof(PCB));
	PCB* pcb2 = malloc(sizeof(PCB));

	uint32_t estimacion_actual1;
	uint32_t estimacion_actual2;

}

bool criterio_remocion_lista(void* pcb) {
	return true;
}
