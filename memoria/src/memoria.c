#include "memoria.h"

int main(void) {

	init_memoria();
	log_info(LOGGER, "Inicializa memoria");
	log_info(LOGGER, "Ya me conecte con swamp y los marcos max son: %i \n", MAX_FRAMES_SWAP);


	//coordinador_multihilo();

	int a0 = memalloc(0,10,1);

	int a1 = memalloc(1,10,1);

	int a2 = memalloc(0,20000,1);

	int a3 = memalloc(1,4,1);

	int a4 = memalloc(0, 10,1);

	/*
	int a3 = memalloc(0, 12,1);
	int a4 = memalloc(0, 12,1);
	int a5 = memalloc(0, 40,1);
	int a6 = memalloc(0,31,1);
	 */
	return EXIT_SUCCESS;
}


//--------------------------------------------------- INICIALIZACION MEMORIA ---------------------------------------------//

void init_memoria() {

	//Levantamos archivo de configuracion
	CONFIG = crear_archivo_config_memoria("/home/utnso/workspace/tp-2021-2c-DesacatadOS/memoria/src/memoria.config");

	//Inicializamos logger
	LOGGER = log_create("/home/utnso/workspace/tp-2021-2c-DesacatadOS/memoria/MEMORIA.log", "MEMORIA", 0, LOG_LEVEL_INFO);

	//Iniciamos servidor
	SERVIDOR_MEMORIA = iniciar_servidor(CONFIG.ip_memoria, CONFIG.puerto_memoria);

	//Instanciamos memoria principal
	MEMORIA_PRINCIPAL = malloc(CONFIG.tamanio_memoria);

	//Inicializamos tlb
	init_tlb();

	//Inicializamos PIDS_CONEXION
	PIDS_CLIENTE = list_create();

	if (MEMORIA_PRINCIPAL == NULL) {
	   	perror("MALLOC FAIL!\n");
	   	log_info(LOGGER,"No se pudo alocar correctamente la memoria principal.");
        return;
	}

	CONEXION_SWAP   = crear_conexion(CONFIG.ip_swap, CONFIG.puerto_swap);
	MAX_FRAMES_SWAP = solicitar_marcos_max_swap();

	//Inicializamos PID_GLOBAL
	PID_GLOBAL = 0;

	//Senales
	signal(SIGINT,  &signal_metricas);
	signal(SIGUSR1, &signal_dump);
	signal(SIGUSR2, &signal_clean_tlb);

	//Iniciamos paginacion
	iniciar_paginacion();
}

void iniciar_paginacion() {

	int cant_frames_ppal = CONFIG.tamanio_memoria / CONFIG.tamanio_pagina;

	log_info(LOGGER, "Tengo %d frames de %d bytes en memoria principal", cant_frames_ppal, CONFIG.tamanio_pagina);

	//Creamos lista de tablas de paginas
	TABLAS_DE_PAGINAS = list_create();

	//Creamos lista de frames de memoria ppal.
	FRAMES_MEMORIA = list_create();

	for (int i = 0 ; i < cant_frames_ppal ; i++) {

		t_frame* frame = malloc(sizeof(t_frame));
		frame->ocupado = false;
		frame->id      = i;
		frame->pid     = -1;

		list_add(FRAMES_MEMORIA, frame);
	}

	POSICION_CLOCK = 0;
}

t_memoria_config crear_archivo_config_memoria(char* ruta) {
    t_config* memoria_config;
    memoria_config = config_create(ruta);
    t_memoria_config config;

    if (memoria_config == NULL) {
        log_info(LOGGER, "No se pudo leer el archivo de configuracion de memoria\n");
        exit(-1);
    }

    config.ip_memoria          = config_get_string_value(memoria_config, "IP");
    config.puerto_memoria      = config_get_string_value(memoria_config, "PUERTO");
    config.ip_swap             = config_get_string_value(memoria_config, "IP_SWAP");
	config.puerto_swap         = config_get_string_value(memoria_config, "PUERTO_SWAP");
    config.tamanio_memoria     = config_get_int_value   (memoria_config, "TAMANIO");
    config.tamanio_pagina      = config_get_int_value   (memoria_config, "TAMANIO_PAGINA");
    config.alg_remp_mmu        = config_get_string_value(memoria_config, "ALGORITMO_REEMPLAZO_MMU");
    config.tipo_asignacion     = config_get_string_value(memoria_config, "TIPO_ASIGNACION");

    if (string_equals_ignore_case(config.tipo_asignacion,"FIJA")) {
    	config.marcos_max      = config_get_int_value   (memoria_config, "MARCOS_POR_CARPINCHO");
    }

    config.cant_entradas_tlb   = config_get_int_value   (memoria_config, "CANTIDAD_ENTRADAS_TLB");
    config.alg_reemplazo_tlb   = config_get_string_value(memoria_config, "ALGORITMO_REEMPLAZO_TLB");
    config.retardo_acierto_tlb = config_get_int_value   (memoria_config, "RETARDO_ACIERTO_TLB");
    config.retardo_fallo_tlb   = config_get_int_value   (memoria_config, "RETARDO_FALLO_TLB");
    config.path_dump_tlb       = config_get_string_value(memoria_config, "PATH_DUMP_TLB");
    config.kernel_existe	   = config_get_int_value   (memoria_config, "KERNEL_EXISTE");

    return config;
}

//--------------------------------------------------------- COORDINACION DE CARPINCHOS ----------------------------------------------------------//

void coordinador_multihilo(){

	while(1) {

		pthread_t hilo_atender_carpincho;

		int *socket_cliente = malloc(sizeof(int));
		(*socket_cliente)   = accept(SERVIDOR_MEMORIA, NULL, NULL);

		log_info(LOGGER, "Se conecto un cliente %i\n", *socket_cliente);

		pthread_create(&hilo_atender_carpincho, NULL, (void*)atender_carpinchos, socket_cliente);
		pthread_detach(hilo_atender_carpincho);
	}
}

void recibir_peticion_para_continuar(int conexion) {

	uint32_t result;
	int bytes_recibidos = recv(conexion, &result, sizeof(uint32_t), MSG_WAITALL);

	printf("Ya recibi peticion %d\n", bytes_recibidos);

}

void dar_permiso_para_continuar(int conexion){

	uint32_t handshake  = 1;
	int numero_de_bytes = send(conexion, &handshake, sizeof(uint32_t), 0);
	printf("Ya di permiso %d\n", numero_de_bytes);

}

