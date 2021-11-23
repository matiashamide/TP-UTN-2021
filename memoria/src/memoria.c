#include "memoria.h"

int main(void) {

	init_memoria();
	log_info(LOGGER, "inicializa memoria");

	//------PRUEBAS--------//

	void* buffer = malloc(CONFIG.tamanio_pagina);

	enviar_pagina(TIRAR_A_SWAP, CONFIG.tamanio_pagina, buffer, CONEXION_SWAP, 0, 2);

	//---------------------//

	return EXIT_SUCCESS;
}


//--------------------------------------------------- INICIALIZACION MEMORIA ---------------------------------------------//

void init_memoria() {

	//Inicializamos logger
	LOGGER = log_create("memoria.log", "MEMORIA", 0, LOG_LEVEL_INFO);

	//Levantamos archivo de configuracion
	CONFIG = crear_archivo_config_memoria("/home/utnso/workspace/tp-2021-2c-DesacatadOS/memoria/src/memoria.config");

	//Iniciamos servidor
	SERVIDOR_MEMORIA = iniciar_servidor(CONFIG.ip_memoria, CONFIG.puerto_memoria);

	//Instanciamos memoria principal
	MEMORIA_PRINCIPAL = malloc(CONFIG.tamanio_memoria);

	//Inicializamos tlb
	init_tlb(CONFIG.cant_entradas_tlb , CONFIG.alg_reemplazo_tlb);

	if (MEMORIA_PRINCIPAL == NULL) {
	   	perror("MALLOC FAIL!\n");
	   	log_info(LOGGER,"No se pudo alocar correctamente la memoria principal.");
        return;
	}

	CONEXION_SWAP = crear_conexion(CONFIG.ip_swap, CONFIG.puerto_swap);
	MAX_MARCOS_SWAP = solicitar_marcos_max_swap();

	//Senales
	signal(SIGINT,  &signal_metricas);
	signal(SIGUSR1, &signal_dump);
	signal(SIGUSR2, &signal_clean_tlb);

	//Iniciamos paginacion
	iniciar_paginacion();
}

void iniciar_paginacion() {

	int cant_frames_ppal = CONFIG.tamanio_memoria / CONFIG.tamanio_pagina;

	log_info(LOGGER, "Tengo %d marcos de %d bytes en memoria principal", cant_frames_ppal, CONFIG.tamanio_pagina);

	//Creamos lista de tablas de paginas
	TABLAS_DE_PAGINAS = list_create();

	//Creamos lista de frames de memoria ppal.
	FRAMES_MEMORIA = list_create();

	for (int i = 0 ; i < cant_frames_ppal ; i++) {

		t_frame* frame = malloc(sizeof(t_frame));
		frame->ocupado = false;
		frame->id      = i;

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
    	config.marcos_max      = config_get_int_value   (memoria_config, "MARCOS_MAXIMOS");
    }

    config.cant_entradas_tlb   = config_get_int_value   (memoria_config, "CANTIDAD_ENTRADAS_TLB");
    config.alg_reemplazo_tlb   = config_get_string_value(memoria_config, "ALGORITMO_REEMPLAZO_TLB");
    config.retardo_acierto_tlb = config_get_int_value   (memoria_config, "RETARDO_ACIERTO_TLB");
    config.retardo_fallo_tlb   = config_get_int_value   (memoria_config, "RETARDO_FALLO_TLB");

    return config;
}

//--------------------------------------------------------- COORDINACION DE CARPINCHOS ----------------------------------------------------------//

void coordinador_multihilo(){

	while(1) {

		int socket = esperar_cliente(SERVIDOR_MEMORIA);

		log_info(LOGGER, "Se conecto un cliente %i\n", socket);

		pthread_t* hilo_atender_carpincho = malloc(sizeof(pthread_t));
		pthread_create(hilo_atender_carpincho, NULL, (void*)atender_carpinchos, (void*)socket);
		pthread_detach(*hilo_atender_carpincho);
	}
}

void atender_carpinchos(int cliente) {

	peticion_carpincho operacion = recibir_operacion(cliente);

	int size_paquete = recibir_entero(cliente);
	int retorno, pid, dir_logica;
	switch (operacion) {

	case MEMALLOC:;
		log_info(LOGGER, "el cliente %i solicito alocar memoria" , cliente);
		int size = recibir_entero(cliente);
		int pid = recibir_entero(cliente);;

		retorno = memalloc(size , pid);


	break;

	case MEMREAD:;
		log_info(LOGGER, "el cliente %i solicito leer memoria" , cliente);
		pid = recibir_entero(cliente);
		dir_logica = recibir_entero(cliente);
		void* dest = malloc(size_paquete - 2* sizeof(int));
		recv(cliente , dest , size_paquete - 2* sizeof(int), 0);

		retorno = memread( pid, dir_logica ,  dest , size_paquete - 2* sizeof(uint32_t));

	break;

	case MEMFREE:;
	log_info(LOGGER, "el cliente %i solicito liberar memoria" , cliente);
		pid = recibir_entero(cliente);
		dir_logica = recibir_entero(cliente);

		retorno = memfree(pid , dir_logica);

	break;

	case MEMWRITE:;
	log_info(LOGGER, "el cliente %i solicito escribir memoria" , cliente);
		pid = recibir_entero(cliente);
		dir_logica = recibir_entero(cliente);
		void* contenido = malloc(size_paquete - 2* sizeof(int));
		recv(cliente , contenido , size_paquete - 2* sizeof(int),0);

		retorno = memwrite(pid , dir_logica , contenido , size_paquete - sizeof(int) * 2 );
	break;

	case MENSAJE:;
		char* mensaje = recibir_mensaje(cliente);
		printf("%s",mensaje);
		fflush(stdout);
	break;

	//Faltan algunos cases;
	default:;
	log_info(LOGGER, "el cliente %i solicito algo no reconocido como solicitud" , cliente);
	break;

	}

	send(cliente, retorno, sizeof(uint32_t) , 0);

}

