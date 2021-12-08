#include "planificacion.h"

//TODO arreglar mallocs y memory leaks

void algoritmo_planificador_largo_plazo() {

	while(1) {
		sem_wait(&sem_algoritmo_planificador_largo_plazo);
		sem_wait(&sem_grado_multiprogramacion);

		pthread_mutex_lock(&mutex_lista_ready_suspended);

		if(list_size(LISTA_READY_SUSPENDED) > 0){
			algoritmo_planificador_mediano_plazo_ready_suspended();
		} else {
			pthread_mutex_lock(&mutex_lista_new);
			PCB* pcb = (PCB*) list_remove(LISTA_NEW, 0);
			pthread_mutex_unlock(&mutex_lista_new);

			pthread_mutex_lock(&mutex_lista_ready);
			list_add(LISTA_READY, pcb);
			log_info(LOGGER, "Pasó a READY el carpincho %d\n", pcb->PID);
			pthread_mutex_unlock(&mutex_lista_ready);

		}
		pthread_mutex_unlock(&mutex_lista_ready_suspended);

		sem_post(&sem_cola_ready);
	}
}

void algoritmo_planificador_mediano_plazo_ready_suspended() {
	PCB* pcb = (PCB*) list_remove(LISTA_READY_SUSPENDED, 0);
	avisar_dessuspension_a_memoria(pcb);
	pthread_mutex_lock(&mutex_lista_ready);
	list_add(LISTA_READY, pcb);
	log_info(LOGGER, "Pasó a READY el carpincho %d\n", pcb->PID);
	pthread_mutex_unlock(&mutex_lista_ready);

}

void algoritmo_planificador_mediano_plazo_blocked_suspended() {

	pthread_mutex_lock(&mutex_lista_blocked);

	int cantidad_procesos_blocked = list_size(LISTA_BLOCKED);

	pthread_mutex_lock(&mutex_lista_new);
	int cantidad_procesos_new = list_size(LISTA_NEW);
	pthread_mutex_unlock(&mutex_lista_new);

	if(cantidad_procesos_blocked == CONFIG_KERNEL.grado_multiprogramacion && cantidad_procesos_new > 0) {
		PCB* pcb = (PCB*) list_remove(LISTA_BLOCKED, cantidad_procesos_blocked-1);

		pthread_mutex_lock(&mutex_lista_blocked_suspended);
		avisar_suspension_a_memoria(pcb);
		list_add(LISTA_BLOCKED_SUSPENDED, pcb);
		log_info(LOGGER, "Pasó a BLOCKED-SUSPENDED el carpincho %d\n", pcb->PID);
		pthread_mutex_unlock(&mutex_lista_blocked_suspended);

		sem_post(&sem_grado_multiprogramacion);
	}
	pthread_mutex_unlock(&mutex_lista_blocked);
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
	gettimeofday(&procesador_libre->timeValBefore, NULL);
	sem_post(&procesador_libre->sem_exec);
	log_info(LOGGER, "Pasó a EXEC el carpincho %d\n", pcb->PID);
	pthread_mutex_unlock(&mutex_lista_procesadores);

}

