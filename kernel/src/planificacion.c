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
		pthread_mutex_unlock(&mutex_lista_ready);

		sem_post(&sem_cola_ready);
	}

}

void algoritmo_planificador_corto_plazo() {

	while(1) {
		sem_wait(&sem_cola_ready);
		sem_wait(&sem_grado_multiprocesamiento);
		PCB* pcb;

		if(strcmp(CONFIG_KERNEL.alg_plani, "SJF") == 0) {
			pcb = algoritmo_SJF();
		} else if (strcmp(CONFIG_KERNEL.alg_plani, "HRRN") == 0){
			pcb = algoritmo_HRRN();
		} else {
			log_info(LOGGER, "El algoritmo de planificacion ingresado no existe\n");
		}

		correr_dispatcher(pcb);

	}

}

void correr_dispatcher(PCB* pcb) {

	bool _criterio_procesador_libre(void* elemento) {
			return (((t_procesador*)elemento)->bit_de_ocupado == 0);
		}

	pthread_mutex_lock(&mutex_lista_procesadores);
	t_procesador* procesador_libre = list_find(LISTA_PROCESADORES, _criterio_procesador_libre);
	procesador_libre->lugar_PCB = pcb;
	procesador_libre->bit_de_ocupado = 1;
	dar_permiso_para_continuar(pcb->conexion);
	sem_post(&procesador_libre->sem_exec);
	pthread_mutex_unlock(&mutex_lista_procesadores);
}

PCB* algoritmo_SJF() {


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

	return pcb;
}

PCB* algoritmo_HRRN() {

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

	return pcb;

}