//--------------------------------------------------- FUNCIONES PRINCIPALES ----------------------------------------------//

int memalloc(int pid, int size){
	int dir_logica = -1;

	//[CASO A]: Llega un proceso nuevo
	if (tabla_por_pid(pid) == NULL){

		log_info(LOGGER, "el proceso %i no existe, inicializandolo... " , pid);

		int frames_necesarios = ceil(size / CONFIG.tamanio_pagina);

		if (string_equals_ignore_case(CONFIG.tipo_asignacion, "FIJA")) {

			if (frames_necesarios > MAX_MARCOS_SWAP || !hay_frames_libres_mp(frames_necesarios)) {
				printf("No se puede asginar %i bytes cantidad de memoria al proceso %i (cant maxima) ", size, pid);
				log_info(LOGGER, "No se puede asginar %i bytes cantidad de memoria al proceso %i (cant maxima) ", size, pid);

				return -1;
			}

			frames_necesarios = MAX_MARCOS_SWAP;
		}

		if (reservar_espacio_en_swap(pid, frames_necesarios) == -1) {
			printf("No se puede asginar %i bytes cantidad de memoria al proceso %i (cant maxima) ", size, pid);
			log_info(LOGGER, "No se puede asginar %i bytes cantidad de memoria al proceso %i (cant maxima) ", size, pid);

			return -1;
		}

		//Crea tabla de paginas para el proceso
		t_tabla_pagina* nueva_tabla = malloc(sizeof(t_tabla_pagina));
		nueva_tabla->paginas = list_create();
		nueva_tabla->PID = pid;

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
		header->is_free    		  = true;
		header->next_alloc 		  = NULL;
		header->prev_alloc        = 0;

		//Bloque de paginas en donde se meten los headers
		void* buffer_pags_proceso = malloc(frames_necesarios * CONFIG.tamanio_pagina);

		memcpy(buffer_pags_proceso, header, sizeof(heap_metadata));
		memcpy(buffer_pags_proceso + sizeof(heap_metadata) + size, header_sig, sizeof(heap_metadata));

		//Crea las paginas y las guarda en memoria
		for (int i = 0 ; i < frames_necesarios ; i++) {

			t_pagina* pagina      = malloc(sizeof(t_pagina));
			pagina->pid 		  = pid;
			pagina->id  		  = i;
			pagina->frame_ppal    = solicitar_frame_en_ppal(pid);
			pagina->modificado    = 1;
			pagina->lock          = 1;
			pagina->presencia     = 1;
			pagina->tiempo_uso    = obtener_tiempo();
			pagina->uso           = 0;
			//pagina->espacio_disponible = CONFIG.tamanio_pagina;

			list_add(nueva_tabla->paginas, pagina);

			memcpy(MEMORIA_PRINCIPAL + pagina->frame_ppal* CONFIG.tamanio_pagina, buffer_pags_proceso + i * CONFIG.tamanio_pagina, CONFIG.tamanio_pagina);
			//unlock_pagina();
		}

		dir_logica = header_sig->prev_alloc;
		free(header);
		log_info(LOGGER, "el proceso %i fue inicializado " , pid);

	} else {

	//[CASO B]: El proceso existe en memoria
		log_info(LOGGER, "el proceso %i ya existe en memoria, alocando memoria para el mismo " , pid);
		t_alloc_disponible* alloc = obtener_alloc_disponible(pid, size, 0);

		if (alloc->flag_ultimo_alloc) {
			//Agregamos paginas si es posible
			if ( string_equals_ignore_case(CONFIG.tipo_asignacion, "FIJA")) {
				printf("No se puede asginar %i bytes cantidad de memoria al proceso %i (cant maxima) ", size, pid);
				log_info(LOGGER, "No se puede asginar %i bytes cantidad de memoria al proceso %i (cant maxima) ", size, pid);

				return -1;
			}

			//Proceso existente con asignacion dinamica
			int cantidad_paginas;
			t_list* paginas_proceso = tabla_por_pid(pid)->paginas;
			int tamanio_total = list_size(paginas_proceso) * CONFIG.tamanio_pagina;
			int size_necesario_extra = size - tamanio_total - alloc->direc_logica;
			cantidad_paginas = ceil(size_necesario_extra/CONFIG.tamanio_pagina);


			if (reservar_espacio_en_swap(pid, cantidad_paginas) == -1 ){
				printf("No se puede asginar %i bytes cantidad de memoria al proceso %i (cant maxima) ", size, pid);
				log_info(LOGGER, "No se puede asginar %i bytes cantidad de memoria al proceso %i (cant maxima) ", size, pid);

				return -1;
			}

			int nro_pagina,offset;
			nro_pagina = list_size(paginas_proceso) -1;
			offset = list_size(paginas_proceso) * CONFIG.tamanio_pagina - alloc->direc_logica;

			//Actualizamos el ultimo header
			heap_metadata* ultimo_header = desserializar_header(pid,nro_pagina, offset);
			ultimo_header->is_free = false;
			ultimo_header->next_alloc = alloc->direc_logica + size;


			//Creamos nuevo header
			int nro_pagina_nueva, offset_nuevo;
			nro_pagina_nueva = floor(ultimo_header->next_alloc/CONFIG.tamanio_pagina);
			offset_nuevo = (ultimo_header->next_alloc/CONFIG.tamanio_pagina - floor(ultimo_header->next_alloc/CONFIG.tamanio_pagina)) * CONFIG.tamanio_pagina;

			heap_metadata* nuevo_header = malloc(sizeof(heap_metadata));
			nuevo_header->is_free = true;
			nuevo_header->prev_alloc = alloc->direc_logica;
			nuevo_header->next_alloc = NULL;

			for (int i = 1 ; i <= cantidad_paginas ; i++) {

				t_pagina* pagina      = malloc(sizeof(t_pagina));

				lockear(pagina);
				pagina->pid 		  = pid;
				pagina->id  		  = ((t_pagina*)list_get(tabla_por_pid(pid)->paginas, list_size(tabla_por_pid(pid)->paginas)))->id + i;
				pagina->frame_ppal    = solicitar_frame_en_ppal(pid);
				pagina->modificado    = 1;
				pagina->lock          = 1;
				pagina->presencia     = 1;
				pagina->tiempo_uso    = obtener_tiempo();
				pagina->uso           = 0;

				list_add(paginas_proceso, pagina);
				unlockear(pagina);
			}

			dir_logica = ultimo_header->next_alloc;
			guardar_header(pid, nro_pagina, offset, ultimo_header);
			guardar_header(pid, nro_pagina_nueva , offset_nuevo, nuevo_header);

			free(ultimo_header);
			free(nuevo_header);

		}

	}
	log_info(LOGGER, "Se le asignaron %i bytes al proceso %i, correctamente " , size, pid);
	return dir_logica + sizeof(heap_metadata);
}