PCB* algoritmo_SJF() {


	void* _eleccion_SJF(void* elemento1, void* elemento2) {

		double estimacion_actual1;
		double estimacion_actual2;

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
	for(int i = 0; i < list_size(LISTA_READY); i++) {
		PCB* pcbActualizacion = list_get(LISTA_READY, i);
		pcbActualizacion->estimado_anterior = (CONFIG_KERNEL.alfa) * (pcb->real_anterior) + (1-(CONFIG_KERNEL.alfa))*(pcb->estimado_anterior);
	}
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

		double valor_actual1;
		double valor_actual2;

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
	for(int i = 0; i < list_size(LISTA_READY); i++) {
		PCB* pcbActualizacion = list_get(LISTA_READY, i);
		pcbActualizacion->estimado_anterior = (CONFIG_KERNEL.alfa) * (pcb->real_anterior) + (1-(CONFIG_KERNEL.alfa))*(pcb->estimado_anterior);
	}
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
			destroy_sem(estructura_procesador);
			break;
		case CALL_IO:
			call_IO(estructura_procesador);
			break;
		case MEMALLOC:
			memalloc(estructura_procesador);
			break;
		case MEMREAD:
			memread(estructura_procesador);
			break;
		case MEMFREE:
			memfree(estructura_procesador);
			break;
		case MEMWRITE:
			memwrite(estructura_procesador);
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
	//int bytes_recibidos =
			recv(conexion, &result, sizeof(uint32_t), MSG_WAITALL);

	//printf("Ya recibi peticion %d\n", bytes_recibidos);


}

void dar_permiso_para_continuar(int conexion){

	uint32_t handshake = 1;

	//int numero_de_bytes =
			send(conexion, &handshake, sizeof(uint32_t), 0);

	//printf("Ya di permiso %d\n", numero_de_bytes);
}

void avisar_finalizacion_por_deadlock(int conexion) {
	uint32_t handshake = 2;

	//int numero_de_bytes =
			send(conexion, &handshake, sizeof(uint32_t), 0);

	//printf("Finalice un carpincho por deadlock %d\n", numero_de_bytes);
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

	log_info(LOGGER, "El carpincho %d, inicializó el semaforo %s con valor %d\n", estructura_procesador->lugar_PCB->PID, nuevo_semaforo->nombre, nuevo_semaforo->value);

	dar_permiso_para_continuar(estructura_procesador->lugar_PCB->conexion);
}


void wait_sem(t_procesador* estructura_procesador) {

	uint32_t size_nombre_semaforo;

	recv(estructura_procesador->lugar_PCB->conexion, &size_nombre_semaforo, sizeof(uint32_t), MSG_WAITALL);

	char* nombre = malloc(size_nombre_semaforo);

	recv(estructura_procesador->lugar_PCB->conexion, nombre, size_nombre_semaforo, MSG_WAITALL);

	log_info(LOGGER, "El carpincho %d, hizo wait al semaforo %s\n", estructura_procesador->lugar_PCB->PID, nombre);

	bool _criterio_busqueda_semaforo(void* elemento) {
				return (strcmp(((t_semaforo_mate*)elemento)->nombre, nombre) == 0);
			}

		pthread_mutex_lock(&mutex_lista_semaforos_mate);
		t_semaforo_mate* semaforo = (t_semaforo_mate*) list_find(LISTA_SEMAFOROS_MATE, _criterio_busqueda_semaforo);
		if(semaforo->value > 0) {
			semaforo->value -= 1;

			bool _criterio_busqueda_semaforo_en_carpincho(void* elemento) {
					return (strcmp(((t_registro_uso_recurso*)elemento)->nombre, nombre) == 0);
				}

			if(list_any_satisfy(estructura_procesador->lugar_PCB->recursos_usados, _criterio_busqueda_semaforo_en_carpincho)) {
				t_registro_uso_recurso* recurso = (t_registro_uso_recurso*) list_find(estructura_procesador->lugar_PCB->recursos_usados, _criterio_busqueda_semaforo_en_carpincho);
				recurso->cantidad += 1;
			} else {
				t_registro_uso_recurso* recurso = malloc(sizeof(t_registro_uso_recurso));
				recurso->nombre = malloc(size_nombre_semaforo);
				memcpy(recurso->nombre, nombre, size_nombre_semaforo);
				recurso->cantidad = 1;
				list_add(estructura_procesador->lugar_PCB->recursos_usados, recurso);
				//TODO hacerle free a esto
			}

			dar_permiso_para_continuar(estructura_procesador->lugar_PCB->conexion);
		} else {
			list_add(semaforo->cola_bloqueados, estructura_procesador->lugar_PCB);
			pthread_mutex_lock(&mutex_lista_blocked);
			gettimeofday(&estructura_procesador->timeValAfter, NULL);
			estructura_procesador->lugar_PCB->real_anterior = ((estructura_procesador->timeValAfter.tv_sec - estructura_procesador->timeValBefore.tv_sec)*1000000L+estructura_procesador->timeValAfter.tv_usec) - estructura_procesador->timeValBefore.tv_usec;
			list_add(LISTA_BLOCKED, estructura_procesador->lugar_PCB);
			log_info(LOGGER, "Pasó a BLOCKED el carpincho %d\n", estructura_procesador->lugar_PCB->PID);
			pthread_mutex_unlock(&mutex_lista_blocked);
			pthread_mutex_lock(&mutex_lista_procesadores); // TENER CUIDADO (si se bloquea todo ver aqui)
			sem_wait(&estructura_procesador->sem_exec);
			estructura_procesador->bit_de_ocupado = 0;
			pthread_mutex_unlock(&mutex_lista_procesadores);
			sem_post(&sem_grado_multiprocesamiento);
			algoritmo_planificador_mediano_plazo_blocked_suspended();
		}
		pthread_mutex_unlock(&mutex_lista_semaforos_mate);

		free(nombre);

}

void post_sem(t_procesador* estructura_procesador) {

	uint32_t size_nombre_semaforo;

	recv(estructura_procesador->lugar_PCB->conexion, &size_nombre_semaforo, sizeof(uint32_t), MSG_WAITALL);

	char* nombre = malloc(size_nombre_semaforo);

	recv(estructura_procesador->lugar_PCB->conexion, nombre, size_nombre_semaforo, MSG_WAITALL);

	log_info(LOGGER, "El carpincho %d, hizo signal al semaforo %s\n", estructura_procesador->lugar_PCB->PID, nombre);

	bool _criterio_busqueda_semaforo(void* elemento) {
		return (strcmp(((t_semaforo_mate*)elemento)->nombre, nombre) == 0);
	}

	pthread_mutex_lock(&mutex_lista_semaforos_mate);
	t_semaforo_mate* semaforo = (t_semaforo_mate*) list_find(LISTA_SEMAFOROS_MATE, _criterio_busqueda_semaforo);

	if(semaforo->value == 0 && list_size(semaforo->cola_bloqueados) > 0) {
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
			log_info(LOGGER, "Pasó a READY el carpincho %d\n", pcb->PID);
			pthread_mutex_unlock(&mutex_lista_ready);

			sem_post(&sem_cola_ready);
		} else {
			pthread_mutex_lock(&mutex_lista_blocked_suspended);
			list_remove_by_condition(LISTA_BLOCKED_SUSPENDED, _criterio_remocion_lista);
			pthread_mutex_unlock(&mutex_lista_blocked_suspended);

			pthread_mutex_lock(&mutex_lista_ready_suspended);
			list_add(LISTA_READY_SUSPENDED, pcb);
			log_info(LOGGER, "Pasó a READY-SUSPENDED el carpincho %d\n", pcb->PID);
			pthread_mutex_unlock(&mutex_lista_ready_suspended);

			sem_post(&sem_algoritmo_planificador_largo_plazo);
		}

	} else {
		semaforo->value += 1;
	}
	pthread_mutex_unlock(&mutex_lista_semaforos_mate);

	dar_permiso_para_continuar(estructura_procesador->lugar_PCB->conexion);
}