void atender_carpinchos(int* cliente) {

	int existe_kernel = CONFIG.kernel_existe;

	if (!existe_kernel) {
		recibir_peticion_para_continuar((*cliente));
		dar_permiso_para_continuar((*cliente));
	}

	while(1) {

		peticion_carpincho operacion = recibir_operacion(*cliente);
		int size_paquete = recibir_entero(*cliente);
		int pid, retorno, dir_logica, size_contenido;

		switch (operacion) {

		case MEMALLOC:;

			if (existe_kernel) {
				pid = recibir_entero(*cliente);
			} else {
				pid = pid_por_conexion(*cliente);
			}

			int size = recibir_entero(*cliente);
			log_info(LOGGER, "MEMALLOC: el cliente %i solicito alocar memoria de %i bytes", *cliente, size);

			retorno  = memalloc(pid, size, *cliente);
			send(*cliente, &retorno, sizeof(uint32_t), 0);

		break;

		case MEMREAD:;

			if (existe_kernel) {
				pid = recibir_entero(*cliente);
			} else {
				pid = pid_por_conexion(*cliente);
			}

			log_info(LOGGER, "MEMREAD: El cliente %i solicito leer memoria.", *cliente);

			dir_logica     = recibir_entero(*cliente);
			size_contenido = recibir_entero(*cliente);

			void* dest = malloc(size_contenido);

			retorno = memread(pid, dir_logica, dest, size_contenido);

			if (retorno == -1) {
				retorno = MATE_READ_FAULT;
			}

			send(*cliente, &retorno, sizeof(uint32_t), 0);
			send(*cliente, dest, size_contenido, 0);
			free(dest);

		break;

		case MEMFREE:;

			if (existe_kernel) {
				pid = recibir_entero(*cliente);
			} else {
				pid = pid_por_conexion(*cliente);
			}

			log_info(LOGGER, "MEMFREE: El cliente %i solicito liberar memoria.", *cliente);

			dir_logica = recibir_entero(*cliente);

			retorno    = memfree(pid, dir_logica);

			if (retorno == -1) {
				retorno = MATE_FREE_FAULT;
			}

			send(*cliente, &retorno, sizeof(uint32_t), 0);

		break;

		case MEMWRITE:;

			if (existe_kernel) {
				pid = recibir_entero(*cliente);
			} else {
				pid = pid_por_conexion(*cliente);
			}

			log_info(LOGGER, "MEMWRITE: El cliente %i solicito escribir memoria.", *cliente);

			dir_logica     = recibir_entero(*cliente);
			size_contenido = recibir_entero(*cliente);

			void* contenido = malloc(size_contenido);
			recv(*cliente, contenido, size_contenido, 0);

			retorno = memwrite(pid, contenido, dir_logica, size_paquete - sizeof(int)*2);

			if(retorno == -1){
				retorno = MATE_WRITE_FAULT;
			}

			send(*cliente, &retorno, sizeof(uint32_t), 0);
			free(contenido);

		break;

		case MEMSUSP:;

			log_info(LOGGER, "MEMSUSP: El cliente %i solicito suspender el proceso.", *cliente);
			pid = recibir_entero(*cliente);
			suspender_proceso(pid);

		break;

		case MEMDESSUSP:;

			log_info(LOGGER, "MEMDESSUSP: El cliente %i solicito dessuspender el proceso.", *cliente);
			pid = recibir_entero(*cliente);
			dessuspender_proceso(pid);

		break;

		case CLOSE:;

			log_info(LOGGER, "Se desconecto el cliente %i\n", *cliente);

			if (existe_kernel) {
				pid = recibir_entero(*cliente);
			} else {
				pid = pid_por_conexion(*cliente);
			}

			eliminar_proceso(pid);

			if (!existe_kernel) {
				dar_permiso_para_continuar((*cliente));
			}

		break;

		case MENSAJE:;

			char* mensaje = recibir_mensaje(*cliente);
			printf("%s",mensaje);
			fflush(stdout);

		break;

		default:; log_info(LOGGER, "No se entendio la solicitud del cliente %i.", *cliente);
		break;

		}
	}
}

//--------------------------------------------------- FUNCIONES PRINCIPALES ----------------------------------------------//

int memalloc(int pid, int size, int cliente){
	int dir_logica = -1;
	int paginas_necesarias = ceil(((double)size + sizeof(heap_metadata)*2)/ (double)CONFIG.tamanio_pagina);

	//[CASO A]: Llega un proceso nuevo
	if (tabla_por_pid(pid) == NULL){

		if(pid == -1){
			pthread_mutex_lock(&mutex_PID);
			pid = PID_GLOBAL;
			PID_GLOBAL++;
			pthread_mutex_unlock(&mutex_PID);

			t_pid_cliente* pid_cliente = malloc(sizeof(t_pid_cliente));
			pid_cliente->cliente = cliente;
			pid_cliente->pid = pid;
			list_add(PIDS_CLIENTE, pid_cliente);
		}

		log_info(LOGGER, "Inicializando el proceso %i..." , pid);


		if (reservar_espacio_en_swap(pid, paginas_necesarias) == -1) {
			log_info(LOGGER, "Error: no se pudo asginar %i bytes al proceso %i.", size, pid);
			return -1;
		}

		if (string_equals_ignore_case(CONFIG.tipo_asignacion, "FIJA")) {

			pthread_mutex_lock(&mutex_frames);

			if (!hay_frames_libres_mp(CONFIG.marcos_max)) {
				log_info(LOGGER, "Error: se supero el nivel de multiprogramacion, no hay frames libes en MP.", size, pid);
				return -1;
			}

			//Reservamos los frames al PID
			t_list* frames_libres_MP = list_filter(FRAMES_MEMORIA, (void*)esta_libre_y_desasignado);

			for (int i = 0; i < CONFIG.marcos_max; i++){
				t_frame* frame_libre = list_get(frames_libres_MP,i);
				frame_libre->pid = pid;
			}

			pthread_mutex_unlock(&mutex_frames);
		}

		//Crea tabla de paginas para el proceso
		t_tabla_pagina* nueva_tabla = malloc(sizeof(t_tabla_pagina));
		nueva_tabla->paginas = list_create();
		nueva_tabla->PID     = pid;

		pthread_mutex_lock(&mutex_tablas_dp);
		list_add(TABLAS_DE_PAGINAS, nueva_tabla);
		pthread_mutex_unlock(&mutex_tablas_dp);

		//inicializo header inicial
		heap_metadata* header 	  = malloc(sizeof(heap_metadata));
		header->is_free           = false;
		header->next_alloc        = sizeof(heap_metadata) + size;
		header->prev_alloc        = NULL;

		//Armo el alloc siguiente
		heap_metadata* header_sig = malloc(sizeof(heap_metadata));
		header_sig->is_free       = true;
		header_sig->next_alloc 	  = NULL;
		header_sig->prev_alloc    = 0;

		//Bloque de paginas en donde se meten los headers
		void* buffer_pags_proceso = malloc(paginas_necesarias * CONFIG.tamanio_pagina);

		memcpy(buffer_pags_proceso, header, sizeof(heap_metadata));
		memcpy(buffer_pags_proceso + sizeof(heap_metadata) + size, header_sig, sizeof(heap_metadata));

		//Crea las paginas y las guarda en memoria
		for (int i = 0 ; i < paginas_necesarias ; i++) {

			t_pagina* pagina      = malloc(sizeof(t_pagina));
			pagina->pid 		  = pid;
			pagina->id  		  = i;
			pagina->frame_ppal    = solicitar_frame_en_ppal(pid);
			pagina->modificado    = 1;
			pagina->lock          = 1;
			pagina->presencia     = 1;
			pagina->tiempo_uso    = obtener_tiempo();
			pagina->uso           = 0;

			list_add(nueva_tabla->paginas, pagina);

			memcpy(MEMORIA_PRINCIPAL + pagina->frame_ppal * CONFIG.tamanio_pagina, buffer_pags_proceso + i * CONFIG.tamanio_pagina, CONFIG.tamanio_pagina);
			unlockear(pagina);
		}

		dir_logica = header_sig->prev_alloc;
		free(header);
		log_info(LOGGER, "El proceso %i fue inicializado correctamente." , pid);

	} else {

	//[CASO B]: El proceso existe en memoria
		log_info(LOGGER, "Alocando %i bytes de memoria para el proceso %i.", size, pid);
		t_alloc_disponible* alloc = obtener_alloc_disponible(pid, size, 0);

		if(alloc->flag_ultimo_alloc){

			t_list* paginas_proceso = tabla_por_pid(pid)->paginas;
			int tamanio_total = list_size(paginas_proceso) * CONFIG.tamanio_pagina;
			int size_necesario_extra = size + sizeof(heap_metadata) - (tamanio_total - alloc->direc_logica - sizeof(heap_metadata));
			int cantidad_paginas = ceil((double)size_necesario_extra/(double)CONFIG.tamanio_pagina);

			if (string_equals_ignore_case(CONFIG.tipo_asignacion, "FIJA")) {

				if (list_size(tabla_por_pid(pid)->paginas) + cantidad_paginas >= MAX_FRAMES_SWAP){
					log_info(LOGGER, "Error: el proceso alcanzo su cantidad maxima de frames.", size, pid);
					return -1;

				} else {

					reservar_espacio_en_swap(pid, cantidad_paginas);
				}
			}

			if (string_equals_ignore_case(CONFIG.tipo_asignacion, "DINAMICA")) {

				if (reservar_espacio_en_swap(pid, cantidad_paginas) == -1 ) {
					log_info(LOGGER, "Error: no se pueden alocar %i bytes de memoria al proceso %i ya que no hay espacio suficiente en swap.", size, pid);
					return -1;
				}
			}

			int nro_pagina,offset;
			nro_pagina = list_size(paginas_proceso) - 1;
			offset = alloc->direc_logica - CONFIG.tamanio_pagina * nro_pagina;

			//Actualizamos el ultimo header
			heap_metadata* ultimo_header = desserializar_header(pid,nro_pagina, offset);
			ultimo_header->is_free       = false;
			ultimo_header->next_alloc    = alloc->direc_logica + sizeof(heap_metadata) + size;

			//Creamos nuevo header
			int nro_pagina_nueva, offset_nuevo;
			nro_pagina_nueva = floor((double)ultimo_header->next_alloc/(double)CONFIG.tamanio_pagina);

			double dir_fisica = (double)ultimo_header->next_alloc / (double)CONFIG.tamanio_pagina;
			offset_nuevo      = (int)((dir_fisica - nro_pagina_nueva) * CONFIG.tamanio_pagina);

			heap_metadata* nuevo_header = malloc(sizeof(heap_metadata));
			nuevo_header->is_free       = true;
			nuevo_header->prev_alloc    = alloc->direc_logica;
			nuevo_header->next_alloc    = NULL;

			int id_ult_pag = ((t_pagina*)list_get(tabla_por_pid(pid)->paginas, nro_pagina))->id;

			for (int i = 1 ; i <= cantidad_paginas ; i++) {

				t_pagina* pagina      = malloc(sizeof(t_pagina));

				lockear(pagina);
				pagina->pid 		  = pid;
				pagina->id  		  = id_ult_pag + i;
				pagina->frame_ppal    = solicitar_frame_en_ppal(pid);
				pagina->modificado    = 1;
				pagina->lock          = 1;
				pagina->presencia     = 1;
				pagina->tiempo_uso    = obtener_tiempo();
				pagina->uso           = 0;

				list_add(paginas_proceso, pagina);
				unlockear(pagina);
			}

			dir_logica = ultimo_header->prev_alloc;
			guardar_header(pid, nro_pagina, offset, ultimo_header);
			guardar_header(pid, nro_pagina_nueva , offset_nuevo, nuevo_header);

			free(ultimo_header);
			free(nuevo_header);

		}
		dir_logica = alloc->direc_logica;
	}

	log_info(LOGGER, "Se le asignaron correctamente %i bytes al proceso %i.", size, pid);
	return dir_logica + sizeof(heap_metadata);
}