int memfree(int pid, int dir_logica){

	t_list* paginas_proceso = tabla_por_pid(pid)->paginas;

	if (paginas_proceso == NULL || list_size(paginas_proceso) == 0) {
		return MATE_READ_FAULT;
	}

	int pag_en_donde_empieza_el_alloc = floor((dir_logica - sizeof(heap_metadata)) / CONFIG.tamanio_pagina);

	t_pagina* pagina = pagina_por_id(pid, pag_en_donde_empieza_el_alloc);

	lockear(pagina);
	// Buscar pagina o paginas en donde esta el alloc
	// si no estan en mp, traerlas
	//actualizar uso de pag



	//fijarse si la direccion del alloc es correcta, sino devolver MATE_FREE_FAULT (-5)

	//liberar alloc
	//moverse 1 antes y uno despues para ver si hay que liberar algo mas
	//(capaz aca nos serviria tener en la t_pagina un cant. espacio libre e ir sumando, si es == tam pagina, liberar entera)

	return 0;
}

int tentativa_de_memfree(int pid , int dir_logica){

	t_list* paginas_proceso = tabla_por_pid(pid)->paginas;

		if (paginas_proceso == NULL || list_size(paginas_proceso) == 0) {
			return MATE_READ_FAULT;
		}

	int pag_en_donde_empieza_el_header = floor((dir_logica - sizeof(heap_metadata)) / CONFIG.tamanio_pagina);
	heap_metadata* header_a_liberar = desserializar_header(pid , pag_en_donde_empieza_el_header , dir_logica );

	if(header_a_liberar->next_alloc == NULL){
			log_info(LOGGER,"no se puede liberar esta posicion, ultimo header del proceso siempre esta libre");
			free(header_a_liberar);
			return -1;
	}

	int pag_en_donde_empieza_el_header_siguiente = floor((header_a_liberar->next_alloc) / CONFIG.tamanio_pagina);
	heap_metadata* header_siguiente = desserializar_header(pid , pag_en_donde_empieza_el_header_siguiente , header_a_liberar->next_alloc);

	header_a_liberar->is_free = true;
	guardar_header(pid, pag_en_donde_empieza_el_header , header_siguiente->prev_alloc ,header_a_liberar);

	//CASO A :: caso en que queremos liberar primer alloc

	if(header_a_liberar->prev_alloc == NULL){


		//CASO A.1 :: caso en el que el siguiente header este ocupado
		if( !header_siguiente->is_free){

			guardar_header(pid, pag_en_donde_empieza_el_header , dir_logica ,header_a_liberar);
			return 1;
		}
		//CASO A.2 :: caso en el que el siguiente header este libre
		if(header_siguiente->is_free){

			header_a_liberar->next_alloc = header_siguiente->next_alloc;
			guardar_header(pid, pag_en_donde_empieza_el_header , header_siguiente->prev_alloc ,header_a_liberar);
			calcular_pagina_libre( pid , sizeof(heap_metadata) , header_a_liberar->next_alloc );
		}

	}
	//CASO B :: caso en que hay header anterior y header siguiente
	else{

		int pag_en_donde_empieza_el_header_anterior = floor((header_a_liberar->prev_alloc) / CONFIG.tamanio_pagina);
		heap_metadata* header_anterior = desserializar_header(pid , pag_en_donde_empieza_el_header_anterior , header_a_liberar->prev_alloc);

		//CASO B.1 :: caso en el que los headers  esten ocupados
		if(!header_anterior->is_free && !header_siguiente->is_free){

			guardar_header(pid, pag_en_donde_empieza_el_header , header_siguiente->prev_alloc ,header_a_liberar);
			calcular_pagina_libre( pid , header_anterior->next_alloc + 9 , header_a_liberar->next_alloc);

		}

		//CASO B.2 :: caso en el que el header anterior este libre
		if(!header_anterior->is_free && header_siguiente->is_free){

			header_anterior->next_alloc = header_a_liberar->next_alloc;
			guardar_header(pid , pag_en_donde_empieza_el_header_anterior , header_a_liberar->prev_alloc , header_anterior);
			calcular_pagina_libre( pid , header_a_liberar->prev_alloc + 9 , header_a_liberar->next_alloc );
		}

		//CASO B.3 :: caso en el que el header siguiente este libre
		if(header_anterior->is_free && !header_siguiente->is_free){

			header_a_liberar->next_alloc = header_siguiente->next_alloc;
			guardar_header(pid, pag_en_donde_empieza_el_header , header_anterior->next_alloc , header_a_liberar);
			calcular_pagina_libre( pid , header_siguiente->prev_alloc + 9 , header_a_liberar->next_alloc );
		}

		//CASE B.4 :: caso en el que ambos header estan libres
		if(header_anterior->is_free && header_siguiente->is_free){

			header_anterior->next_alloc = header_siguiente->next_alloc;
			guardar_header(pid , pag_en_donde_empieza_el_header_anterior , header_a_liberar->prev_alloc , header_anterior);
			calcular_pagina_libre(pid , header_a_liberar->prev_alloc + 9 , header_a_liberar->next_alloc);

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

	int pag_inicial = floor(dir_logica / CONFIG.tamanio_pagina);
	int pag_final = floor((dir_logica + size)/ CONFIG.tamanio_pagina);

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
	int frame_pag = buscar_pagina(pid, pagina);

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
			frame_pag = buscar_pagina(pid, pagina);

			memcpy(dest + CONFIG.tamanio_pagina - offset + (i -1) * CONFIG.tamanio_pagina, MEMORIA_PRINCIPAL + frame_pag * CONFIG.tamanio_pagina, CONFIG.tamanio_pagina);
			unlockear(pagina);
			size -= CONFIG.tamanio_pagina;
		}

		pagina =  pagina_por_id(pid, index_pag_inicial + i);
		lockear(pagina);
		frame_pag = buscar_pagina(pid, pagina);
		memcpy(dest + CONFIG.tamanio_pagina - offset + (i-1) * CONFIG.tamanio_pagina, MEMORIA_PRINCIPAL + frame_pag * CONFIG.tamanio_pagina , size);
		unlockear(pagina);

	}
	//TODO: ver si hay que mandar la dest por socket
	return 1;
}