void ejecutar(t_procesador* estructura_procesador) {

	while (1) {
		sem_wait(&estructura_procesador->sem_exec);
		sem_post(&estructura_procesador->sem_exec);

		peticion_carpincho cod_op = recibir_operacion_carpincho(estructura_procesador->lugar_PCB->conexion);

		printf("Peticion del carpincho: %d\n", cod_op);

		switch (cod_op) {
		case INICIALIZAR_SEM:
			init_sem(estructura_procesador);
			break;
		case ESPERAR_SEM:
			wait_sem(estructura_procesador);
			break;
		case POST_SEM:
			post_sem(estructura_procesador);
			break;
		case DESTROY_SEM:
			//TODO DEFINIR
			break;
		case CALL_IO:
			//TODO DEFINIR
			break;
		case MEMALLOC:
			//TODO DEFINIR
			break;
		case MEMREAD:
			//TODO DEFINIR
			break;
		case MEMFREE:
			//TODO DEFINIR
			break;
		case MEMWRITE:
			//TODO DEFINIR
			break;
		case CLOSE:
			mate_close(estructura_procesador);
			break;
		case MENSAJE:
			//TODO DEFINIR
			break;
		default:
			log_warning(LOGGER,"Operacion desconocida. No quieras meter la pata");
			printf("Operacion desconocida. No quieras meter la pata\n");
			break;
		}

	}
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------- MENSAJES MATE LIB ------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------


void recibir_peticion_para_continuar(int conexion) {


	uint32_t result;
	int bytes_recibidos = recv(conexion, &result, sizeof(uint32_t), MSG_WAITALL);

	printf("Ya recibi peticion %d\n", bytes_recibidos);


}

void dar_permiso_para_continuar(int conexion){

	uint32_t handshake = 1;

	int numero_de_bytes = send(conexion, &handshake, sizeof(uint32_t), 0);

	printf("Ya di permiso %d\n", numero_de_bytes);

}

// TODO manejar error de crear un semaforo con el mismo nombre que uno ya existente
void init_sem(t_procesador* estructura_procesador) {

	t_semaforo_mate* nuevo_semaforo = malloc(sizeof(t_semaforo_mate));

	uint32_t size;
	uint32_t size_nombre_semaforo;

	recv(estructura_procesador->lugar_PCB->conexion, &size, sizeof(uint32_t), MSG_WAITALL);

	recv(estructura_procesador->lugar_PCB->conexion, &size_nombre_semaforo, sizeof(uint32_t), MSG_WAITALL);

	nuevo_semaforo->nombre = malloc(size_nombre_semaforo);

	recv(estructura_procesador->lugar_PCB->conexion, nuevo_semaforo->nombre, size_nombre_semaforo, MSG_WAITALL);

	recv(estructura_procesador->lugar_PCB->conexion, &nuevo_semaforo->value, sizeof(unsigned int), MSG_WAITALL);

	nuevo_semaforo->cola_bloqueados = list_create();

	pthread_mutex_lock(&mutex_lista_semaforos_mate);
	list_add(LISTA_SEMAFOROS_MATE, nuevo_semaforo);
	pthread_mutex_unlock(&mutex_lista_semaforos_mate);

	printf("Tamanio lista de semaforos: %d\n", list_size(LISTA_SEMAFOROS_MATE));
	printf("Nombre semaforo: %s\n", nuevo_semaforo->nombre);
	printf("Valor semaforo: %d\n", nuevo_semaforo->value);


	//fflush(stdout);

	dar_permiso_para_continuar(estructura_procesador->lugar_PCB->conexion);


}


void wait_sem(t_procesador* estructura_procesador) {

	uint32_t size_nombre_semaforo;

	recv(estructura_procesador->lugar_PCB->conexion, &size_nombre_semaforo, sizeof(uint32_t), MSG_WAITALL);

	char* nombre = malloc(size_nombre_semaforo);

	recv(estructura_procesador->lugar_PCB->conexion, nombre, size_nombre_semaforo, MSG_WAITALL);

	bool _criterio_busqueda_semaforo(void* elemento) {
				return (strcmp(((t_semaforo_mate*)elemento)->nombre, nombre) == 0);
			}

	//TEST
	pthread_mutex_lock(&mutex_lista_blocked);
	printf("Tamanio blocked: %d\n", list_size(LISTA_BLOCKED));
	pthread_mutex_unlock(&mutex_lista_blocked);
	//FIN TEST

		pthread_mutex_lock(&mutex_lista_semaforos_mate);
		t_semaforo_mate* semaforo = (t_semaforo_mate*) list_find(LISTA_SEMAFOROS_MATE, _criterio_busqueda_semaforo);
		if(semaforo->value > 0) {
			semaforo->value -= 1;
			//TEST
			printf("Tamanio cola bloqueados semaforo: %d\n", list_size(semaforo->cola_bloqueados));
			//FIN TEST
			dar_permiso_para_continuar(estructura_procesador->lugar_PCB->conexion);
		} else {
			list_add(semaforo->cola_bloqueados, estructura_procesador->lugar_PCB);
			//TEST
			printf("Tamanio cola bloqueados semaforo: %d\n", list_size(semaforo->cola_bloqueados));
			//FIN TEST
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

		//TEST
		pthread_mutex_lock(&mutex_lista_blocked);
		printf("Tamanio blocked: %d\n", list_size(LISTA_BLOCKED));
		pthread_mutex_unlock(&mutex_lista_blocked);
		//FIN TEST

		free(nombre);

}

void post_sem(t_procesador* estructura_procesador) {

	uint32_t size_nombre_semaforo;

	recv(estructura_procesador->lugar_PCB->conexion, &size_nombre_semaforo, sizeof(uint32_t), MSG_WAITALL);

	char* nombre = malloc(size_nombre_semaforo);

	recv(estructura_procesador->lugar_PCB->conexion, nombre, size_nombre_semaforo, MSG_WAITALL);

	bool _criterio_busqueda_semaforo(void* elemento) {
		return (strcmp(((t_semaforo_mate*)elemento)->nombre, nombre) == 0);
	}

	pthread_mutex_lock(&mutex_lista_semaforos_mate);
	t_semaforo_mate* semaforo = (t_semaforo_mate*) list_find(LISTA_SEMAFOROS_MATE, _criterio_busqueda_semaforo);

	if(semaforo->value == 0) {
		semaforo->value += 1;
		PCB* pcb = (PCB*) list_remove(semaforo->cola_bloqueados, 0);

		bool _criterio_remocion_lista(void* elemento) {
				return (((PCB*)elemento)->PID == pcb->PID);
			}

		if(list_any_satisfy(LISTA_BLOCKED, _criterio_remocion_lista)) {

			pthread_mutex_lock(&mutex_lista_blocked);
			list_remove_by_condition(LISTA_BLOCKED, _criterio_remocion_lista);
			pthread_mutex_unlock(&mutex_lista_blocked);

			pthread_mutex_lock(&mutex_lista_ready);
			list_add(LISTA_READY, pcb);
			pthread_mutex_unlock(&mutex_lista_ready);

			sem_post(&sem_cola_ready);
		} //TODO ACA HACER UN ELSE SI NO ESTA EN BLOCKED (Que significa que esta en blocked suspended)

	} else {
		semaforo->value += 1;
	}
	pthread_mutex_unlock(&mutex_lista_semaforos_mate);

	dar_permiso_para_continuar(estructura_procesador->lugar_PCB->conexion);

	//TEST
	pthread_mutex_lock(&mutex_lista_blocked);
	printf("Tamanio blocked: %d\n", list_size(LISTA_BLOCKED));
	pthread_mutex_unlock(&mutex_lista_blocked);
	//FIN TEST
	// una vez encontrado, incrementarle en 1 el valor. HECHO, fijarse el comentario de arriba
	// desbloquear el proceso q tiene en la cola en primer lugar (FIFO). HECHO, fijarse el comentario de arriba
}

void destroy_sem(t_procesador* estructura_procesador) {
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

void mate_close(t_procesador* estructura_procesador) {

	printf("Cierro conexion con carpincho: %d\n", estructura_procesador->lugar_PCB->PID);


	dar_permiso_para_continuar(estructura_procesador->lugar_PCB->conexion);

	close(estructura_procesador->lugar_PCB->conexion);

	pthread_mutex_lock(&mutex_lista_procesadores); // TENER CUIDADO (si se bloquea todo ver aqui)
	sem_wait(&estructura_procesador->sem_exec);
	estructura_procesador->bit_de_ocupado = 0;
	pthread_mutex_unlock(&mutex_lista_procesadores);
	sem_post(&sem_grado_multiprocesamiento);

}