int memfree(int pid, int dir_logica){

t_list* paginas_proceso = tabla_por_pid(pid)->paginas;

	if (paginas_proceso == NULL || list_size(paginas_proceso) == 0) {
		return MATE_FREE_FAULT;
	}

	int pag_en_donde_empieza_el_header = floor(((double)dir_logica - sizeof(heap_metadata)) / (double)CONFIG.tamanio_pagina);
	int offset_header = (double)(((dir_logica - sizeof(heap_metadata)) / (double)CONFIG.tamanio_pagina) - pag_en_donde_empieza_el_header) * CONFIG.tamanio_pagina;
	heap_metadata* header_a_liberar = desserializar_header(pid, pag_en_donde_empieza_el_header, offset_header);

	if (header_a_liberar->next_alloc == NULL){
		log_info(LOGGER,"Error: no se puede liberar esta posicion.");
		free(header_a_liberar);
		return MATE_FREE_FAULT;
	}

	int pag_en_donde_empieza_el_header_siguiente = floor((double)(header_a_liberar->next_alloc) / (double)CONFIG.tamanio_pagina);
	int offset_header_siguiente = (double)(((header_a_liberar->next_alloc) / (double)CONFIG.tamanio_pagina) - pag_en_donde_empieza_el_header_siguiente) * CONFIG.tamanio_pagina;
	heap_metadata* header_siguiente = desserializar_header(pid, pag_en_donde_empieza_el_header_siguiente, offset_header_siguiente);

	header_a_liberar->is_free = true;

	//CASO A :: Caso en que queremos liberar primer alloc
	if (header_a_liberar->prev_alloc == NULL && header_siguiente->prev_alloc == 0) {

		//CASO A.1 :: caso en el que el siguiente header este ocupado (OK)
		if(!header_siguiente->is_free){
			guardar_header(pid, pag_en_donde_empieza_el_header, offset_header, header_a_liberar);
			liberar_si_hay_paginas_libres(pid, sizeof(heap_metadata), header_a_liberar->next_alloc);
		}

		//CASO A.2 :: caso en el que el siguiente header este libre (OK)
		if(header_siguiente->is_free){

			header_a_liberar->next_alloc = header_siguiente->next_alloc;
			guardar_header(pid, pag_en_donde_empieza_el_header, offset_header, header_a_liberar);

			int pagina_post_siguiente = floor((double)header_siguiente->next_alloc / (double)CONFIG.tamanio_pagina);
			int offset_header_post_siguiente = (double)(((header_siguiente->next_alloc) / (double)CONFIG.tamanio_pagina) - pagina_post_siguiente) * CONFIG.tamanio_pagina;

			heap_metadata* header_post_siguiente = desserializar_header(pid, pagina_post_siguiente, offset_header_post_siguiente);
			header_post_siguiente->prev_alloc = header_siguiente->prev_alloc;
			guardar_header(pid, pagina_post_siguiente, offset_header_post_siguiente, header_post_siguiente);

			liberar_si_hay_paginas_libres(pid, sizeof(heap_metadata), header_a_liberar->next_alloc);
			free(header_post_siguiente);
		}

	}

	//CASO B :: Caso en que hay header anterior y header siguiente
	else {

		int pag_en_donde_empieza_el_header_anterior = floor((header_a_liberar->prev_alloc) / CONFIG.tamanio_pagina);
		int offset_header_anterior = (double)(((header_a_liberar->prev_alloc) / (double)CONFIG.tamanio_pagina) - pag_en_donde_empieza_el_header_anterior) * CONFIG.tamanio_pagina;
		heap_metadata* header_anterior = desserializar_header(pid , pag_en_donde_empieza_el_header_anterior , offset_header_anterior);

		//CASO B.1 :: caso en el que los headers  esten ocupados (OK)
		if(!header_anterior->is_free && !header_siguiente->is_free){
			guardar_header(pid, pag_en_donde_empieza_el_header, offset_header, header_a_liberar);
			liberar_si_hay_paginas_libres( pid , header_anterior->next_alloc + sizeof(heap_metadata), header_a_liberar->next_alloc);
		}

		//CASO B.2 :: caso en el que el header anterior este libre (OK)
		if(header_anterior->is_free && !header_siguiente->is_free){

			header_anterior->next_alloc = header_a_liberar->next_alloc;
			guardar_header(pid , pag_en_donde_empieza_el_header_anterior, offset_header_anterior , header_anterior);

			header_siguiente->prev_alloc = header_a_liberar->prev_alloc;
			guardar_header(pid , pag_en_donde_empieza_el_header_siguiente, offset_header_siguiente , header_siguiente);

			liberar_si_hay_paginas_libres(pid, header_a_liberar->prev_alloc + sizeof(heap_metadata) , header_a_liberar->next_alloc );

		}

		//CASO B.3 :: caso en el que el header siguiente este libre (OK)
		if(!header_anterior->is_free && header_siguiente->is_free){

			header_a_liberar->next_alloc = header_siguiente->next_alloc;
			guardar_header(pid, pag_en_donde_empieza_el_header, offset_header, header_a_liberar);

			int pag_post_siguiente = floor((double)header_siguiente->next_alloc / (double)CONFIG.tamanio_pagina);
			int offset_header_post_siguiente = (double)(((header_siguiente->next_alloc) / (double)CONFIG.tamanio_pagina) - pag_post_siguiente) * CONFIG.tamanio_pagina;

			heap_metadata* header_post_siguiente = desserializar_header(pid, pag_post_siguiente, offset_header_post_siguiente);
			header_post_siguiente->prev_alloc = header_siguiente->prev_alloc;
			guardar_header(pid, pag_post_siguiente, offset_header_post_siguiente, header_post_siguiente);

			liberar_si_hay_paginas_libres(pid, header_siguiente->prev_alloc + sizeof(heap_metadata), header_siguiente->next_alloc);
			free(header_post_siguiente);
		}

		//CASE B.4 :: caso en el que ambos headers estan libres
		if(header_anterior->is_free && header_siguiente->is_free){

			header_anterior->next_alloc = header_siguiente->next_alloc;
			guardar_header(pid , pag_en_donde_empieza_el_header_anterior, offset_header_anterior, header_anterior);

			int pag_post_siguiente = floor((double)header_siguiente->next_alloc / (double)CONFIG.tamanio_pagina);
			int offset_header_post_siguiente = (double)(((header_siguiente->next_alloc) / (double)CONFIG.tamanio_pagina) - pag_post_siguiente) * CONFIG.tamanio_pagina;

			heap_metadata* header_post_siguiente = desserializar_header(pid, pag_post_siguiente, offset_header_post_siguiente);
			header_post_siguiente->prev_alloc = header_a_liberar->prev_alloc;
			guardar_header(pid, pag_post_siguiente, offset_header_post_siguiente, header_post_siguiente);

			liberar_si_hay_paginas_libres(pid, header_a_liberar->prev_alloc + sizeof(heap_metadata), header_siguiente->next_alloc);
			free(header_post_siguiente);
		}

		free(header_anterior);
	}


	free(header_a_liberar);
	free(header_siguiente);

	return 1;
}