int memwrite(int pid, int dir_logica, void* contenido, int size) {
	t_list* paginas_proceso = tabla_por_pid(pid)->paginas;

		if (paginas_proceso == NULL || list_size(paginas_proceso) == 0) {
			return MATE_WRITE_FAULT;
		}

		int pag_en_donde_empieza_el_alloc = floor((dir_logica - sizeof(heap_metadata)) / CONFIG.tamanio_pagina);

		if(pag_en_donde_empieza_el_alloc > list_size(paginas_proceso)){
			return MATE_WRITE_FAULT;
		}


		int retorno = escribir_contenido(pid, dir_logica,  contenido, pag_en_donde_empieza_el_alloc, size);

		if(retorno == -1){
			return MATE_WRITE_FAULT;
		}

	return 1;
}


//------------------------------------------------- FUNCIONES ALLOCs/HEADERs ---------------------------------------------//


int escribir_contenido(int pid, int dir_logica, void* contenido, int pag_en_donde_empieza_el_alloc, int size){

	t_list* paginas_proceso = tabla_por_pid(pid)->paginas;
	t_pagina* pagina =  pagina_por_id(pid, pag_en_donde_empieza_el_alloc);

	lockear(pagina);
	set_modificado(pagina);
	int frame_en_donde_esta_el_alloc = buscar_pagina(pid, pag_en_donde_empieza_el_alloc);

	int offset = dir_logica - sizeof(heap_metadata) - CONFIG.tamanio_pagina * pag_en_donde_empieza_el_alloc;
	heap_metadata* header = desserializar_header(pid, pag_en_donde_empieza_el_alloc, offset);

	int tam_alloc = header->next_alloc - 1 - dir_logica;

	if(tam_alloc < size){
		return -1;
		log_info(LOGGER, "se intento escribir algo mas grande que el tamanio del alloc");
	}

	void* alloc_buffer = malloc(tam_alloc);

		//El alloc esta en una sola pagina
	if (CONFIG.tamanio_pagina - offset - tam_alloc >= 0) {
		memcpy( MEMORIA_PRINCIPAL + frame_en_donde_esta_el_alloc * CONFIG.tamanio_pagina + offset + sizeof(heap_metadata), contenido , tam_alloc);
		unlockear(pagina);
	} else {
		tam_alloc -= CONFIG.tamanio_pagina - offset;
		memcpy( MEMORIA_PRINCIPAL + frame_en_donde_esta_el_alloc * CONFIG.tamanio_pagina + offset + sizeof(heap_metadata), contenido, CONFIG.tamanio_pagina - offset);
		unlockear(pagina);

		int offset_copiado = CONFIG.tamanio_pagina - offset;

		int cant_pags = ceil(tam_alloc / CONFIG.tamanio_pagina);
		int frame_actual;

		for (int i = 1 ; i <= cant_pags ; i++) {

			pagina = pagina_por_id(pid,pag_en_donde_empieza_el_alloc + i);

			lockear(pagina);
			set_modificado(pagina);
			frame_actual = buscar_pagina(pid, pag_en_donde_empieza_el_alloc + i);

			if (tam_alloc > CONFIG.tamanio_pagina) {
				memcpy(MEMORIA_PRINCIPAL + frame_actual * CONFIG.tamanio_pagina + offset + sizeof(heap_metadata), contenido + offset_copiado , CONFIG.tamanio_pagina);
				unlockear(pagina);
				offset_copiado += CONFIG.tamanio_pagina;
				tam_alloc -= CONFIG.tamanio_pagina;
			} else {
				memcpy(MEMORIA_PRINCIPAL + frame_actual * CONFIG.tamanio_pagina + offset + sizeof(heap_metadata), contenido + offset_copiado , tam_alloc);
				unlockear(pagina);
			}
		}
	}

	return 1;
}

