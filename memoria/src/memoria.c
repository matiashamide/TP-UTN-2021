/*
 ============================================================================
 Name        : memoria.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "memoria.h"

int main(void) {

	init_memoria();

	printf("%i" , sizeof(heap_metadata));
	//enviar_mensaje("hola como estas", SERVIDOR_MEMORIA);
	//coordinador_multihilo();

	//memalloc(10,0);

	signal(SIGINT, &print_SIGINT);

	return EXIT_SUCCESS;
}


//--------------------------------------------------------------------------------------------

void init_memoria(){

	//Inicializamos logger
	LOGGER = log_create("memoria.log", "MEMORIA", 0, LOG_LEVEL_INFO);

	//Levantamos archivo de configuracion
	CONFIG = crear_archivo_config_memoria("/home/utnso/workspace/tp-2021-2c-DesacatadOS/memoria/src/memoria.config");

	//Iniciamos servidor
	SERVIDOR_MEMORIA = iniciar_servidor(CONFIG.ip_memoria,CONFIG.puerto_memoria);

	//Instanciamos memoria principal
	MEMORIA_PRINCIPAL = malloc(CONFIG.tamanio_memoria);

	if (MEMORIA_PRINCIPAL == NULL) {
	   	perror("MALLOC FAIL!\n");
	   	log_info(LOGGER,"No se pudo alocar correctamente la memoria principal.");
        return;
	}

	//Iniciamos paginacion
	iniciar_paginacion();
}


void coordinador_multihilo(){

	while(1){

		int socket = esperar_cliente(SERVIDOR_MEMORIA);

		pthread_t* hilo_atender_carpincho = malloc(sizeof(pthread_t));
		pthread_create(hilo_atender_carpincho, NULL, (void*)atender_carpinchos, (void*)socket);
		pthread_detach(*hilo_atender_carpincho);
	}
}

void atender_carpinchos(int cliente) {

	peticion_carpincho operacion = recibir_operacion(cliente);

	switch (operacion) {

	case MEMALLOC:;
		/*int* pid  = malloc(4);
		pid = recibir_entero();
		size = recibir_entero();
		*/

		int size = 0;
		int pid = 0;

		memalloc(size , pid);
	break;

	case MEMREAD:;
	break;

	case MEMFREE:;
	break;

	case MEMWRITE:;
	break;

	case MENSAJE:;
		char* mensaje = recibir_mensaje(cliente);
		printf("%s",mensaje);
		fflush(stdout);
	break;

	//Faltan algunos cases;
	default:; break;

	}
}

void iniciar_paginacion() {

	int cant_frames_ppal = CONFIG.tamanio_memoria / CONFIG.tamanio_pagina;

	log_info(LOGGER, "Tengo %d marcos de %d bytes en memoria principal", cant_frames_ppal, CONFIG.tamanio_pagina);

	//Creamos lista de tablas de paginas
	TABLAS_DE_PAGINAS = list_create();

	//Creamos lista de frames de memoria ppal.
	MARCOS_MEMORIA = list_create();

	for (int i = 0 ; i < cant_frames_ppal ; i++) {

		t_frame* frame = malloc(sizeof(t_frame));
		frame->ocupado = 0;
		frame->id = i;

		list_add(MARCOS_MEMORIA, frame);
	}
}