int memread(int pid, int dir_logica, void* dest, int size) {

	t_list* paginas_proceso = tabla_por_pid(pid)->paginas;

	if (paginas_proceso == NULL || list_size(paginas_proceso) == 0) {
		return MATE_READ_FAULT;
	}

	int pag_inicial = floor((double)dir_logica / (double)CONFIG.tamanio_pagina);
	int pag_final = floor(((double)dir_logica + size)/ (double)CONFIG.tamanio_pagina);

	if(pag_inicial > list_size(paginas_proceso) || dir_logica + size > list_size(paginas_proceso) * CONFIG.tamanio_pagina){
		return MATE_READ_FAULT;
	}

	int index_pag_inicial;

	for(int i = 0; i < list_size(paginas_proceso); i++){
		t_pagina* pagina =  pagina_por_id(pid, i);
		if(pagina->id == pag_inicial){
			index_pag_inicial = i;
		}
	}

	int offset = dir_logica - CONFIG.tamanio_pagina * pag_inicial;
	t_pagina* pagina =  pagina_por_id(pid, index_pag_inicial);
	lockear(pagina);
	int frame_pag = buscar_pagina(pid, pagina->id);

	//EL memread se hace en una sola pagina
	if(size <= CONFIG.tamanio_pagina - offset){
		memcpy(dest, MEMORIA_PRINCIPAL + frame_pag * CONFIG.tamanio_pagina + offset, size);
		unlockear(pagina);
		return 1;
	} else {
		//El memread se hace en varias paginas

		memcpy(dest, MEMORIA_PRINCIPAL + frame_pag * CONFIG.tamanio_pagina + offset, CONFIG.tamanio_pagina - offset);
		unlockear(pagina);
		size -= CONFIG.tamanio_pagina - offset;

		int i;
		//Entra al for solo si tiene que copiar paginas enteras
		for(i = 1; i <= pag_final - pag_inicial - 1; i++){

			pagina =  pagina_por_id(pid, index_pag_inicial + i);
			lockear(pagina);
			frame_pag = buscar_pagina(pid, pagina->id);

			memcpy(dest + CONFIG.tamanio_pagina - offset + (i -1) * CONFIG.tamanio_pagina, MEMORIA_PRINCIPAL + frame_pag * CONFIG.tamanio_pagina, CONFIG.tamanio_pagina);
			unlockear(pagina);
			size -= CONFIG.tamanio_pagina;
		}

		pagina =  pagina_por_id(pid, index_pag_inicial + i);
		lockear(pagina);
		frame_pag = buscar_pagina(pid, pagina->id);
		memcpy(dest + CONFIG.tamanio_pagina - offset + (i-1) * CONFIG.tamanio_pagina, MEMORIA_PRINCIPAL + frame_pag * CONFIG.tamanio_pagina , size);
		unlockear(pagina);

	}

	return 1;
}

int memwrite(int pid, void* contenido, int dir_logica,  int size) {

	t_list* paginas_proceso = tabla_por_pid(pid)->paginas;

	if (paginas_proceso == NULL || list_size(paginas_proceso) == 0) {
		return MATE_WRITE_FAULT;
	}

	int pag_inicial = floor((double)dir_logica / (double)CONFIG.tamanio_pagina);
	int pag_final = floor((double)(dir_logica + size)/ (double)CONFIG.tamanio_pagina);

	if(pag_inicial > list_size(paginas_proceso) || dir_logica + size > list_size(paginas_proceso) * CONFIG.tamanio_pagina){
		return MATE_WRITE_FAULT;
	}

	int index_pag_inicial;

	for(int i = 0; i < list_size(paginas_proceso); i++){
		t_pagina* pagina =  pagina_por_id(pid, i);
		if(pagina->id == pag_inicial){
			index_pag_inicial = i;
		}
	}

	int offset = dir_logica - CONFIG.tamanio_pagina * pag_inicial;
	t_pagina* pagina =  pagina_por_id(pid, index_pag_inicial);
	lockear(pagina);
	int frame_pag = buscar_pagina(pid, pagina->id);

	//EL memwrite se hace en una sola pagina
	if(size <= CONFIG.tamanio_pagina - offset){
		memcpy(MEMORIA_PRINCIPAL + frame_pag * CONFIG.tamanio_pagina + offset, contenido, size);
		set_modificado(pagina);
		unlockear(pagina);
		return 1;
	} else {
		//El memwrite se hace en varias paginas

		memcpy(MEMORIA_PRINCIPAL + frame_pag * CONFIG.tamanio_pagina + offset, contenido ,  CONFIG.tamanio_pagina - offset);
		set_modificado(pagina);
		unlockear(pagina);
		size -= CONFIG.tamanio_pagina - offset;

		int i;
		//Entra al for solo si tiene que copiar paginas enteras
		for(i = 1; i <= pag_final - pag_inicial - 1; i++){

			pagina =  pagina_por_id(pid, index_pag_inicial + i);
			lockear(pagina);
			frame_pag = buscar_pagina(pid, pagina->id);

			memcpy(MEMORIA_PRINCIPAL + frame_pag * CONFIG.tamanio_pagina, contenido + CONFIG.tamanio_pagina - offset + (i-1) * CONFIG.tamanio_pagina , CONFIG.tamanio_pagina);
			set_modificado(pagina);
			unlockear(pagina);
			size -= CONFIG.tamanio_pagina;
		}

		pagina =  pagina_por_id(pid, index_pag_inicial + i);
		lockear(pagina);
		frame_pag = buscar_pagina(pid, pagina->id);
		memcpy(MEMORIA_PRINCIPAL + frame_pag * CONFIG.tamanio_pagina , contenido + CONFIG.tamanio_pagina - offset + (i-1) * CONFIG.tamanio_pagina, size);
		set_modificado(pagina);
		unlockear(pagina);

	}

	return 1;
}

void suspender_proceso(int pid) {
	t_list* paginas_proceso =  (tabla_por_pid(pid))->paginas;
	//Sirve para dinamico y fijo
	for (int i = 0; i < list_size(paginas_proceso); i++) {
		t_pagina* pagina = list_get(paginas_proceso,i);
		if(pagina->presencia){
			t_frame* frame = list_get(FRAMES_MEMORIA,pagina->frame_ppal);
			frame->ocupado = false;
			frame->pid = -1;

			if(pagina->modificado){
				tirar_a_swap(pagina);
			}
		}
	 }
	//Sirve para asignacion fija cuando cant_pags < cant_marcos_max
	bool frames_del_pid(void * elemento){
		t_frame* frame_aux = (t_frame*) elemento;
		return frame_aux->pid == pid;
	}
	if (string_equals_ignore_case(CONFIG.tipo_asignacion, "FIJA" )){
		t_list* frames_memoria_pid = list_filter(FRAMES_MEMORIA, frames_del_pid);
		for(int i = 0; i < list_size(frames_memoria_pid); i++) {
			t_frame* frame = list_get(frames_memoria_pid, i);
			frame->ocupado = false;
			frame->pid = -1;
		}
	}

}