//TODO: Ver si estamos devolviendo la DL o DF y Poner lock a la pagina para que no me la saque otro proceso mientras la uso

t_alloc_disponible* obtener_alloc_disponible(int pid, int size, uint32_t posicion_heap_actual) {

	t_alloc_disponible* alloc = malloc(sizeof(t_alloc_disponible));

	t_list* paginas_proceso = tabla_por_pid(pid)->paginas;
	int nro_pagina = 0, offset = 0;

	nro_pagina = ceil(posicion_heap_actual / CONFIG.tamanio_pagina);
	offset = posicion_heap_actual - CONFIG.tamanio_pagina * nro_pagina;

	t_pagina* pag = malloc(sizeof(t_pagina));
	//TODO: nro pagina aca esta bien u obtengo por id ??
	pag = list_get(paginas_proceso, nro_pagina);

	heap_metadata* header = desserializar_header(pid, nro_pagina, offset);

	if (header->is_free) {

		//----------------------------------//
		//  [Caso A]: El next alloc es NULL
		//----------------------------------//

		if(header->next_alloc == NULL) {

			int tamanio_total = list_size(paginas_proceso) * CONFIG.tamanio_pagina;

			if (tamanio_total - posicion_heap_actual - sizeof(heap_metadata) * 2 > size) {

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
				guardar_header(pid, nro_pagina, offset + size, header_nuevo);

				free(header_nuevo);

				alloc->direc_logica = header->next_alloc;
				alloc->flag_ultimo_alloc = 0;
				return alloc;
			}

			alloc->direc_logica      = -1;
			alloc->flag_ultimo_alloc =  0;
			return alloc;

		//-------------------------------------//
		//  [Caso B]: El next alloc NO es NULL
		//-------------------------------------//

		} else {

			// B.A. Entra justo, reutilizo el alloc libre
			if (header->next_alloc - posicion_heap_actual - sizeof(heap_metadata) == size) {
				header->is_free = false;
				guardar_header(pid, nro_pagina, offset, header);

				alloc->direc_logica = posicion_heap_actual;
				alloc->flag_ultimo_alloc = 0;
				return alloc;


			// B.B. Sobra espacio, hay que meter un header nuevo
			}  else if (header->next_alloc - posicion_heap_actual - sizeof(heap_metadata)*2 > size) {

				//Pag. en donde vamos a insertar el nuevo header
				int nro_pagina_nueva = 0, offset_pagina_nueva = 0;
				nro_pagina_nueva = ceil((posicion_heap_actual + size + 9) / CONFIG.tamanio_pagina);
				offset_pagina_nueva = posicion_heap_actual + size + 9 - CONFIG.tamanio_pagina * nro_pagina_nueva;

				t_pagina* pag_nueva = malloc(sizeof(t_pagina));
				pag_nueva = pagina_por_id(pid, nro_pagina_nueva);

				if (!pag_nueva->presencia) { //Si esta en principal == 1
					traer_pagina_a_mp(pag_nueva);
				}

				//Pag. en donde esta el header siguiente al header final (puede ser la misma pag. que la anterior)
				int nro_pagina_final = 0, offset_pagina_final = 0;
				nro_pagina_final = ceil((header->next_alloc) / CONFIG.tamanio_pagina);

				offset_pagina_final = header->next_alloc - CONFIG.tamanio_pagina * nro_pagina_nueva;

				t_pagina* pag_final = malloc(sizeof(t_pagina));
				pag_final = pagina_por_id(pid, nro_pagina_final);

				heap_metadata* header_final = desserializar_header(pid, nro_pagina_final, offset_pagina_final);

				//Creo header nuevo y lo guardo
				heap_metadata* header_nuevo = malloc(sizeof(heap_metadata));
				header_nuevo->is_free       = true;
				header_nuevo->prev_alloc    = posicion_heap_actual;
				header_nuevo->next_alloc    = header->next_alloc;
				guardar_header(pid, nro_pagina_nueva, offset_pagina_nueva, header_nuevo);

				//Actualizo y guardo los headers que ya estaban.
				header->next_alloc = posicion_heap_actual + sizeof(heap_metadata) + size;
				header_final->prev_alloc = posicion_heap_actual + sizeof(heap_metadata) + size;
				guardar_header(pid, nro_pagina, offset, header);
				guardar_header(pid, nro_pagina_final, offset_pagina_final, header_final);

				free(pag_nueva);
				free(pag_final);
				free(header_nuevo);
				free(header_final);

				alloc->direc_logica      = header->next_alloc;
				alloc->flag_ultimo_alloc = 0;
				return alloc;
			}
			obtener_alloc_disponible(pid,size,header->next_alloc);
		}
	}

	alloc->direc_logica      = posicion_heap_actual;
	alloc->flag_ultimo_alloc = 1;
	return alloc;
}

