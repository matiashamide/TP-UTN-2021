#include "planificacion.h"

//TODO arreglar mallocs y memory leaks

void algoritmo_planificador_largo_plazo() {

	while(1) {
		sem_wait(&sem_cola_new);
		sem_wait(&sem_grado_multiprogramacion);

		pthread_mutex_lock(&mutex_lista_new);
		PCB* pcb = (PCB*) list_remove(LISTA_NEW, 0);
		pthread_mutex_unlock(&mutex_lista_new);

		pthread_mutex_lock(&mutex_lista_ready);
		list_add(LISTA_READY, pcb);
		// Esto es algo solamente para testear que pasen los procesos necesarios a Ready
		PCB* pcb_test = malloc(sizeof(PCB));
		pcb_test = (PCB*) list_get(LISTA_READY, 0);
		printf("LISTA READY\n");
		printf("Tamanio ready %d\n", list_size(LISTA_READY));
		printf("PID carpincho: %d\n", pcb_test->PID);
		//free(pcb_test);
		//ACA termina el testeo
		pthread_mutex_unlock(&mutex_lista_ready);

		sem_post(&sem_cola_ready);
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


	void* _eleccion_SJF(void* elemento1, void* elemento2) {

		uint32_t estimacion_actual1;
		uint32_t estimacion_actual2;

		estimacion_actual1 = (CONFIG_KERNEL.alfa) * (((PCB*)elemento1)->real_anterior) + (1-(CONFIG_KERNEL.alfa))*(((PCB*)elemento1)->estimado_anterior);
		estimacion_actual2 = (CONFIG_KERNEL.alfa) * (((PCB*)elemento2)->real_anterior) + (1-(CONFIG_KERNEL.alfa))*(((PCB*)elemento2)->estimado_anterior);

		if(estimacion_actual1 <= estimacion_actual2) {
			return ((PCB*)elemento1);
		} else {
			return ((PCB*)elemento2);
		}
	}

	pthread_mutex_lock(&mutex_lista_ready);
	PCB* pcb = (PCB*) list_get_minimum(LISTA_READY, _eleccion_SJF);
	pthread_mutex_unlock(&mutex_lista_ready);

	bool _criterio_remocion_lista(void* elemento) {
		return (((PCB*)elemento)->PID == pcb->PID);
	}

	pthread_mutex_lock(&mutex_lista_ready);
	list_remove_by_condition(LISTA_READY, _criterio_remocion_lista);
	pthread_mutex_unlock(&mutex_lista_ready);

	pthread_mutex_lock(&mutex_lista_exec);
	list_add(LISTA_EXEC, pcb);
	// Esto es algo solamente para testear que pasen los procesos necesarios a Exec
	PCB* pcb_test = malloc(sizeof(PCB));
	pcb_test = (PCB*) list_get(LISTA_EXEC, 0);
	printf("LISTA EXEC\n");
	printf("Tamanio exec %d\n", list_size(LISTA_EXEC));
	printf("PID carpincho: %d\n", pcb_test->PID);
	//free(pcb_test);
	//ACA termina el testeo
	pthread_mutex_unlock(&mutex_lista_exec);

	//TODO aca deberian ponerse a ejecutar los procesos en los hilos de multiprocesamiento

	// TODO que recorra hasta encontrar un lugar vacio p meter el pcb
	// void *list_find(t_list *, bool(*closure)(void*));
	// TODO hacerle signal al semaforo del procesador correspondiente

}

void algoritmo_HRRN() {

	void* _eleccion_HRRN(void* elemento1, void* elemento2) {

		uint32_t valor_actual1;
		uint32_t valor_actual2;

		valor_actual1 = (((CONFIG_KERNEL.alfa) * (((PCB*)elemento1)->real_anterior) + (1-(CONFIG_KERNEL.alfa))*(((PCB*)elemento1)->estimado_anterior))
						+(((PCB*)elemento1)->tiempo_espera))
						/(((CONFIG_KERNEL.alfa) * (((PCB*)elemento1)->real_anterior) + (1-(CONFIG_KERNEL.alfa))*(((PCB*)elemento1)->estimado_anterior)));
		valor_actual2 = (((CONFIG_KERNEL.alfa) * (((PCB*)elemento2)->real_anterior) + (1-(CONFIG_KERNEL.alfa))*(((PCB*)elemento2)->estimado_anterior))
						+(((PCB*)elemento2)->tiempo_espera))
						/(((CONFIG_KERNEL.alfa) * (((PCB*)elemento2)->real_anterior) + (1-(CONFIG_KERNEL.alfa))*(((PCB*)elemento2)->estimado_anterior)));

		if(valor_actual1 >= valor_actual2) {
			return ((PCB*)elemento1);
		} else {
			return ((PCB*)elemento2);
		}
	}

	pthread_mutex_lock(&mutex_lista_ready);
	PCB* pcb = (PCB*) list_get_maximum(LISTA_READY, _eleccion_HRRN);
	pthread_mutex_unlock(&mutex_lista_ready);


	bool _criterio_remocion_lista(void* elemento) {
		return (((PCB*)elemento)->PID == pcb->PID);
	}

	pthread_mutex_lock(&mutex_lista_ready);
	list_remove_by_condition(LISTA_READY, _criterio_remocion_lista);
	pthread_mutex_unlock(&mutex_lista_ready);

	pthread_mutex_lock(&mutex_lista_exec);
	list_add(LISTA_EXEC, pcb);
	// Esto es algo solamente para testear que pasen los procesos necesarios a Exec
	PCB* pcb_test = malloc(sizeof(PCB));
	pcb_test = (PCB*) list_get(LISTA_EXEC, 0);
	printf("LISTA EXEC\n");
	printf("PID carpincho: %d\n", pcb_test->PID);
	//free(pcb_test);
	//ACA termina el testeo
	pthread_mutex_unlock(&mutex_lista_exec);

	//TODO aca deberian ponerse a ejecutar los procesos en los hilos de multiprocesamiento

	// TODO que recorra hasta encontrar un lugar vacio p meter el pcb
	// void *list_find(t_list *, bool(*closure)(void*));
	// TODO hacerle signal al semaforo del procesador correspondiente

}

void ejecutar(t_procesador* estructura_procesador) {
	while (1) {
		sem_wait(&estructura_procesador->sem_exec);
	}
}

/* while (1) {
		int cod_op = recibir_operacion(cliente_fd);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(cliente_fd);
			break;
		case PAQUETE:
			lista = recibir_paquete(cliente_fd);
			log_info(logger, "Me llegaron los siguientes valores:\n");
			list_iterate(lista, (void*) iterator);
			break;
		case -1:
			log_error(logger, "el cliente se desconecto. Terminando servidor");
			return EXIT_FAILURE;
		default:
			log_warning(logger,"Operacion desconocida. No quieras meter la pata");
			break;
		}
	} */