void dessuspender_proceso(int pid) {

	if (string_equals_ignore_case(CONFIG.tipo_asignacion, "FIJA" )){

		for(int i = 0; i < CONFIG.marcos_max; i++) {
			int frame = solicitar_frame_en_ppal(pid);

			if(frame == -1){
				log_info(LOGGER, "Error: fallo la dessuspension del proceso %i", pid);
				return;
			}
		}
	}
}

void eliminar_proceso(int pid) {
	t_tabla_pagina* tabla_proceso = tabla_por_pid(pid);
	t_list* paginas_proceso = tabla_proceso->paginas;

	if (tabla_proceso == NULL) {
		exit(1);
	}

	for (int i = 0; i < list_size(paginas_proceso); i++) {

		t_pagina* pagina = list_get(paginas_proceso,i);

		if(pagina->presencia) {

			t_frame* frame = list_get(FRAMES_MEMORIA,pagina->frame_ppal);
			frame->ocupado = false;
		}

		free(pagina);
	}

	bool frames_del_pid(void * elemento){
		t_frame* frame_aux = (t_frame*) elemento;
		return frame_aux->pid == pid;
	}

	if (string_equals_ignore_case(CONFIG.tipo_asignacion, "FIJA" )){
		t_list* frames_memoria_pid = list_filter(FRAMES_MEMORIA, frames_del_pid);
		for(int i = 0; i < list_size(frames_memoria_pid); i++) {
			t_frame* frame = list_get(frames_memoria_pid, i);
			frame->ocupado = false;
			frame->pid = -1;
		}
	}

	bool tabla_es_de_pid(void* element){
		t_tabla_pagina* tabla_aux = (t_tabla_pagina*) element;
		return tabla_aux->PID == pid;
	}

	list_remove_and_destroy_by_condition(TABLAS_DE_PAGINAS,tabla_es_de_pid,free);

	eliminar_proceso_swap(pid);

	bool tabla_es_de_pid2(void* element){
		t_pid_cliente* tabla_aux = (t_pid_cliente*) element;
		return tabla_aux->pid == pid;
	}
	list_remove_and_destroy_by_condition(PIDS_CLIENTE,tabla_es_de_pid2,free);

}

//------------------------------------------------- FUNCIONES ALLOCs/HEADERs ---------------------------------------------//

t_alloc_disponible* obtener_alloc_disponible(int pid, int size, uint32_t posicion_heap_actual) {

	t_list* paginas_proceso = tabla_por_pid(pid)->paginas;
	int nro_pagina = 0, offset = 0;

	nro_pagina = floor((double)posicion_heap_actual / (double)CONFIG.tamanio_pagina);
	offset     = posicion_heap_actual - CONFIG.tamanio_pagina * nro_pagina;

	t_alloc_disponible* alloc = malloc(sizeof(t_alloc_disponible));

	heap_metadata* header = desserializar_header(pid, nro_pagina, offset);

	if (header->is_free) {

		//----------------------------------//
		//  [Caso A]: El next alloc es NULL
		//----------------------------------//

		if(header->next_alloc == NULL) {

			int tamanio_total      = list_size(paginas_proceso) * CONFIG.tamanio_pagina;
			int tamanio_disponible = tamanio_total - (int)posicion_heap_actual - sizeof(heap_metadata) * 2;

			if (tamanio_disponible > size) {

				//Creo header nuevo
				heap_metadata* header_nuevo = malloc(sizeof(heap_metadata));
				header_nuevo->is_free 		= true;
				header_nuevo->next_alloc 	= NULL;
				header_nuevo->prev_alloc 	= posicion_heap_actual;

				//Actualizo header viejo
				header->is_free = false;
				header->next_alloc = posicion_heap_actual + sizeof(heap_metadata) + size;

				//Copio 1ero el header viejo actualizado y despues el header nuevo
				guardar_header(pid, nro_pagina, offset, header);
				guardar_header(pid, nro_pagina, offset + size + sizeof(heap_metadata), header_nuevo);

				alloc->direc_logica      = header_nuevo->prev_alloc;
				alloc->flag_ultimo_alloc = 0;

				free(header_nuevo);
				free(header);
				return alloc;
			}

			alloc->direc_logica      = posicion_heap_actual;
			alloc->flag_ultimo_alloc =  1;
			return alloc;

		//-------------------------------------//
		//  [Caso B]: El next alloc NO es NULL
		//-------------------------------------//

		} else {

			// B.A. Entra justo, reutilizo el alloc libre
			if (header->next_alloc - posicion_heap_actual - sizeof(heap_metadata) == size) {
				header->is_free = false;
				guardar_header(pid, nro_pagina, offset, header);

				alloc->direc_logica      = posicion_heap_actual;
				alloc->flag_ultimo_alloc = 0;
				return alloc;


			// B.B. Sobra espacio, hay que meter un header nuevo
			}  else if (header->next_alloc - posicion_heap_actual - sizeof(heap_metadata) * 2 > size) {

				//Pag. en donde vamos a insertar el nuevo header
				int nro_pagina          = floor(((double)posicion_heap_actual + size + sizeof(heap_metadata)) / (double)CONFIG.tamanio_pagina);
				int offset_pagina_nueva = posicion_heap_actual + size + sizeof(heap_metadata) - CONFIG.tamanio_pagina * nro_pagina;

				//Pag. en donde esta el header siguiente al header final (puede ser la misma pag. que la anterior)
				int nro_pagina_final    = floor(((double)header->next_alloc) / (double)CONFIG.tamanio_pagina);
				int offset_pagina_final = header->next_alloc - CONFIG.tamanio_pagina * nro_pagina_final;

				heap_metadata* header_final = desserializar_header(pid, nro_pagina_final, offset_pagina_final);

				//Creo header nuevo y lo guardo
				heap_metadata* header_nuevo = malloc(sizeof(heap_metadata));
				header_nuevo->is_free       = true;
				header_nuevo->prev_alloc    = posicion_heap_actual;
				header_nuevo->next_alloc    = header->next_alloc;
				guardar_header(pid, nro_pagina, offset_pagina_nueva, header_nuevo);

				//Actualizo y guardo los headers que ya estaban.
				header->next_alloc = posicion_heap_actual + sizeof(heap_metadata) + size;
				header_final->prev_alloc = posicion_heap_actual + sizeof(heap_metadata) + size;
				guardar_header(pid, nro_pagina, offset, header);
				guardar_header(pid, nro_pagina_final, offset_pagina_final, header_final);

				alloc->direc_logica      = header_nuevo->prev_alloc;
				alloc->flag_ultimo_alloc = 0;

				free(header_nuevo);
				free(header_final);

				return alloc;
			}

		}
	}

	return obtener_alloc_disponible(pid, size, header->next_alloc);
}

void guardar_header(int pid, int index_pag, int offset, heap_metadata* header){

	t_list* pags_proceso = tabla_por_pid(pid)->paginas;
	t_pagina* pag        = (t_pagina*)list_get(pags_proceso, index_pag);

	int diferencia;
	if (offset > CONFIG.tamanio_pagina) {
		diferencia = offset - CONFIG.tamanio_pagina;
	} else {
		diferencia = CONFIG.tamanio_pagina - offset - sizeof(heap_metadata);
	}

	lockear(pag);

	int frame = buscar_pagina(pid, pag->id);

	//El header entra completo en la pagina
	if (diferencia >= 0) {
		memcpy(MEMORIA_PRINCIPAL + CONFIG.tamanio_pagina * frame + offset, header, sizeof(heap_metadata));

	} else {
		t_pagina* pag_sig       = (t_pagina*)list_get(pags_proceso, index_pag + 1);

		void* header_buffer  = malloc(sizeof(heap_metadata));
		memcpy(header_buffer, header, sizeof(heap_metadata));

		diferencia = abs(diferencia);

		lockear(pag_sig);

		int frame_sig = buscar_pagina(pid, pag_sig->id);

		memcpy(MEMORIA_PRINCIPAL + CONFIG.tamanio_pagina * frame + offset, header_buffer, sizeof(heap_metadata) - diferencia);
		memcpy(MEMORIA_PRINCIPAL + CONFIG.tamanio_pagina * frame_sig , header_buffer + (sizeof(heap_metadata) - diferencia), diferencia);

		set_modificado(pag_sig);
		unlockear(pag_sig);
		free(header_buffer);
	}

	set_modificado(pag);
	unlockear(pag);
}