int obtener_pos_ultimo_alloc(int pid) {

	int pos_ult_alloc = 0;

	heap_metadata* header = malloc(sizeof(heap_metadata));
	header->next_alloc = 0;

	while(header->next_alloc != NULL) {

		header = desserializar_header(pid , floor(pos_ult_alloc / CONFIG.tamanio_pagina ), header->next_alloc);
		pos_ult_alloc = header->next_alloc;

	}

	free(header);
	return pos_ult_alloc;
}

void guardar_header(int pid, int nro_pagina, int offset, heap_metadata* header){

	t_list* paginas_proceso = tabla_por_pid(pid)->paginas;
	//TODO nro pagina esta bien aca ???
	t_pagina* pagina        = (t_pagina*)list_get(paginas_proceso, nro_pagina);
	t_pagina* pagina_sig    = (t_pagina*)list_get(paginas_proceso, nro_pagina+1);

	int diferencia = CONFIG.tamanio_pagina - offset - sizeof(heap_metadata);

	lockear(pagina);
	int frame = buscar_pagina(pid, nro_pagina);

	//El header entra completo en la pagina
	if (diferencia >= 0) {
		memcpy(MEMORIA_PRINCIPAL + CONFIG.tamanio_pagina * frame + offset, header, sizeof(heap_metadata));

	} else {

		diferencia = abs(diferencia);
		lockear(pagina_sig);
		int frame_sig = buscar_pagina(pid, nro_pagina +1);
		memcpy(MEMORIA_PRINCIPAL + CONFIG.tamanio_pagina * frame + offset, header, sizeof(heap_metadata) - diferencia);
		memcpy(MEMORIA_PRINCIPAL + CONFIG.tamanio_pagina * frame_sig , header + sizeof(heap_metadata) - diferencia, diferencia);
		set_modificado(pagina_sig);
		unlockear(pagina_sig);

	}

	set_modificado(pagina);
	unlockear(pagina);
}