int memalloc(int size , int pid) {

	//Si el proceso no tiene nada guardado en memoria
	if (list_get(TABLAS_DE_PAGINAS, pid) == NULL) {

		//Crea tabla de paginas para el proceso
		t_tabla_pagina* nueva_tabla = malloc(sizeof(t_tabla_pagina));
		nueva_tabla->paginas = list_create();
		nueva_tabla->PID = pid;

		//Chequear que el size a guardar sea menor a la cantidad max de marcos permitida
		int marcos_necesarios = ceil(size / CONFIG.tamanio_pagina);

		if (marcos_necesarios > CONFIG.marcos_max) {

			printf("No se puede asginar %i bytes cantidad de memoria al proceso %i (cant maxima) " , size , pid);
			log_info(LOGGER , "No se puede asginar %i bytes cantidad de memoria al proceso %i (cant maxima) " , size , pid);

			return EXIT_FAILURE;
		}

		//Verificar si entra en memoria y solicitamos marcos
		t_list* marcos_a_ocupar = obtener_marcos(marcos_necesarios);

		if (marcos_a_ocupar == NULL) {
			printf(" No hay mas espacio en la memoria para el proceso %i ", pid);
			log_info(LOGGER," No hay mas espacio en la memoria para el proceso %i ", pid);
			return EXIT_FAILURE;
		}

		//Se agregan los marcos a ocupar en la tabla de paginas del proceso
		for (int i = 0 ; i < list_size(marcos_a_ocupar) ; i++) {
			t_pagina* pagina = malloc(sizeof(t_pagina));

			pagina->frame_ppal = ((t_frame*)list_get(marcos_a_ocupar, i))-> id;
			list_add(nueva_tabla->paginas, pagina);
		}

		pthread_mutex_lock(&mutexTablas);
		list_add(TABLAS_DE_PAGINAS, nueva_tabla);
		pthread_mutex_unlock(&mutexTablas);

		//inicializo header inicial
		heap_metadata* header = malloc(sizeof(heap_metadata));
		header->is_free = false;
		header->next_alloc = sizeof(heap_metadata) + size;
		header->prev_alloc = NULL;

		//armo el alloc siguiente
		heap_metadata* header_siguiente = malloc(sizeof(heap_metadata));
		header->is_free = true;
		header->next_alloc = NULL;
		header->prev_alloc = 0;

		//Bloque en donde se meteran los header
		void* marquinhos = malloc(marcos_necesarios * CONFIG.tamanio_pagina);

		memcpy(marquinhos, header, sizeof(heap_metadata));
		memcpy(marquinhos + sizeof(heap_metadata) + size, header_siguiente, sizeof(heap_metadata));

		//Copia del bloque 'marquinhos' a memoria;
		serializar_paginas_en_memoria(nueva_tabla->paginas, marquinhos);

		free(header);
		free(header_siguiente);
	}

	else
	// Si el proceso ya existe en memoria
	{
		void* marquinhos = traer_marquinhos_del_proceso(pid);
		heap_metadata* header;
		uint32_t i = 0;

		header = desserializar_header(marquinhos + i);

		t_list* tabla_proceso  = ((t_tabla_pagina*) list_get(TABLAS_DE_PAGINAS, pid))->paginas;

		do {

			i = header->next_alloc;

			if (header->is_free && header->next_alloc != NULL){
				heap_metadata* header_siguiente = desserializar_header(marquinhos + header->next_alloc);

				if (size == header->next_alloc - header_siguiente->prev_alloc - sizeof(heap_metadata)){
					//guardo directamente
					header->is_free = false;
					memcpy(marquinhos + header_siguiente->prev_alloc, header, sizeof(heap_metadata));

				}
				else if(size + sizeof(heap_metadata) < header->next_alloc - header_siguiente->prev_alloc - sizeof(heap_metadata)){

					header->is_free = false;

					//Creo header y guardo
					heap_metadata* nuevo_header = malloc(sizeof(heap_metadata));

					//Inicializo header
					nuevo_header->is_free = true;
					nuevo_header->next_alloc = header->next_alloc;
					nuevo_header->prev_alloc = header_siguiente->prev_alloc;

					//Actualizo el header anterior al nuevo
					header->next_alloc = header_siguiente->prev_alloc + size  + sizeof(heap_metadata);

					//Actualizo el header posterior al nuevo
					header_siguiente->prev_alloc = header->next_alloc;

					memcpy(marquinhos + nuevo_header->prev_alloc, header, sizeof(heap_metadata));
					memcpy(marquinhos + header->next_alloc, nuevo_header, sizeof(heap_metadata));
					memcpy(marquinhos + nuevo_header->next_alloc, header_siguiente, sizeof(heap_metadata));

					free(nuevo_header);

				}
				free(header_siguiente);
				break;
			}
			else if(header->next_alloc == NULL && header->is_free){
				//caso que el ultimo sea NULL

				heap_metadata* header_anterior = desserializar_header(marquinhos + header->prev_alloc);

				int espacio_total = sizeof(marquinhos);

				if(espacio_total - header_anterior->next_alloc + sizeof(heap_metadata) == size){
					header->is_free = false;
					memcpy(marquinhos + header_anterior->next_alloc , header , sizeof(heap_metadata) );

					break;
				}
				if( size + 9 > espacio_total - header_anterior->next_alloc + sizeof(heap_metadata)  ){

					heap_metadata* nuevo_header = malloc(sizeof(heap_metadata));

					//actualizo el header original
					header->is_free = false;
					header->next_alloc = header_anterior->next_alloc + sizeof(heap_metadata) + size;

					//Inicializo header nuevo
					nuevo_header->is_free = true;
					nuevo_header->prev_alloc = header_anterior->next_alloc;
					nuevo_header->next_alloc = NULL;

					//lo meto a memoria el actualizado
					memcpy(marquinhos + header_anterior->next_alloc , header , sizeof(heap_metadata));
					//lo meto a memoria el nuevo
					memcpy(marquinhos + header->next_alloc , nuevo_header , sizeof(heap_metadata));

					free(nuevo_header);
					break;
				}
			}


			serializar_paginas_en_memoria(tabla_proceso , marquinhos);
		}

		while(header->next_alloc != NULL );
		free(header);
		//crear paginas nuevas xq no habia
	}
	return 0;
}