heap_metadata* desserializar_header(int pid, int index_pag, int offset_header) {

	int frame_pagina;
	int offset = 0;

	heap_metadata* header = malloc(sizeof(heap_metadata));
	void* buffer          = malloc(sizeof(heap_metadata));

	t_list* pags_proceso  = tabla_por_pid(pid)->paginas;
	t_pagina* pag         = list_get(pags_proceso, index_pag);

	frame_pagina = buscar_pagina(pid, pag->id);

	lockear(pag);

	int diferencia = CONFIG.tamanio_pagina - offset_header - sizeof(heap_metadata);
	if (diferencia >= 0) {
		memcpy(buffer, MEMORIA_PRINCIPAL + frame_pagina * CONFIG.tamanio_pagina + offset_header, sizeof(heap_metadata));
	} else {

		t_pagina* pag_sig       = (t_pagina*)list_get(pags_proceso, index_pag + 1);
		int frame_pag_siguiente = buscar_pagina(pid, pag_sig->id);

		lockear(pag_sig);

		diferencia = abs(diferencia);

		memcpy(buffer, MEMORIA_PRINCIPAL + frame_pagina * CONFIG.tamanio_pagina + offset_header, sizeof(heap_metadata) - diferencia);
		memcpy(buffer + (sizeof(heap_metadata) - diferencia), MEMORIA_PRINCIPAL + frame_pag_siguiente * CONFIG.tamanio_pagina, diferencia);

		unlockear(pag_sig);
	}

	memcpy(&(header->prev_alloc), buffer + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(&(header->next_alloc), buffer + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(&(header->is_free),    buffer + offset, sizeof(uint8_t));

	unlockear(pag);

	free(buffer);
	return header;
}

int liberar_si_hay_paginas_libres(int pid, int posicion_inicial, int posicion_final){

	t_list* paginas_proceso = tabla_por_pid(pid)->paginas;

	int nro_pagina_inicial = floor((double)posicion_inicial / (double)CONFIG.tamanio_pagina);
	int nro_pagina_final ;
	bool eliminar_ultima = false;

	if(posicion_final == NULL){
		nro_pagina_final =  list_size(paginas_proceso) - 1;

		if( list_size(paginas_proceso) * CONFIG.tamanio_pagina - posicion_inicial > CONFIG.tamanio_pagina ){
			eliminar_ultima = true;
		}
	}else {
		nro_pagina_final = floor((double)posicion_final   / (double)CONFIG.tamanio_pagina);
	}

	for (int i = 1 ; i < nro_pagina_final - nro_pagina_inicial ; i++) {

		int nro_pagina = nro_pagina_inicial + i;
		t_pagina* pag = list_get(paginas_proceso , nro_pagina );

		int nro_frame  = buscar_pagina(pid, pag->id);

		t_frame* frame = list_get(FRAMES_MEMORIA, nro_frame);

		frame->ocupado = false;

		t_pagina* pagina_eliminada = (t_pagina*)list_remove(paginas_proceso, nro_pagina);
		eliminar_pag_swap(pid, pagina_eliminada->id);
		free(pagina_eliminada);

		actualizar_headers_por_liberar_pagina(pid, nro_pagina);
	}

	if(eliminar_ultima){

		t_pagina* pag = list_get(paginas_proceso , nro_pagina_final);
		int nro_frame  = buscar_pagina(pid, pag->id);

		t_frame* frame = list_get(FRAMES_MEMORIA, nro_frame);
		frame->ocupado = false;

		t_pagina* pagina_eliminada = (t_pagina*)list_remove(paginas_proceso, nro_pagina_final);
		eliminar_pag_swap(pid, pagina_eliminada->id);
		free(pagina_eliminada);
	}

	return 1;
}

void actualizar_headers_por_liberar_pagina(int pid, int nro_pag_liberada){
	int i = 1;
	int nro_pagina, offset;

	heap_metadata* header = desserializar_header(pid, 0, 0);

	//Iterar hasta llegar al header anterior a la pagina liberada
	while (header->next_alloc / CONFIG.tamanio_pagina < nro_pag_liberada) {
		offset = (((double)header->next_alloc / CONFIG.tamanio_pagina) - floor((double)header->next_alloc / (double)CONFIG.tamanio_pagina)) * CONFIG.tamanio_pagina;
		nro_pagina = floor(header->next_alloc / (double)CONFIG.tamanio_pagina);
		header = desserializar_header(pid, nro_pagina, offset);
	}

	header->next_alloc -= CONFIG.tamanio_pagina;
	guardar_header(pid, nro_pagina, offset, header);

	while(header->next_alloc != NULL){

		nro_pagina = floor((double)header->next_alloc / (double)CONFIG.tamanio_pagina);
		offset     = (((double)header->next_alloc / CONFIG.tamanio_pagina) - nro_pagina) * CONFIG.tamanio_pagina;

		header     = desserializar_header(pid, nro_pagina, offset);

		if(i > 1){
			header->prev_alloc -= CONFIG.tamanio_pagina;
		}

		if (header->next_alloc != NULL) {
			header->next_alloc -= CONFIG.tamanio_pagina;
		}

		guardar_header(pid, nro_pagina, offset, header);
		i++;
	}

	free(header);
}

//------------------------------------------------ FUNCIONES SECUNDARIAS -----------------------------------------------//

int buscar_pagina(int pid, int pag) {
	t_list* pags_proceso = tabla_por_pid(pid)->paginas;
	t_pagina* pagina     = pagina_por_id(pid, pag);

	int frame = -1;

	if (pags_proceso == NULL || pagina == NULL) {
		return -1;
	}

	pagina->uso = obtener_tiempo();

	//NO esta en memoria
	if (!pagina->presencia) {

		log_info(LOGGER, "PAGE FAULT! La pag %i del proceso %i no se encuentra en memoria.", pag, pid);
		frame = traer_pagina_a_mp(pagina);
		actualizar_tlb(pid, pag, frame);

	//Esta en memoria
	} else {
		frame = buscar_pag_tlb(pid, pag);

		//TLB miss
		if (frame == -1) {
			frame = pagina->frame_ppal;
			actualizar_tlb(pid, pag,frame);

			usleep(CONFIG.retardo_fallo_tlb * 1000);
		}
		usleep(CONFIG.retardo_acierto_tlb * 1000);
	}

	return frame;
}

int solicitar_frame_en_ppal(int pid){

	//Caso asignacion FIJA

	if (string_equals_ignore_case(CONFIG.tipo_asignacion, "FIJA")) {
		t_list* pags_proceso = tabla_por_pid(pid)->paginas;

		if (list_size(pags_proceso) < CONFIG.marcos_max) {

			pthread_mutex_lock(&mutex_frames);
			t_frame* frame = list_get(frames_libres_del_proceso(pid),0);

			if (frame != NULL) {
				frame->ocupado = true;
				frame->pid     = pid;

				pthread_mutex_unlock(&mutex_frames);
				return frame->id;
			}
		}
		pthread_mutex_unlock(&mutex_frames);
		return ejecutar_algoritmo_reemplazo(pid);
	}

	//Caso asignacion DINAMICA

	pthread_mutex_lock(&mutex_frames);
	t_frame* frame = (t_frame*)list_find(FRAMES_MEMORIA, (void*)esta_libre_frame);

	if (frame != NULL) {
		frame->ocupado = true;
		pthread_mutex_unlock(&mutex_frames);
		return frame->id;
	}
	pthread_mutex_unlock(&mutex_frames);

	return ejecutar_algoritmo_reemplazo(pid);

}


//------------------------------------------------ ALGORITMOS DE REEMPLAZO -----------------------------------------------//

int ejecutar_algoritmo_reemplazo(int pid) {

	int retorno = -1;

	if (string_equals_ignore_case(CONFIG.alg_remp_mmu, "LRU"))
		retorno = reemplazar_con_LRU(pid);

	if(string_equals_ignore_case(CONFIG.alg_remp_mmu, "CLOCK-M"))
		retorno = reemplazar_con_CLOCK_M(pid);

	return retorno;
}

int reemplazar_con_LRU(int pid) {

	t_list* paginas;

	if (string_equals_ignore_case(CONFIG.tipo_asignacion, "FIJA")) {
		t_list* paginas_proceso = tabla_por_pid(pid)->paginas;
		paginas_proceso = list_filter(paginas_proceso, (void*)esta_en_mp);
		paginas = list_filter(paginas_proceso, (void*)no_esta_lockeada);
	}

	if (string_equals_ignore_case(CONFIG.tipo_asignacion, "DINAMICA")) {
		paginas = list_filter(paginas_en_mp(), (void*)no_esta_lockeada);
	}

	int masVieja(t_pagina* unaPag, t_pagina* otraPag) {
		return (otraPag->tiempo_uso > unaPag->tiempo_uso);
	}

	list_sort(paginas, (void*)masVieja);

	//COMO REEMPLAZO SEGUN LRU, ELIJO LA PRIMERA QUE ES LA MAS VIEJA
	t_pagina* pag_reemplazo = list_get(paginas, 0);

	log_info(LOGGER, "[REEMPLAZO LRU] Saco NRO_PAG %i del PID %i en FRAME %i", pag_reemplazo->id, pag_reemplazo->pid, pag_reemplazo->frame_ppal);

	lockear(pag_reemplazo);

	//SI EL BIT DE MODIFICADO ES 1, LA GUARDO EM MV -> PORQUE TIENE CONTENIDO DIFERENTE A LO QUE ESTA EN MV
	if(pag_reemplazo->modificado) {
		tirar_a_swap(pag_reemplazo);
		pag_reemplazo->modificado = false;
	}

	pag_reemplazo->presencia  = false;
	unlockear(pag_reemplazo);

	return pag_reemplazo->frame_ppal;
}

int reemplazar_con_CLOCK_M(int pid) {

	int pag_seleccionada;
	int encontre = 0;
	int busco_modificado = 0;
	int i = POSICION_CLOCK;
	int primera_vuelta = 1;

	t_list* paginas = list_filter(paginas_en_mp(), (void*)no_esta_lockeada);
	t_pagina* victima;

	while(!encontre) {

		t_pagina* pagina = list_get(paginas, i);

		if (i >= list_size(paginas)) {
			i = 0;
		}

		if (i == POSICION_CLOCK && !encontre && !primera_vuelta) {
			busco_modificado = 1;
		}

		primera_vuelta = 0;

		if (!busco_modificado) {

			if (pagina->uso == 0 && pagina->modificado == 0) {

				pag_seleccionada = i;
				POSICION_CLOCK   = i;
				encontre         = 1;

			} else {

				i++;

			}

		} else {

			if(pagina->uso == 0) {

				pag_seleccionada = i;
				POSICION_CLOCK   = i;
				encontre         = 1;

			} else {
				i++;
				pagina->uso = 0;
			}

		}

	}

	victima = list_get(paginas, pag_seleccionada);

	log_info(LOGGER, "[REEMPLAZO CLOCK-M] Saco NRO_PAG %i del PID %i en FRAME %i", victima->id, victima->pid, victima->frame_ppal);

	lockear(victima);

	//SI EL BIT DE MODIFICADO ES 1, LA GUARDO EM MV -> PORQUE TIENE CONTENIDO DIFERENTE A LO QUE ESTA EN MV
	if (victima->modificado) {
		tirar_a_swap(victima);
		victima->modificado = false;
	}

	victima->presencia = 0;
	unlockear(victima);

	return victima->frame_ppal;
}


//--------------------------------------------- FUNCIONES PARA LAS LISTAS ADMIN. -----------------------------------------//

bool hay_frames_libres_mp(int cant_frames_necesarios) {
	t_list* frames_libes_MP = list_filter(FRAMES_MEMORIA, esta_libre_frame);
	return list_size(frames_libes_MP) >= cant_frames_necesarios;
}

t_list* paginas_en_mp(){

	t_list* paginas = list_create();

	//BUSCO TODAS LAS PAGINAS

	void paginas_tabla(t_tabla_pagina* una_tabla) {
		list_add_all(paginas, una_tabla->paginas);
	}

	pthread_mutex_lock(&mutex_tablas_dp);
	list_iterate(TABLAS_DE_PAGINAS, (void*)paginas_tabla);
	pthread_mutex_unlock(&mutex_tablas_dp);

	//FILTRO LAS QUE TENGAN EL BIT DE PRESENCIA EN 1 => ESTAN EN MP
	t_list* paginas_en_mp = list_filter(paginas, (void*)esta_en_mp);

	list_destroy(paginas);
	return paginas_en_mp;
}

t_tabla_pagina* tabla_por_pid(int pid){

	t_tabla_pagina* tabla;

	int _mismo_id(t_tabla_pagina* tabla_aux) {
		return (tabla_aux->PID == pid);
	}

    pthread_mutex_lock(&mutex_tablas_dp);
	tabla = list_find(TABLAS_DE_PAGINAS, (void*)_mismo_id);
	pthread_mutex_unlock(&mutex_tablas_dp);

	return tabla;
}

t_pagina* pagina_por_id(int pid, int id) {

	t_list* paginas = tabla_por_pid(pid)->paginas;
	t_pagina* pagina;

	int _mismo_id(t_pagina* pag_aux) {
		return (pag_aux->id == id);
	}

	pagina = list_find(paginas, (void*)_mismo_id);

	return pagina;
}

int index_de_pag(t_list* paginas, int id_pag) {

	t_pagina* pag_aux;

	for (int i = 0 ; i < list_size(paginas); i++) {

		pag_aux = list_get(paginas, i);

		if (pag_aux->id == id_pag) {
			return i;
		}
	}
	return -1;
}

t_list* frames_libres_del_proceso(int pid){

	int _mismo_pid_y_libre(t_frame* frame_aux) {
		return (frame_aux->pid == pid && !frame_aux->ocupado);
	}

	return list_filter(FRAMES_MEMORIA, (void*)_mismo_pid_y_libre);

}

int pid_por_conexion(int cliente) {

	int _mismo_pid(t_pid_cliente* pid_conexion) {
		return pid_conexion->cliente == cliente;
	}

	t_pid_cliente* pid_cliente = list_find(PIDS_CLIENTE, (void*)_mismo_pid);

	if (pid_cliente == NULL){
		return -1;
	}

	return pid_cliente->pid;
}

//------------------------------------------------ INTERACCIONES CON SWAMP -----------------------------------------------//

int solicitar_marcos_max_swap() {
	t_paquete_swap* paquete = malloc(sizeof(t_paquete_swap));

	paquete->cod_op         = SOLICITAR_MARCOS_MAX;
	paquete->buffer         = malloc(sizeof(t_buffer));
	paquete->buffer->size   = 0;
	paquete->buffer->stream = NULL;

	int bytes;

	void* a_enviar = serializar_paquete_swap(paquete, &bytes);

	pthread_mutex_lock(&mutex_swamp);
	send(CONEXION_SWAP, a_enviar, bytes, 0);
	int retorno = recibir_entero(CONEXION_SWAP);
	pthread_mutex_unlock(&mutex_swamp);

	free(a_enviar);
	eliminar_paquete_swap(paquete);

	if(retorno > 0){
		log_info(LOGGER, "Se inicializo conexion monohilo con swap");
	}

	return retorno;
}

int reservar_espacio_en_swap(int pid, int cant_pags) {
	t_paquete_swap* paquete = malloc(sizeof(t_paquete_swap));
	int offset = 0;

	paquete->cod_op         = RESERVAR_ESPACIO;
	paquete->buffer         = malloc(sizeof(t_buffer));
	paquete->buffer->size   = sizeof(uint32_t) * 2;
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &pid, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + offset, &cant_pags, sizeof(uint32_t));

	int bytes;

	void* a_enviar = serializar_paquete_swap(paquete, &bytes);

	pthread_mutex_lock(&mutex_swamp);
	send(CONEXION_SWAP, a_enviar, bytes, 0);
	int retorno = recibir_entero(CONEXION_SWAP);
	pthread_mutex_unlock(&mutex_swamp);

	free(a_enviar);
	eliminar_paquete_swap(paquete);

	return retorno;
}

int traer_pagina_a_mp(t_pagina* pagina) {

	int pos_frame = -1;
	void* pag_serializada = traer_de_swap(pagina->pid, pagina->id);

	//Busco frame en donde voy a alojar la pagina que me traigo de SWAP: ya sea un frame libre o bien un frame de pag q reempl.

	if (string_equals_ignore_case(CONFIG.tipo_asignacion, "FIJA")) {

		if (list_size(frames_libres_del_proceso(pagina->pid)) > 0) {
			t_frame* frame = list_get(frames_libres_del_proceso(pagina->pid),0);
			pos_frame = frame->id;
		} else {
			pos_frame = ejecutar_algoritmo_reemplazo(pagina->pid);
		}
	}

	if (string_equals_ignore_case(CONFIG.tipo_asignacion, "DINAMICA")) {

		pthread_mutex_lock(&mutex_frames);
		t_frame* frame = (t_frame*)list_find(FRAMES_MEMORIA, (void*)esta_libre_frame);
		if (frame != NULL) {
			frame->ocupado = true;
			pos_frame = frame->id;
		} else {
			pos_frame = ejecutar_algoritmo_reemplazo(pagina->pid);
		}
		pthread_mutex_unlock(&mutex_frames);
	}

	memcpy(MEMORIA_PRINCIPAL + pos_frame * CONFIG.tamanio_pagina, pag_serializada, CONFIG.tamanio_pagina);

	pagina->presencia  = true;
	pagina->modificado = 0;
	pagina->frame_ppal = pos_frame;

	return pos_frame;
}

void* traer_de_swap(uint32_t pid, uint32_t nro_pagina) {
	t_paquete_swap* paquete = malloc(sizeof(t_paquete_swap));

	paquete->cod_op         = TRAER_DE_SWAP;
	paquete->buffer         = malloc(sizeof(t_buffer));
	paquete->buffer->size   = sizeof(uint32_t) * 2;
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &pid, sizeof(uint32_t));
	int offset = sizeof(uint32_t);
	memcpy(paquete->buffer->stream + offset, &nro_pagina, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	int bytes;

	void* a_enviar = serializar_paquete_swap(paquete, &bytes);
	void* buffer_pag = malloc(CONFIG.tamanio_pagina);

	pthread_mutex_lock(&mutex_swamp);
	send(CONEXION_SWAP, a_enviar, bytes, 0);
	recv(CONEXION_SWAP, buffer_pag, CONFIG.tamanio_pagina, MSG_WAITALL);
	pthread_mutex_unlock(&mutex_swamp);

	free(a_enviar);
	eliminar_paquete_swap(paquete);

	return buffer_pag;
}

void tirar_a_swap(t_pagina* pagina) {

	log_info(LOGGER, "[MP->SWAMP] Tirando la pagina modificada %i en swap, del proceso ", pagina->id , pagina->pid);

	void* buffer_pag = malloc(CONFIG.tamanio_pagina);
	memcpy(buffer_pag, MEMORIA_PRINCIPAL + pagina->frame_ppal * CONFIG.tamanio_pagina, CONFIG.tamanio_pagina);
	pthread_mutex_lock(&mutex_swamp);
	enviar_pagina(buffer_pag, CONEXION_SWAP, pagina->pid, pagina-> id);
	pthread_mutex_unlock(&mutex_swamp);
	free(buffer_pag);

}

void enviar_pagina(void* pagina, int socket_cliente, uint32_t pid, uint32_t nro_pagina) {
	t_paquete_swap* paquete = malloc(sizeof(t_paquete_swap));

	paquete->cod_op = TIRAR_A_SWAP;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = CONFIG.tamanio_pagina + sizeof(uint32_t) * 2;
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &pid, sizeof(uint32_t));
	int offset = sizeof(uint32_t);
	memcpy(paquete->buffer->stream + offset, &nro_pagina, sizeof(uint32_t));
	offset 	  += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + offset, pagina, CONFIG.tamanio_pagina);

	int bytes;

	void* a_enviar = serializar_paquete_swap(paquete, &bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete_swap(paquete);
}