heap_metadata* desserializar_header(int pid, int nro_pag, int offset_header) {
	int offset = 0;
	heap_metadata* header = malloc(sizeof(heap_metadata));
	void* buffer = malloc(sizeof(heap_metadata));

	t_list* pags_proceso = tabla_por_pid(pid)->paginas;
	//TODO: nro_pagina ta bien aca ?
	t_pagina* pag = (t_pagina*)list_get(pags_proceso, nro_pag);

	int frame_pagina = buscar_pagina(pid, nro_pag);

	lockear(pag);

	int diferencia = CONFIG.tamanio_pagina - offset_header - sizeof(heap_metadata);
	if (diferencia >= 0) {
		memcpy(buffer, MEMORIA_PRINCIPAL + frame_pagina * CONFIG.tamanio_pagina + offset_header, sizeof(heap_metadata));
	} else {
		int frame_pag_siguiente = buscar_pagina(pid, nro_pag + 1);
		t_pagina* pag_sig = (t_pagina*)list_get(pags_proceso, nro_pag + 1);
		lockear(pag_sig);
		diferencia = abs(diferencia);
		memcpy(buffer, MEMORIA_PRINCIPAL + frame_pagina * CONFIG.tamanio_pagina + offset_header, sizeof(heap_metadata) - diferencia);
		memcpy(buffer+sizeof(heap_metadata) - diferencia, MEMORIA_PRINCIPAL + frame_pag_siguiente * CONFIG.tamanio_pagina, diferencia);
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

int calcular_pagina_libre( int pid , int posicion_inicial, int posicion_final){

	t_list* paginas_proceso = tabla_por_pid(pid)->paginas;

	int nro_pagina_inicial = floor(posicion_inicial / CONFIG.tamanio_pagina);
	int nro_pagina_final = floor(posicion_final / CONFIG.tamanio_pagina);

	//CASO A :: caso lindo en el que no se liberan paginas
	if(nro_pagina_final - nro_pagina_inicial < 1  ){
		return 1;
	}
	//CASO B :: caso feo en el q hay que liberar
	else if(nro_pagina_final - nro_pagina_inicial == 2){
		// no contemplo el caso en el que haya dos paginas libre dentro de un alloc, seria un caso un poco borde y recursivo (pueden se 2 o mas)el cual no pienso hacer salu2


		//buscar_pagina(pid , nro_pagina_inicial + 1) la puedo liberar sin traerla a memoria

		 list_remove(paginas_proceso , nro_pagina_inicial + 1);

		actualizar_headers_por_liberar_pagina(pid , nro_pagina_inicial + 1);

		return 1;
	}

	return -1;
}

int actualizar_headers_por_liberar_pagina(int pid , int nro_pag_liberada){

	heap_metadata* header= desserializar_header(pid , 0 , 0);
	int offset;

	//iterar hasta llegar al header anterior a la pagina liberada y offset = posicion de ese header
	while(header->next_alloc / CONFIG.tamanio_pagina < nro_pag_liberada){
	offset = header->next_alloc;
	header = desserializar_header(pid , floor(header->next_alloc / CONFIG.tamanio_pagina), header->next_alloc);
	}

	// a este punto tengo el header anterior a la pagina liberada, tengo que actualizar los valores de los proximos


	int i = 1;

	while(header->next_alloc != NULL){

		int pos_actual_del_header = header->next_alloc;

		int nro_pagina = floor(header->next_alloc / CONFIG.tamanio_pagina);
		header = desserializar_header(pid , nro_pagina , pos_actual_del_header ); // header de la siguiente pagina liberada


			if(i>1){ //el header siguiente a la pagina liberada tiene que conservar el prev alloc -> (i > 1 significa todos los otros headers encontrados)
				header->prev_alloc -= CONFIG.tamanio_pagina;
			}

			header->next_alloc -= CONFIG.tamanio_pagina;

		guardar_header(pid , nro_pagina , pos_actual_del_header ,header );

		i += 1;
 	}

	return 1;
}

//------------------------------------------------ FUNCIONES SECUNDARIAS -----------------------------------------------//

int buscar_pagina(int pid, int pag) {
	t_list* pags_proceso = tabla_por_pid(pid)->paginas;
	t_pagina* pagina     = pagina_por_id(pid, pag);
	//todo deberiamos cambiarlo a que encuentre en base a una lambda mismo_id(), porque si no  al liberar agarramos otra pagina

	log_info(LOGGER, "buscando frame de memoria de la pag %i del proceso %i " , pag, pid);
	int frame = -1;

	if (pags_proceso == NULL || pagina == NULL) {
		return -1;
	}

	pagina->uso = obtener_tiempo();

	if (!pagina->presencia) {
		log_info(LOGGER, "la pag %i del proceso %i no se encuentra en memoria, PAGE FAULT " , pag, pid);
		frame = traer_pagina_a_mp(pagina);
		actualizar_tlb(pid, pag, frame);
	} else {
		frame = buscar_pag_tlb(pid, pag);

		if (frame == -1) {
			log_info(LOGGER, "la pag %i del proceso %i se encuentra en memoria, PAGE FAULT " , pag, pid);
			frame = pagina->frame_ppal;
			actualizar_tlb(pid, pag ,frame);
		}else {
			log_info(LOGGER, "la pag %i del proceso %i se encuentra en la tlb " , pag, pid);
		}
	}

	return frame;
}

int solicitar_frame_en_ppal(int pid){

	//Caso asignacion FIJA

	if (string_equals_ignore_case(CONFIG.tipo_asignacion, "FIJA")) {
		t_list* pags_proceso = tabla_por_pid(pid)->paginas;

		if (list_size(pags_proceso) < CONFIG.marcos_max) {

			pthread_mutex_lock(&mutex_frames);
			t_frame* frame = (t_frame*)list_find(FRAMES_MEMORIA, (void*)esta_libre_frame);

			if (frame != NULL) {
				frame->ocupado = true;
			}
			pthread_mutex_unlock(&mutex_frames);
			return frame->id;

		}

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

	if (string_equals_ignore_case(CONFIG.alg_reemplazo_tlb, "LRU" ))
		retorno = reemplazar_con_LRU(pid);

	if(string_equals_ignore_case(CONFIG.alg_reemplazo_tlb, "CLOCK-M"))
		retorno = reemplazar_con_CLOCK(pid);

	log_info(LOGGER, "se ejecuto el algoritmo de reemplazo, para el frame victima %i " , retorno);

	return retorno;
}

int reemplazar_con_LRU(int pid) {

	t_list* paginas;

	if (string_equals_ignore_case(CONFIG.tipo_asignacion, "FIJA")) {
		t_list* paginas_proceso = tabla_por_pid(pid)->paginas;
		paginas = list_filter(paginas_proceso, (void*)esta_en_mp);
	}

	if (string_equals_ignore_case(CONFIG.tipo_asignacion, "DINAMICA")) {
		paginas = list_filter(paginas_en_mp(), (void*)no_lock);
	}

	int masVieja(t_pagina* unaPag, t_pagina* otraPag) {
		return (otraPag->tiempo_uso > unaPag->tiempo_uso);
	}

	list_sort(paginas, (void*)masVieja);

	//COMO REEMPLAZO SEGUN LRU, ELIJO LA PRIMERA QUE ES LA MAS VIEJA
	t_pagina* pag_reemplazo = list_get(paginas, 0);
	log_info(LOGGER, "Voy a reemplazar la pagina %d del proceso %d que estaba en el frame %d", pag_reemplazo->id, pag_reemplazo->pid, pag_reemplazo->frame_ppal);
	lockear(pag_reemplazo);

	//SI EL BIT DE MODIFICADO ES 1, LA GUARDO EM MV -> PORQUE TIENE CONTENIDO DIFERENTE A LO QUE ESTA EN MV
	if(pag_reemplazo->modificado) {
		tirar_a_swap(pag_reemplazo);
	} else {
		pag_reemplazo->presencia = 0;
		unlockear(pag_reemplazo);
	}

	return pag_reemplazo->frame_ppal;
}

int reemplazar_con_CLOCK(int pid) {

	int pag_seleccionada;
	int no_encontre = 1;
	int i = POSICION_CLOCK;

	t_list* paginas_mp = paginas_mp;
	t_pagina* victima;

	while(no_encontre) {

		if (i >= list_size(paginas_mp)) {
			i = 0;
		}

		t_pagina* pagina = list_get(paginas_mp, i);

		if (pagina->uso == 0) {

			pag_seleccionada = i;
			POSICION_CLOCK   = i;
			no_encontre      = 0;

		} else {

			pagina->uso      = 0;
			i++;

		}

	}

	victima = list_get(paginas_mp, pag_seleccionada);

	log_info(LOGGER, "Voy a reemplazar la pagina %d del proceso %d que estaba en el frame %d", victima->id, victima->pid, victima->frame_ppal);
	lockear(victima);

	//SI EL BIT DE MODIFICADO ES 1, LA GUARDO EM MV -> PORQUE TIENE CONTENIDO DIFERENTE A LO QUE ESTA EN MV
	if(victima->modificado) {
		tirar_a_swap(victima);
	} else {
		victima->presencia = 0;
		unlockear(victima);
	}

	return victima->frame_ppal;
}

//--------------------------------------------- FUNCIONES PARA LAS LISTAS ADMIN. -----------------------------------------//

bool hay_frames_libres_mp(int cant_frames_necesarios) {
	t_list* frames_libes_MP = list_filter(FRAMES_MEMORIA, (void*)esta_libre_frame);
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

    pthread_mutex_lock(&mutex_tablas_dp);
	pagina = list_find(paginas, (void*)_mismo_id);
	pthread_mutex_unlock(&mutex_tablas_dp);

	return pagina;
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
		log_info(LOGGER, "se inicializo conexion monohilo con swap");
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

	//TODO: Pensar que pasa cuando la asignacion es fija y tenes pags sin usar?
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
	} else {
		pos_frame = ejecutar_algoritmo_reemplazo(pagina->pid);
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
	void* buffer_pag = malloc;

	pthread_mutex_lock(&mutex_swamp);
	send(CONEXION_SWAP, a_enviar, bytes, 0);
	buffer_pag = recibir_pagina(CONEXION_SWAP, CONFIG.tamanio_pagina);
	pthread_mutex_unlock(&mutex_swamp);

	free(a_enviar);
	eliminar_paquete_swap(paquete);

	return buffer_pag;
}

void tirar_a_swap(t_pagina* pagina) {

	log_info(LOGGER, "tirando la pagina modificada %i en swap, del proceso " , pagina->id , pagina->pid);

	void* buffer_pag = malloc(CONFIG.tamanio_pagina);
	memcpy(buffer_pag, MEMORIA_PRINCIPAL + pagina->frame_ppal * CONFIG.tamanio_pagina, CONFIG.tamanio_pagina);
	enviar_pagina(TIRAR_A_SWAP, CONFIG.tamanio_pagina, buffer_pag, CONEXION_SWAP, pagina->pid , pagina-> id);
	free(buffer_pag);

}

//-------------------------------------------- FUNCIONES DE ESTADO - t_frame & t_pagina - ------------------------------//

bool esta_libre_frame(t_frame* frame) {
	return frame->ocupado;
}

int no_lock(t_pagina* pag){
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
	log_info(LOGGER, "[MEMORIA]: Recibi la senial de imprimir metricas, imprimiendo\n...");
	generar_metricas_tlb();
}
void signal_dump(){
	log_info(LOGGER, "[MEMORIA]: Recibi la senial de generar el dump, generando\n...");
	dumpear_tlb();
}
void signal_clean_tlb(){
	log_info(LOGGER, "[MEMORIA]: Recibi la senial para limpiar TLB, limpiando\n...");
	limpiar_tlb();
}