void* traer_marquinhos_del_proceso(int pid) {

	t_list* paginas_proceso = ((t_tabla_pagina*)list_get(TABLAS_DE_PAGINAS, pid))->paginas;

	void* buffer = malloc(sizeof(CONFIG.tamanio_pagina) * list_size(paginas_proceso));

	for (int i = 0 ; i < list_size(paginas_proceso) ; i++) {
		int frame_aux = ((t_pagina*)list_get(paginas_proceso, i))->frame_ppal;
		memcpy(buffer + i * CONFIG.tamanio_pagina, MEMORIA_PRINCIPAL + frame_aux * CONFIG.tamanio_pagina, CONFIG.tamanio_pagina);
	}

	return buffer;
}

heap_metadata* desserializar_header(void* buffer) {
	int offset = 0;
	heap_metadata* header = malloc(sizeof(heap_metadata));

	memcpy(&(header->prev_alloc), buffer + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(&(header->next_alloc), buffer + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(&(header->is_free), buffer + offset, sizeof(uint8_t));
	return header;
}

void serializar_paginas_en_memoria(t_list* paginas , void* contenido) {

	for (int i = 0 ; i< list_size(paginas); i++) {

		int offset = ((t_pagina*)list_get(paginas , i))->frame_ppal * CONFIG.tamanio_pagina ;

		memcpy(MEMORIA_PRINCIPAL + offset, contenido + i * CONFIG.tamanio_pagina, CONFIG.tamanio_pagina);
	}

	return;
}

t_list* obtener_marcos(int cant_marcos) {

	bool marco_libre(t_frame* marco) {
		return marco->ocupado;
	}

	void ocupar_frame(t_frame* marco) {
		marco->ocupado = 1;
	}

	pthread_mutex_lock(&mutexMarcos);
	t_list* marcos_libres = list_filter(MARCOS_MEMORIA, (void*)marco_libre);

	if (list_size(marcos_libres) < cant_marcos) {
		pthread_mutex_unlock(&mutexMarcos);
		return NULL;
	}

	t_list* marcos_asignados = list_take(marcos_libres, cant_marcos);

	for (int i = 0 ; i < list_size(marcos_asignados) ; i++) {
		ocupar_frame((t_frame*)list_take(marcos_asignados , i));
	}
	pthread_mutex_unlock(&mutexMarcos);

	return marcos_asignados;
}

t_memoria_config crear_archivo_config_memoria(char* ruta) {
    t_config* memoria_config;
    memoria_config = config_create(ruta);
    t_memoria_config config;

    if (memoria_config == NULL) {
        log_info(LOGGER, "No se pudo leer el archivo de configuracion de memoria\n");
        exit(-1);
    }

    config.ip_memoria = config_get_string_value(memoria_config, "IP");
    config.puerto_memoria = config_get_string_value(memoria_config, "PUERTO");
    config.tamanio_memoria = config_get_int_value(memoria_config, "TAMANIO");
    config.tamanio_pagina = config_get_int_value(memoria_config, "TAMANIO_PAGINA");
    config.alg_remp_mmu = config_get_string_value(memoria_config, "ALGORITMO_REEMPLAZO_MMU");
    config.tipo_asignacion = config_get_string_value(memoria_config, "TIPO_ASIGNACION");
    //config.marcos_max = config_get_int_value(memoria_config, "MARCOS_MAXIMOS");  --> ver en que casos esta esta esta config y meter un if
    config.cant_entradas_tlb = config_get_int_value(memoria_config, "CANTIDAD_ENTRADAS_TLB");
    config.alg_reemplazo_tlb = config_get_string_value(memoria_config, "ALGORITMO_REEMPLAZO_TLB");
    config.retardo_acierto_tlb = config_get_int_value(memoria_config, "RETARDO_ACIERTO_TLB");
    config.retardo_fallo_tlb = config_get_int_value(memoria_config, "RETARDO_FALLO_TLB");

    return config;
}