void eliminar_pag_swap(int pid , int nro_pagina){

	t_paquete_swap* paquete = malloc(sizeof(t_paquete_swap));
	int offset = 0;

	paquete->cod_op         = LIBERAR_PAGINA;
	paquete->buffer         = malloc(sizeof(t_buffer));
	paquete->buffer->size   = sizeof(uint32_t) * 2;
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &pid, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + offset, &nro_pagina, sizeof(uint32_t));

	int bytes;

	void* a_enviar = serializar_paquete_swap(paquete, &bytes);

	pthread_mutex_lock(&mutex_swamp);
	send(CONEXION_SWAP, a_enviar, bytes, 0);
	pthread_mutex_unlock(&mutex_swamp);

	free(a_enviar);
	eliminar_paquete_swap(paquete);

}

void eliminar_proceso_swap(int pid){

		t_paquete_swap* paquete = malloc(sizeof(t_paquete_swap));

		paquete->cod_op         = KILL_PROCESO;
		paquete->buffer         = malloc(sizeof(t_buffer));
		paquete->buffer->size   = sizeof(uint32_t);
		paquete->buffer->stream = malloc(paquete->buffer->size);

		memcpy(paquete->buffer->stream, &pid, sizeof(uint32_t));

		int bytes;

		void* a_enviar = serializar_paquete_swap(paquete, &bytes);

		pthread_mutex_lock(&mutex_swamp);
		send(CONEXION_SWAP, a_enviar, bytes, 0);
		pthread_mutex_unlock(&mutex_swamp);

		free(a_enviar);
		eliminar_paquete_swap(paquete);

}