void destroy_sem(t_procesador* estructura_procesador) {
	uint32_t size_nombre_semaforo;

	recv(estructura_procesador->lugar_PCB->conexion, &size_nombre_semaforo, sizeof(uint32_t), MSG_WAITALL);

	char* nombre = malloc(size_nombre_semaforo);

	recv(estructura_procesador->lugar_PCB->conexion, nombre, size_nombre_semaforo, MSG_WAITALL);

	log_info(LOGGER, "El carpincho %d, hizo destroy al semaforo %s\n", estructura_procesador->lugar_PCB->PID, nombre);

	bool _criterio_busqueda_semaforo(void* elemento) {
		return (strcmp(((t_semaforo_mate*)elemento)->nombre, nombre) == 0);
	}

	pthread_mutex_lock(&mutex_lista_semaforos_mate);
	t_semaforo_mate* semaforo = (t_semaforo_mate*) list_find(LISTA_SEMAFOROS_MATE, _criterio_busqueda_semaforo);

	if(list_size(semaforo->cola_bloqueados) > 0) {
		int tamanio_cola_blocked = list_size(semaforo->cola_bloqueados);
		for(int i = 0; i < tamanio_cola_blocked; i++) {

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
				log_info(LOGGER, "Pasó a READY el carpincho %d\n", pcb->PID);
				pthread_mutex_unlock(&mutex_lista_ready);

				sem_post(&sem_cola_ready);
			} else {
				pthread_mutex_lock(&mutex_lista_blocked_suspended);
				list_remove_by_condition(LISTA_BLOCKED_SUSPENDED, _criterio_remocion_lista);
				pthread_mutex_unlock(&mutex_lista_blocked_suspended);

				pthread_mutex_lock(&mutex_lista_ready_suspended);
				list_add(LISTA_READY_SUSPENDED, pcb);
				log_info(LOGGER, "Pasó a READY-SUSPENDED el carpincho %d\n", pcb->PID);
				pthread_mutex_unlock(&mutex_lista_ready_suspended);

				sem_post(&sem_algoritmo_planificador_largo_plazo);
			}
		}
	}

	list_remove_by_condition(LISTA_SEMAFOROS_MATE, _criterio_busqueda_semaforo);
	log_info(LOGGER, "Se eliminó el semaforo %s\n", nombre);

	pthread_mutex_unlock(&mutex_lista_semaforos_mate);

	free(semaforo->cola_bloqueados);
	free(semaforo->nombre);
	free(semaforo);

	dar_permiso_para_continuar(estructura_procesador->lugar_PCB->conexion);
}

void call_IO(t_procesador* estructura_procesador) {
	uint32_t size_nombre_dispositivo;

	recv(estructura_procesador->lugar_PCB->conexion, &size_nombre_dispositivo, sizeof(uint32_t), MSG_WAITALL);

	char* nombre = malloc(size_nombre_dispositivo);

	recv(estructura_procesador->lugar_PCB->conexion, nombre, size_nombre_dispositivo, MSG_WAITALL);

	log_info(LOGGER, "El carpincho %d, pidió hacer I/O en %s\n", estructura_procesador->lugar_PCB->PID, nombre);

	bool _criterio_busqueda_dispositivo(void* elemento) {
			return (strcmp(((t_dispositivo*)elemento)->nombre, nombre) == 0);
		}

	pthread_mutex_lock(&mutex_lista_dispositivos_io);
	t_dispositivo* dispositivo = (t_dispositivo*) list_find(LISTA_DISPOSITIVOS_IO, _criterio_busqueda_dispositivo);
	list_add(dispositivo->cola_espera, estructura_procesador->lugar_PCB);
	sem_post(&dispositivo->sem);
	pthread_mutex_unlock(&mutex_lista_dispositivos_io);

	pthread_mutex_lock(&mutex_lista_blocked);
	gettimeofday(&estructura_procesador->timeValAfter, NULL);
	estructura_procesador->lugar_PCB->real_anterior = ((estructura_procesador->timeValAfter.tv_sec - estructura_procesador->timeValBefore.tv_sec)*1000000L+estructura_procesador->timeValAfter.tv_usec) - estructura_procesador->timeValBefore.tv_usec;
	list_add(LISTA_BLOCKED, estructura_procesador->lugar_PCB);
	log_info(LOGGER, "Pasó a BLOCKED el carpincho %d\n", estructura_procesador->lugar_PCB->PID);
	pthread_mutex_unlock(&mutex_lista_blocked);
	pthread_mutex_lock(&mutex_lista_procesadores); // TENER CUIDADO (si se bloquea todo ver aqui)
	sem_wait(&estructura_procesador->sem_exec);
	estructura_procesador->bit_de_ocupado = 0;
	pthread_mutex_unlock(&mutex_lista_procesadores);
	sem_post(&sem_grado_multiprocesamiento);
	algoritmo_planificador_mediano_plazo_blocked_suspended();
}

