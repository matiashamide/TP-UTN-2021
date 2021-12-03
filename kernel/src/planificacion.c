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
			pthread_mutex_unlock(&mutex_lista_ready);
		}
		pthread_mutex_unlock(&mutex_lista_ready_suspended);

		sem_post(&sem_cola_ready);
	}
}

void algoritmo_planificador_mediano_plazo_ready_suspended() {
	PCB* pcb = (PCB*) list_remove(LISTA_READY_SUSPENDED, 0);

	pthread_mutex_lock(&mutex_lista_ready);
	list_add(LISTA_READY, pcb);
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
		list_add(LISTA_BLOCKED_SUSPENDED, pcb);
		//TEST
		printf("Tamanio blocked suspended: %d\n", list_size(LISTA_BLOCKED_SUSPENDED));
		//FIN TEST
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
			destroy_sem(estructura_procesador);
			break;
		case CALL_IO:
			call_IO(estructura_procesador);
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

void avisar_finalizacion_por_deadlock(int conexion) {
	uint32_t handshake = 2;

	int numero_de_bytes = send(conexion, &handshake, sizeof(uint32_t), 0);

	printf("Finalice un carpincho por deadlock %d\n", numero_de_bytes);
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
			//TEST
			printf("Tamanio cola bloqueados semaforo: %d\n", list_size(semaforo->cola_bloqueados));
			if(list_size(estructura_procesador->lugar_PCB->recursos_usados) > 0){
				t_registro_uso_recurso* uso_recurso = list_get(estructura_procesador->lugar_PCB->recursos_usados, 0);
				printf("El carpincho esta usando el recurso %s %d veces\n", uso_recurso->nombre, uso_recurso->cantidad);
			}
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
			algoritmo_planificador_mediano_plazo_blocked_suspended();
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

	dar_permiso_para_continuar(estructura_procesador->lugar_PCB->conexion);

	//TEST
	pthread_mutex_lock(&mutex_lista_blocked);
	printf("Tamanio blocked: %d\n", list_size(LISTA_BLOCKED));
	pthread_mutex_unlock(&mutex_lista_blocked);
	//FIN TEST
}

void destroy_sem(t_procesador* estructura_procesador) {
	uint32_t size_nombre_semaforo;

	recv(estructura_procesador->lugar_PCB->conexion, &size_nombre_semaforo, sizeof(uint32_t), MSG_WAITALL);

	char* nombre = malloc(size_nombre_semaforo);

	recv(estructura_procesador->lugar_PCB->conexion, nombre, size_nombre_semaforo, MSG_WAITALL);

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
			}
		}
	}

	list_remove_by_condition(LISTA_SEMAFOROS_MATE, _criterio_busqueda_semaforo);

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

	bool _criterio_busqueda_dispositivo(void* elemento) {
			return (strcmp(((t_dispositivo*)elemento)->nombre, nombre) == 0);
		}

	pthread_mutex_lock(&mutex_lista_dispositivos_io);
	t_dispositivo* dispositivo = (t_dispositivo*) list_find(LISTA_DISPOSITIVOS_IO, _criterio_busqueda_dispositivo);
	list_add(dispositivo->cola_espera, estructura_procesador->lugar_PCB);
	sem_post(&dispositivo->sem);
	pthread_mutex_unlock(&mutex_lista_dispositivos_io);

	pthread_mutex_lock(&mutex_lista_blocked);
	list_add(LISTA_BLOCKED, estructura_procesador->lugar_PCB);
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
		usleep(dispositivo->rafaga);

		bool _criterio_remocion_lista(void* elemento) {
			return (((PCB*)elemento)->PID == pcb->PID);
		}

		if(list_any_satisfy(LISTA_BLOCKED, _criterio_remocion_lista)) {

			pthread_mutex_lock(&mutex_lista_blocked);
			list_remove_by_condition(LISTA_BLOCKED, _criterio_remocion_lista);
			pthread_mutex_unlock(&mutex_lista_blocked);

			pthread_mutex_lock(&mutex_lista_ready);
			list_add(LISTA_READY, pcb);
			printf("Tamanio ready: %d\n", list_size(LISTA_READY));
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
	}
}



void memalloc(t_procesador* estructura_procesador) {

}

void memread(t_procesador* estructura_procesador) {

}

void memfree(t_procesador* estructura_procesador) {

}

void memwrite(t_procesador* estructura_procesador) {

}

void mate_close(t_procesador* estructura_procesador) {

	printf("Cierro conexion con carpincho: %d\n", estructura_procesador->lugar_PCB->PID);


	dar_permiso_para_continuar(estructura_procesador->lugar_PCB->conexion);

	close(estructura_procesador->lugar_PCB->conexion);

	pthread_mutex_lock(&mutex_lista_procesadores); // TENER CUIDADO (si se bloquea todo ver aqui)
	sem_wait(&estructura_procesador->sem_exec);
	estructura_procesador->bit_de_ocupado = 0;
	pthread_mutex_unlock(&mutex_lista_procesadores);

	for(int k = 0; k < list_size(estructura_procesador->lugar_PCB->recursos_usados); k++) {

		t_registro_uso_recurso* recurso_usado = list_get(estructura_procesador->lugar_PCB->recursos_usados, k);

		printf("Recurso usado: %s\n", recurso_usado->nombre);

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
		usleep(CONFIG_KERNEL.tiempo_deadlock);
		algoritmo_deteccion_deadlock();
	}
}


void algoritmo_deteccion_deadlock() {

	int i;
	int j;
	t_list* lista_bloqueados_por_semaforo = list_create();
	t_list* lista_procesos_en_deadlock = list_create();

	printf("CORRIENDO DETECCION DEADLOCK\n");

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
					list_add(lista_procesos_en_deadlock, pcb);
					//TEST
					printf("El carpincho %d esta en deadlock\n", pcb->PID);
					//FIN TEST
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
		printf("ENCONTRE UN DEADLOCK AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
		algoritmo_deteccion_deadlock();

	} else {
		printf("NO ENCONTRE UN DEADLOCK BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB\n");
		pthread_mutex_unlock(&mutex_lista_semaforos_mate);

	}

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