//-------------------------------------------- FUNCIONES DE ESTADO - t_frame & t_pagina - ------------------------------//

bool esta_libre_frame(t_frame* frame) {
	return !frame->ocupado;
}

bool esta_libre_y_desasignado(t_frame* frame) {
	return !frame->ocupado && frame->pid == -1;
}

int no_esta_lockeada(t_pagina* pag){
	return !pag->lock;
}

int esta_en_mp(t_pagina* pag){
	return pag->presencia;
}

int esta_en_swap(t_pagina* pag){
	return !pag->presencia;
}

void lockear(t_pagina* pag){
	pag->lock = 1;
}

void unlockear(t_pagina* pag){
	pag->lock = 0;
}

void set_modificado(t_pagina* pag){
	pag->modificado = 1;
}

//--------------------------------------------------- FUNCIONES SIGNAL ----------------------------------------------------//

void signal_metricas(){
	log_info(LOGGER, "[SIGNAL METRICAS]: Recibi la senial de imprimir metricas, imprimiendo\n...");
	generar_metricas_tlb();
	exit(1);
}
void signal_dump(){
	log_info(LOGGER, "[SIGNAL DUMP]: Recibi la senial de generar el dump, generando\n...");
	dumpear_tlb();
	exit(1);
}
void signal_clean_tlb(){
	log_info(LOGGER, "[SIGNAL CLEAN TLB]: Recibi la senial para limpiar TLB, limpiando\n...");
	limpiar_tlb();
	exit(1);
}