void ejecutar_io(t_dispositivo* dispositivo) {
	while(1) {
		sem_wait(&dispositivo->sem);
		PCB* pcb = (PCB*) list_remove(dispositivo->cola_espera, 0);
		usleep(dispositivo->rafaga*1000);

		bool _criterio_remocion_lista(void* elemento) {
			return (((PCB*)elemento)->PID == pcb->PID);
		}

		if(list_any_satisfy(LISTA_BLOCKED, _criterio_remocion_lista)) {

			pthread_mutex_lock(&mutex_lista_blocked);
			list_remove_by_condition(LISTA_BLOCKED, _criterio_remocion_lista);
			pthread_mutex_unlock(&mutex_lista_blocked);

			pthread_mutex_lock(&mutex_lista_ready);
			list_add(LISTA_READY, pcb);
			log_info(LOGGER, "Pasó a READY el carpincho %d\n", pcb->PID);
			pthread_mutex_unlock(&mutex_lista_ready);

			sem_post(&sem_cola_ready);
		} else {
			pthread_mutex_lock(&mutex_lista_blocked_suspended);
			list_remove_by_condition(LISTA_BLOCKED_SUSPENDED, _criterio_remocion_lista);
			pthread_mutex_unlock(&mutex_lista_blocked_suspended);

			pthread_mutex_lock(&mutex_lista_ready_suspended);
			list_add(LISTA_READY_SUSPENDED, pcb);
			log_info(LOGGER, "Pasó a READY-SUSPENDED el carpincho %d\n", pcb->PID);
			pthread_mutex_unlock(&mutex_lista_ready_suspended);

			sem_post(&sem_algoritmo_planificador_largo_plazo);
		}
	}
}



void memalloc(t_procesador* estructura_procesador) {

	int32_t entero;
	uint32_t mallocSize;

	recv(estructura_procesador->lugar_PCB->conexion, &entero, sizeof(uint32_t), MSG_WAITALL);
	recv(estructura_procesador->lugar_PCB->conexion, &mallocSize, sizeof(uint32_t), MSG_WAITALL);

	int conexion_memoria = crear_conexion(CONFIG_KERNEL.ip_memoria , CONFIG_KERNEL.puerto_memoria);

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MEMALLOC;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(uint32_t)*2;
	paquete->buffer->stream = malloc(paquete->buffer->size);

	int offset = 0;

	memcpy(paquete->buffer->stream, &estructura_procesador->lugar_PCB->PID, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + offset, &mallocSize, sizeof(unsigned int));

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	int32_t dir_logica;

	send(conexion_memoria, a_enviar, bytes, 0);
	recv(conexion_memoria, &dir_logica , sizeof(int32_t) , 0);

	send(estructura_procesador->lugar_PCB->conexion, &dir_logica, sizeof(int32_t), 0);

	free(a_enviar);
	eliminar_paquete(paquete);

	close(conexion_memoria);

}

void memread(t_procesador* estructura_procesador) {

	int32_t entero;
	uint32_t origin;
	uint32_t size;

	recv(estructura_procesador->lugar_PCB->conexion, &entero, sizeof(uint32_t), MSG_WAITALL);
	recv(estructura_procesador->lugar_PCB->conexion, &origin, sizeof(uint32_t), MSG_WAITALL);
	recv(estructura_procesador->lugar_PCB->conexion, &size, sizeof(uint32_t), MSG_WAITALL);

	int conexion_memoria = crear_conexion(CONFIG_KERNEL.ip_memoria , CONFIG_KERNEL.puerto_memoria);

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MEMREAD;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int32_t) * 3;
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &estructura_procesador->lugar_PCB->PID, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + sizeof(uint32_t), &origin, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + (sizeof(uint32_t)*2), &size, sizeof(uint32_t));

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	MATE_RETURNS retorno;
	void* dest = malloc(size);

	send(conexion_memoria, a_enviar, bytes, 0);
	recv(conexion_memoria, &retorno , sizeof(uint32_t) , 0);
	recv(conexion_memoria, dest, size, 0);

	send(estructura_procesador->lugar_PCB->conexion, &retorno, sizeof(uint32_t), 0);
	send(estructura_procesador->lugar_PCB->conexion, dest, size, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
	free(dest);

	close(conexion_memoria);

}

void memfree(t_procesador* estructura_procesador) {

	int32_t entero;
	int32_t addr;
	recv(estructura_procesador->lugar_PCB->conexion, &entero, sizeof(uint32_t), MSG_WAITALL);
	recv(estructura_procesador->lugar_PCB->conexion, &addr, sizeof(uint32_t), MSG_WAITALL);

	int conexion_memoria = crear_conexion(CONFIG_KERNEL.ip_memoria , CONFIG_KERNEL.puerto_memoria);

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MEMFREE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int32_t)*2;
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &estructura_procesador->lugar_PCB->PID, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + sizeof(uint32_t), &addr, sizeof(uint32_t));

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	MATE_RETURNS retorno;

	send(conexion_memoria, a_enviar, bytes, 0);
	recv(conexion_memoria, &retorno, sizeof(uint32_t), 0);

	send(estructura_procesador->lugar_PCB->conexion, &retorno, sizeof(uint32_t), 0);

	free(a_enviar);
	eliminar_paquete(paquete);

	close(conexion_memoria);

}

void memwrite(t_procesador* estructura_procesador) {

	int32_t entero;
	void* origin;
	uint32_t size;
	uint32_t dest;

	recv(estructura_procesador->lugar_PCB->conexion, &entero, sizeof(uint32_t), MSG_WAITALL);
	recv(estructura_procesador->lugar_PCB->conexion, &dest, sizeof(uint32_t), MSG_WAITALL);
	recv(estructura_procesador->lugar_PCB->conexion, &size, sizeof(uint32_t), MSG_WAITALL);

	origin = malloc(size);

	recv(estructura_procesador->lugar_PCB->conexion, origin, size, MSG_WAITALL);

	int conexion_memoria = crear_conexion(CONFIG_KERNEL.ip_memoria , CONFIG_KERNEL.puerto_memoria);

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MEMWRITE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int32_t) * 3 + size;
	paquete->buffer->stream = malloc(paquete->buffer->size);


	memcpy(paquete->buffer->stream, &estructura_procesador->lugar_PCB->PID, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + sizeof(uint32_t), &dest, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + sizeof(uint32_t)*2, &size, sizeof(uint32_t));
	memcpy(paquete->buffer->stream + sizeof(uint32_t)*3, origin, size);

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	MATE_RETURNS retorno;

	send(conexion_memoria, a_enviar, bytes, 0);
	recv(conexion_memoria, &retorno , sizeof(uint32_t) , 0);

	send(estructura_procesador->lugar_PCB->conexion, &retorno, sizeof(uint32_t), 0);

	free(a_enviar);
	eliminar_paquete(paquete);

	close(conexion_memoria);

}

void mate_close(t_procesador* estructura_procesador) {

	log_info(LOGGER, "El carpincho %d, terminó\n", estructura_procesador->lugar_PCB->PID);

	dar_permiso_para_continuar(estructura_procesador->lugar_PCB->conexion);

	close(estructura_procesador->lugar_PCB->conexion);

	avisar_close_a_memoria(estructura_procesador->lugar_PCB);

	pthread_mutex_lock(&mutex_lista_procesadores); // TENER CUIDADO (si se bloquea todo ver aqui)
	sem_wait(&estructura_procesador->sem_exec);
	estructura_procesador->bit_de_ocupado = 0;
	pthread_mutex_unlock(&mutex_lista_procesadores);

	for(int k = 0; k < list_size(estructura_procesador->lugar_PCB->recursos_usados); k++) {

		t_registro_uso_recurso* recurso_usado = list_get(estructura_procesador->lugar_PCB->recursos_usados, k);

		bool _criterio_busqueda_semaforo(void* elemento) {
			return (strcmp(((t_semaforo_mate*)elemento)->nombre, recurso_usado->nombre) == 0);
		}

		pthread_mutex_lock(&mutex_lista_semaforos_mate);

		t_semaforo_mate* recurso = (t_semaforo_mate*) list_find(LISTA_SEMAFOROS_MATE, _criterio_busqueda_semaforo);

		for (int y = 0; y < recurso_usado->cantidad; y++) {

			if(recurso->value == 0 && list_size(recurso->cola_bloqueados) > 0) {
				PCB* pcb = (PCB*) list_remove(recurso->cola_bloqueados, 0);

				bool _criterio_remocion_lista(void* elemento) {
					return (((PCB*)elemento)->PID == pcb->PID);
				}

				if(list_any_satisfy(LISTA_BLOCKED, _criterio_remocion_lista)) {

					pthread_mutex_lock(&mutex_lista_blocked);
					list_remove_by_condition(LISTA_BLOCKED, _criterio_remocion_lista);
					pthread_mutex_unlock(&mutex_lista_blocked);

					pthread_mutex_lock(&mutex_lista_ready);
					list_add(LISTA_READY, pcb);
					log_info(LOGGER, "Pasó a READY el carpincho %d\n", pcb->PID);
					pthread_mutex_unlock(&mutex_lista_ready);

					sem_post(&sem_cola_ready);
				} else {
					pthread_mutex_lock(&mutex_lista_blocked_suspended);
					list_remove_by_condition(LISTA_BLOCKED_SUSPENDED, _criterio_remocion_lista);
					pthread_mutex_unlock(&mutex_lista_blocked_suspended);

					pthread_mutex_lock(&mutex_lista_ready_suspended);
					list_add(LISTA_READY_SUSPENDED, pcb);
					log_info(LOGGER, "Pasó a READY-SUSPENDED el carpincho %d\n", pcb->PID);
					pthread_mutex_unlock(&mutex_lista_ready_suspended);

					sem_post(&sem_algoritmo_planificador_largo_plazo);
				}

			} else {
				recurso->value += 1;
			}

		}

	}

	bool _criterio_igual_pid(void* elemento) {
		return (((PCB*)elemento)->PID == estructura_procesador->lugar_PCB->PID);
	}

	for(int f = 0; f < list_size(LISTA_SEMAFOROS_MATE); f++) {
		t_semaforo_mate* sema = list_get(LISTA_SEMAFOROS_MATE, f);
		if(list_any_satisfy(sema->cola_bloqueados, _criterio_igual_pid)) {
			list_remove_by_condition(sema->cola_bloqueados, _criterio_igual_pid);
		}
	}

	pthread_mutex_unlock(&mutex_lista_semaforos_mate);

	void _element_destroyer(void* elemento) {
		free(((t_registro_uso_recurso*)elemento)->nombre);
		free(((t_registro_uso_recurso*)elemento));
	}

	list_destroy_and_destroy_elements(estructura_procesador->lugar_PCB->recursos_usados, _element_destroyer);
	free(estructura_procesador->lugar_PCB);

	sem_post(&sem_grado_multiprocesamiento);
	sem_post(&sem_grado_multiprogramacion);
	//TODO liberar los semaforos que le correspondan y mas cosas

}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------- DEADLOCK ------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------

//TODO
//Matriz de asignacion (de recursos asignados)
//Matriz de peticiones (peticiones actuales)
//Vectores de recursos totales y disponibles

//Finalizar procesos (y corro el algoritmo de deteccion cada vez que mato un proceso, se mata al de mayor PID)

//Al matar un carpincho hay que liberar los semaforos que tiene asignados (para eso la lista de recursos usados)

//Las peticiones pendientes para un carpincho va a poder ser solamente una instancia de un semaforo

//Para armar la matriz recorres tus semaforos, y esas son tus columnas de la matriz
//sabemos que todos los carpinchos que estan bloqueados por ese semaforo es que estan pidiendo una instancia de ese semaforo

//esa es la matriz de peticiones pendientes, por cada fila va a haber un 1 nada mas, en alguno de los semaforos
//dos carpinchos podrian tener un uno sobre la misma columna pero cada fila (cada carpincho) va a tener un uno en uno de los semaforos y nada mas
//La cantidad de carpinchos es muy reducida porque solo estas viendo los que estan bloqueados con algun semaforo, el resto no interesan
//

//Una vez que tenes la de peticiones pendientes, recorres por fila y ves, que cada carpincho este reteniendo un recurso que este pidiendo otro, de los mismos que retengo yo.
//No importa cuantas instancias retengas, con que retengas una, ya estas cagando al otro (estas en deadlock)

//Hay que ir limpiando los que no interesan

//LOS RECURSOS ASIGNADOS

//ver el tema de que el deadlock no siga corriendo cada ese tiempo de config mientras estamos recuperandonos de un deadlock

void correr_algoritmo_deadlock() {
	while(1) {
		usleep(CONFIG_KERNEL.tiempo_deadlock*1000);
		algoritmo_deteccion_deadlock();
	}
}


void algoritmo_deteccion_deadlock() {

	int i;
	int j;
	t_list* lista_bloqueados_por_semaforo = list_create();
	t_list* lista_procesos_en_deadlock = list_create();

	pthread_mutex_lock(&mutex_lista_semaforos_mate);
	//Ver tema mutex lista de bloqueados

	for(i = 0; i < list_size(LISTA_SEMAFOROS_MATE); i++) {
		t_semaforo_mate* sem = list_get(LISTA_SEMAFOROS_MATE, i);
		for (j = 0; j < list_size(sem->cola_bloqueados); j++) {
			PCB* proceso = list_get(sem->cola_bloqueados, j);
			list_add(lista_bloqueados_por_semaforo, proceso);
		}
	}

	for(int h = 0; h < list_size(lista_bloqueados_por_semaforo); h++) {
		PCB* pcb = list_get(lista_bloqueados_por_semaforo, h);

		bool _criterio_igual_pid_en_bloqueados(void* elemento) {
			return (((PCB*)elemento)->PID == pcb->PID);
		}

		bool _criterio_busqueda_semaforo_en_que_esta_bloqueado(void* elemento) {
			return (list_any_satisfy(((t_semaforo_mate*)elemento)->cola_bloqueados, _criterio_igual_pid_en_bloqueados));
		}

		t_semaforo_mate* semaforo_en_que_esta_bloqueado = list_find(LISTA_SEMAFOROS_MATE, _criterio_busqueda_semaforo_en_que_esta_bloqueado);

		if(list_size(pcb->recursos_usados) > 0) {
			for(int g = 0; g < list_size(pcb->recursos_usados); g++) {
				t_registro_uso_recurso* recurso_usado = list_get(pcb->recursos_usados, g);

				bool _criterio_busqueda_semaforo(void* elemento) {
					return (strcmp(((t_semaforo_mate*)elemento)->nombre, recurso_usado->nombre) == 0);
				}

				t_semaforo_mate* semaforo = (t_semaforo_mate*) list_find(LISTA_SEMAFOROS_MATE, _criterio_busqueda_semaforo);

				bool _criterio_procesos_distintos(void* elemento) {
					return (((PCB*)elemento)->PID != pcb->PID);
				}

				bool _criterio_remocion_lista(void* elemento) {
					return (((PCB*)elemento)->PID == pcb->PID);
				}

				int procesos_que_lo_piden = list_count_satisfying(semaforo->cola_bloqueados, _criterio_procesos_distintos);

				if(procesos_que_lo_piden > 0 && !list_any_satisfy(lista_procesos_en_deadlock, _criterio_remocion_lista)) {
					t_list* lista_bloqueados_por_semaforo_menos_este = list_filter(lista_bloqueados_por_semaforo, _criterio_procesos_distintos);

					for(int k = 0; k < list_size(lista_bloqueados_por_semaforo_menos_este); k++) {
						PCB* carpincho = list_get(lista_bloqueados_por_semaforo, k);
						for(int o = 0; o < list_size(carpincho->recursos_usados); o++) {
							t_registro_uso_recurso* recurso_usado_carpincho = list_get(carpincho->recursos_usados, o);
							if(strcmp(recurso_usado_carpincho->nombre, semaforo_en_que_esta_bloqueado->nombre) == 0) {
								list_add(lista_procesos_en_deadlock, pcb);
							}
						}
					}
				}
			}
		}
	}

	if(list_size(lista_procesos_en_deadlock) > 0) {

		void* _eleccion_mayor_PID(void* elemento1, void* elemento2) {
			if(((PCB*)elemento1)->PID > ((PCB*)elemento2)->PID) {
				return ((PCB*)elemento1);
			} else {
				return ((PCB*)elemento2);
			}
		}

		PCB* proceso_de_mayor_pid = (PCB*) list_get_maximum(lista_procesos_en_deadlock, _eleccion_mayor_PID);
		log_info(LOGGER, "Finalizo al carpincho %d por deadlock\n", proceso_de_mayor_pid->PID);
		avisar_close_a_memoria(proceso_de_mayor_pid);

		bool _criterio_igual_pid(void* elemento) {
			return (((PCB*)elemento)->PID == proceso_de_mayor_pid->PID);
		}


		if(list_any_satisfy(LISTA_BLOCKED, _criterio_igual_pid)) {
			pthread_mutex_lock(&mutex_lista_blocked);
			list_remove_by_condition(LISTA_BLOCKED, _criterio_igual_pid);
			pthread_mutex_unlock(&mutex_lista_blocked);
		} else {
			pthread_mutex_lock(&mutex_lista_blocked_suspended);
			list_remove_by_condition(LISTA_BLOCKED_SUSPENDED, _criterio_igual_pid);
			pthread_mutex_unlock(&mutex_lista_blocked_suspended);
		}

		for(int k = 0; k < list_size(proceso_de_mayor_pid->recursos_usados); k++) {

			t_registro_uso_recurso* recurso_usado = list_get(proceso_de_mayor_pid->recursos_usados, k);

			bool _criterio_busqueda_semaforo(void* elemento) {
				return (strcmp(((t_semaforo_mate*)elemento)->nombre, recurso_usado->nombre) == 0);
			}

			t_semaforo_mate* recurso = (t_semaforo_mate*) list_find(LISTA_SEMAFOROS_MATE, _criterio_busqueda_semaforo);

			for (int y = 0; y < recurso_usado->cantidad; y++) {

				if(recurso->value == 0 && list_size(recurso->cola_bloqueados) > 0) {
					PCB* pcb = (PCB*) list_remove(recurso->cola_bloqueados, 0);

					bool _criterio_remocion_lista(void* elemento) {
						return (((PCB*)elemento)->PID == pcb->PID);
					}

					if(list_any_satisfy(LISTA_BLOCKED, _criterio_remocion_lista)) {

						pthread_mutex_lock(&mutex_lista_blocked);
						list_remove_by_condition(LISTA_BLOCKED, _criterio_remocion_lista);
						pthread_mutex_unlock(&mutex_lista_blocked);

						pthread_mutex_lock(&mutex_lista_ready);
						list_add(LISTA_READY, pcb);
						log_info(LOGGER, "Pasó a READY el carpincho %d\n", pcb->PID);
						pthread_mutex_unlock(&mutex_lista_ready);

						sem_post(&sem_cola_ready);
					} else {
						pthread_mutex_lock(&mutex_lista_blocked_suspended);
						list_remove_by_condition(LISTA_BLOCKED_SUSPENDED, _criterio_remocion_lista);
						pthread_mutex_unlock(&mutex_lista_blocked_suspended);

						pthread_mutex_lock(&mutex_lista_ready_suspended);
						list_add(LISTA_READY_SUSPENDED, pcb);
						log_info(LOGGER, "Pasó a READY-SUSPENDED el carpincho %d\n", pcb->PID);
						pthread_mutex_unlock(&mutex_lista_ready_suspended);

						sem_post(&sem_algoritmo_planificador_largo_plazo);
					}

				} else {
					recurso->value += 1;
				}

			}

		}

		//TODO aca aumentar el grado de multiprogramacion, cerrar conexiones, y liquidar al carpincho
		avisar_finalizacion_por_deadlock(proceso_de_mayor_pid->conexion);
		close(proceso_de_mayor_pid->conexion);

		list_clean(lista_procesos_en_deadlock);
		list_clean(lista_bloqueados_por_semaforo);

		for(int f = 0; f < list_size(LISTA_SEMAFOROS_MATE); f++) {
			t_semaforo_mate* sema = list_get(LISTA_SEMAFOROS_MATE, f);
			if(list_any_satisfy(sema->cola_bloqueados, _criterio_igual_pid)) {
				list_remove_by_condition(sema->cola_bloqueados, _criterio_igual_pid);
			}
		}

		void _element_destroyer(void* elemento) {
			free(((t_registro_uso_recurso*)elemento)->nombre);
			free(((t_registro_uso_recurso*)elemento));
		}

		list_destroy_and_destroy_elements(proceso_de_mayor_pid->recursos_usados, _element_destroyer);
		free(proceso_de_mayor_pid);

		sem_post(&sem_grado_multiprogramacion);

		pthread_mutex_unlock(&mutex_lista_semaforos_mate);
		algoritmo_deteccion_deadlock();

	} else {
		pthread_mutex_unlock(&mutex_lista_semaforos_mate);

	}

}

void avisar_suspension_a_memoria(PCB* pcb) {

	int conexion_memoria = crear_conexion(CONFIG_KERNEL.ip_memoria , CONFIG_KERNEL.puerto_memoria);

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MEMSUSP;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &pcb->PID, sizeof(uint32_t));

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	MATE_RETURNS retorno;

	send(conexion_memoria, a_enviar, bytes, 0);
	recv(conexion_memoria, &retorno, sizeof(uint32_t), 0);

	free(a_enviar);
	eliminar_paquete(paquete);

	close(conexion_memoria);
}

void avisar_dessuspension_a_memoria(PCB* pcb) {

	int conexion_memoria = crear_conexion(CONFIG_KERNEL.ip_memoria , CONFIG_KERNEL.puerto_memoria);

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MEMDESSUSP;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &pcb->PID, sizeof(uint32_t));

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	MATE_RETURNS retorno;

	send(conexion_memoria, a_enviar, bytes, 0);
	recv(conexion_memoria, &retorno, sizeof(uint32_t), 0);

	free(a_enviar);
	eliminar_paquete(paquete);

	close(conexion_memoria);
}

void avisar_close_a_memoria(PCB* pcb) {

	int conexion_memoria = crear_conexion(CONFIG_KERNEL.ip_memoria , CONFIG_KERNEL.puerto_memoria);

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = CLOSE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &pcb->PID, sizeof(uint32_t));

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	MATE_RETURNS retorno;

	send(conexion_memoria, a_enviar, bytes, 0);
	recv(conexion_memoria, &retorno, sizeof(uint32_t), 0);

	free(a_enviar);
	eliminar_paquete(paquete);

	close(conexion_memoria);
}

/*printf("Cierro conexion con carpincho: %d\n", estructura_procesador->lugar_PCB->PID);


dar_permiso_para_continuar(estructura_procesador->lugar_PCB->conexion);

close(estructura_procesador->lugar_PCB->conexion);

pthread_mutex_lock(&mutex_lista_procesadores); // TENER CUIDADO (si se bloquea todo ver aqui)
sem_wait(&estructura_procesador->sem_exec);
estructura_procesador->bit_de_ocupado = 0;
pthread_mutex_unlock(&mutex_lista_procesadores);
sem_post(&sem_grado_multiprocesamiento);
sem_post(&sem_grado_multiprogramacion);
//TODO liberar los semaforos que le correspondan y mas cosas
 *
 *
 *
 *
 *
 */

/*if(list_any_satisfy(LISTA_BLOCKED, _criterio_remocion_lista)) {

	pthread_mutex_lock(&mutex_lista_blocked);
	list_remove_by_condition(LISTA_BLOCKED, _criterio_remocion_lista);
	pthread_mutex_unlock(&mutex_lista_blocked);

	pthread_mutex_lock(&mutex_lista_ready);
	list_add(LISTA_READY, pcb);
	pthread_mutex_unlock(&mutex_lista_ready);

	sem_post(&sem_cola_ready);
} else {
	pthread_mutex_lock(&mutex_lista_blocked_suspended);
	list_remove_by_condition(LISTA_BLOCKED_SUSPENDED, _criterio_remocion_lista);
	pthread_mutex_unlock(&mutex_lista_blocked_suspended);

	pthread_mutex_lock(&mutex_lista_ready_suspended);
	list_add(LISTA_READY_SUSPENDED, pcb);
	pthread_mutex_unlock(&mutex_lista_ready_suspended);

	sem_post(&sem_algoritmo_planificador_largo_plazo);

	//TEST
	pthread_mutex_lock(&mutex_lista_blocked_suspended);
	printf("Tamanio blocked suspended: %d\n", list_size(LISTA_BLOCKED_SUSPENDED));
	pthread_mutex_unlock(&mutex_lista_blocked_suspended);
	//FIN TEST

	//TEST
	pthread_mutex_lock(&mutex_lista_ready_suspended);
	PCB* carpinchoTest = (PCB*) list_get(LISTA_READY_SUSPENDED, 0);
	printf("PID carpincho en ready suspended: %d\n", carpinchoTest->PID);
	pthread_mutex_unlock(&mutex_lista_ready_suspended);
	//FIN TEST
}
*/

/*

bool _criterio_busqueda_semaforo(void* elemento) {
		return (strcmp(((t_semaforo_mate*)elemento)->nombre, nombre) == 0);
	}

	pthread_mutex_lock(&mutex_lista_semaforos_mate);
	t_semaforo_mate* semaforo = (t_semaforo_mate*) list_find(LISTA_SEMAFOROS_MATE, _criterio_busqueda_semaforo);

	if(semaforo->value == 0 && list_size(semaforo->cola_bloqueados) > 0) {
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
		} else {
			pthread_mutex_lock(&mutex_lista_blocked_suspended);
			list_remove_by_condition(LISTA_BLOCKED_SUSPENDED, _criterio_remocion_lista);
			pthread_mutex_unlock(&mutex_lista_blocked_suspended);

			pthread_mutex_lock(&mutex_lista_ready_suspended);
			list_add(LISTA_READY_SUSPENDED, pcb);
			pthread_mutex_unlock(&mutex_lista_ready_suspended);

			sem_post(&sem_algoritmo_planificador_largo_plazo);

			//TEST
			pthread_mutex_lock(&mutex_lista_blocked_suspended);
			printf("Tamanio blocked suspended: %d\n", list_size(LISTA_BLOCKED_SUSPENDED));
			pthread_mutex_unlock(&mutex_lista_blocked_suspended);
			//FIN TEST

			//TEST
			pthread_mutex_lock(&mutex_lista_ready_suspended);
			PCB* carpinchoTest = (PCB*) list_get(LISTA_READY_SUSPENDED, 0);
			printf("PID carpincho en ready suspended: %d\n", carpinchoTest->PID);
			pthread_mutex_unlock(&mutex_lista_ready_suspended);
			//FIN TEST
		}

	} else {
		semaforo->value += 1;
	}
	pthread_mutex_unlock(&mutex_lista_semaforos_mate);
*/
