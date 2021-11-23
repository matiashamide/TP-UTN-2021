/*
 ============================================================================
 Name        : swamp.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "swamp.h"

int main(void) {

	init_swamp();

	int cliente = esperar_cliente(SERVIDOR_SWAP);
	atender_peticiones(cliente);

	/*
	while(1){
		int cliente = esperar_cliente(SERVIDOR_SWAP);
		//mutex_lock para que no sea multiplexado
		atender_peticiones(cliente);
		//mutex_unlock
		//TODO: vamos a tener que encolar los requests o eso lo hace solo el hilo?
	}
	*/
}

//------------------------------------------------------ INIT SWAMP -----------------------------------------------------//

void init_swamp(){

	//Incializamos logger
	LOGGER = log_create("swamp.log", "SWAMP", 0, LOG_LEVEL_INFO);

	//Inicializamos archivo de configuracion
	CONFIG = crear_archivo_config_swamp("/home/utnso/workspace/tp-2021-2c-DesacatadOS/swamp/src/swamp.config");

	//Iniciamos servidor
	SERVIDOR_SWAP = iniciar_servidor(CONFIG.ip, CONFIG.puerto);

	//Creamos estructuras correspondientes
	METADATA_ARCHIVOS = list_create();
	FRAMES_SWAP = list_create();
	crear_archivos();
	crear_frames();
}

t_swamp_config crear_archivo_config_swamp(char* ruta) {
	t_swamp_config config;
	t_config* swamp_config = config_create(ruta);

    if (swamp_config == NULL) {
        printf("No se pudo leer el archivo de configuracion de Swamp\n");
        exit(-1);
    }

    config.ip             = config_get_string_value(swamp_config, "IP");
    config.puerto         = config_get_string_value(swamp_config, "PUERTO");
    config.tamanio_swamp  = config_get_int_value(swamp_config, "TAMANIO_SWAP");
    config.tamanio_pag    = config_get_int_value(swamp_config, "TAMANIO_PAGINA");
    config.archivos_swamp = config_get_array_value(swamp_config,"ARCHIVOS_SWAP");
    config.marcos_max     = config_get_int_value(swamp_config, "MARCOS_MAXIMOS");
    config.retardo_swap   = config_get_int_value(swamp_config, "RETARDO_SWAP");

    return config;
}

void crear_frames() {

	int frames_x_archivo = CONFIG.tamanio_swamp/CONFIG.tamanio_pag;

	for (int i = 0 ; i < size_char_array(CONFIG.archivos_swamp) ; i++) {

		for (int j= 0 ; j < frames_x_archivo ; j++) {
			t_pagina* pagina = malloc(sizeof(t_pagina));

			pagina->aid     = i;
			pagina->offset  = j * CONFIG.tamanio_pag;
			pagina->ocupado = false;
			pagina->id      = -1;
			pagina->pid     = -1;

			list_add(FRAMES_SWAP, pagina);
		}

	}
}

void crear_archivos() {

	for (int i = 0 ; i < size_char_array(CONFIG.archivos_swamp) ; i++) {

		t_metadata_archivo* md_archivo = malloc(sizeof(t_metadata_archivo));

		md_archivo->id                 = i;
		md_archivo->espacio_disponible = CONFIG.tamanio_swamp;
		md_archivo->fd                 = crear_archivo(CONFIG.archivos_swamp[i], CONFIG.tamanio_swamp);

		list_add(METADATA_ARCHIVOS, md_archivo);
	}

}

int crear_archivo(char* path, int size) {
	int fd;
	void* addr;

	fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0777);
	if (fd < 0) {
	  exit(EXIT_FAILURE);
	}

	ftruncate(fd, size);

	addr = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);

	if (addr == MAP_FAILED) {
		perror("Error mapping \n");
		exit(1);
	}

	memset(addr, '\0', CONFIG.tamanio_swamp);
	munmap(addr, CONFIG.tamanio_swamp);

	return fd;
}

//----------------------------------------------- COORDINADOR Y OPERACIONES PPALES -----------------------------------------//

void atender_peticiones(int cliente){

	t_peticion_swap operacion = recibir_operacion_swap(cliente);

	recibir_entero(cliente); //Size de lo que manda

	uint32_t pid;
	uint32_t nro_pagina;
	t_pagina* pagina = malloc(sizeof(t_pagina));
	t_metadata_archivo* archivo = malloc(sizeof(t_metadata_archivo));
	void* addr;
	void* buffer_pag = malloc(CONFIG.tamanio_pag);

	switch (operacion) {

	case RESERVAR_ESPACIO:;

		pid = recibir_entero(cliente);
		uint32_t cant_paginas = recibir_entero(cliente);

		log_info(LOGGER, "[SWAMP]: Reservando %i paginas para el proceso %i", cant_paginas, pid);

		int rta = reservar_espacio(pid, cant_paginas);
		rta_reservar_espacio(cliente, rta);

		break;

	case TIRAR_A_SWAP:;

		pid         = recibir_entero(cliente);
		nro_pagina  = recibir_entero(cliente);
		buffer_pag  = recibir_pagina(cliente, CONFIG.tamanio_pag);

		log_info(LOGGER, "[SWAMP]: Guardando la pagina %i del proceso %i en SWAMP", nro_pagina, pid);

		pagina = pagina_x_pid_y_nro(pid, nro_pagina);
		archivo = (t_metadata_archivo*)list_find(METADATA_ARCHIVOS, pagina->aid);

		addr = mmap(NULL, CONFIG.tamanio_pag, PROT_WRITE, MAP_SHARED, archivo->fd, 0);

		if (addr == MAP_FAILED) {
			perror("Error mapping \n");
			exit(1);
		}

		memcpy(addr+pagina->offset, buffer_pag, CONFIG.tamanio_pag);
		msync(addr, CONFIG.tamanio_pag, 0);

		munmap(addr, CONFIG.tamanio_swamp);
		free(buffer_pag);

		break;

	case TRAER_DE_SWAP:;

		pid        = recibir_entero(cliente);
		nro_pagina = recibir_entero(cliente);

		log_info(LOGGER, "[SWAMP]: Mandando pagina %i del proceso %i a MP", nro_pagina, pid);

		pagina = pagina_x_pid_y_nro(pid, nro_pagina);
		archivo = (t_metadata_archivo*)list_find(METADATA_ARCHIVOS, pagina->aid);

		addr = mmap(NULL, CONFIG.tamanio_pag, PROT_WRITE, MAP_SHARED, archivo->fd, 0);

		if (addr == MAP_FAILED) {
			perror("Error mapping \n");
			exit(1);
		}

		memcpy(buffer_pag, addr+pagina->offset, CONFIG.tamanio_pag);
		munmap(addr, CONFIG.tamanio_swamp);

		enviar_pagina(TRAER_DE_SWAP, CONFIG.tamanio_pag, buffer_pag, cliente, pid, nro_pagina);

		break;

	case LIBERAR_PAGINA:;

		pid = recibir_entero(cliente);
		nro_pagina = recibir_entero(cliente);
		log_info(LOGGER, "[SWAMP]: Liberando pagina %i del proceso %i", nro_pagina, pid);

		pagina  = pagina_x_pid_y_nro(pid, nro_pagina);
		archivo = (t_metadata_archivo*)list_find(METADATA_ARCHIVOS, pagina->aid);

		archivo->espacio_disponible += CONFIG.tamanio_pag;

		//TODO: si es asignacion fija no desasignarle al proceso la pag

		pagina->id      = -1;
		pagina->pid     = -1;
		pagina->ocupado = false;

	break;

	case SOLICITAR_MARCOS_MAX:;

		log_info(LOGGER, "[SWAMP->MP]: Mandando los marcos_max a MP...");
		rta_marcos_max(cliente);

	break;

	default: log_info(LOGGER, "[SWAMP]: No entiendo la peticion, rey.");
	break;

	}

	free(pagina);
	free(archivo);
	free(buffer_pag);
	free(addr);

	return;
}

int reservar_espacio(int pid, int cant_pag) {

	int nro_archivo = archivo_proceso_existente(pid);
	t_metadata_archivo* archivo;
	//TODO: cheuqear asignacion fija para darle marcos max
	//CASO 1: Proceso existente
	if (nro_archivo != -1) {

		archivo = (t_metadata_archivo*)list_get(METADATA_ARCHIVOS, nro_archivo);

		if (archivo->espacio_disponible >= cant_pag * CONFIG.tamanio_pag) {

			for (int i = 1; i <= cant_pag; i++) {

				bool pagina_libre_del_archivo(void * elemento){
					t_pagina* pag_aux = (t_pagina*) elemento;
					return !pag_aux->ocupado && pag_aux->aid == nro_archivo;
				}

				bool _ultima_pagina_proceso(void* segmento1, void* segmento2) {
					t_pagina* seg1 = (t_pagina*) segmento1;
					t_pagina* seg2 = (t_pagina*) segmento2;

					return seg1->id > seg2->id && seg1->pid == pid && seg2->pid == pid;
				}

				t_pagina* pagina = list_find(FRAMES_SWAP, pagina_libre_del_archivo);

				pagina->ocupado = true;
				pagina->pid     = pid;
				pagina->id      = ((t_pagina*)list_get_maximum(FRAMES_SWAP,(void*) _ultima_pagina_proceso))->id + i;

			}

			archivo->espacio_disponible -= cant_pag * CONFIG.tamanio_pag;

			log_info(LOGGER, "Se reservó %i paginas para el proceso %i",cant_pag,pid);
			return 1;
		}

		log_info(LOGGER, "No hay espacio disponible para el proceso %i",pid);
		return -1;

	} else {

		//CASO 2: Proceso nuevo

		archivo = obtener_archivo_mayor_espacio_libre();

		if (archivo->espacio_disponible >= cant_pag * CONFIG.tamanio_pag) {

			for (int i = 0 ; i < cant_pag ; i++) {

				bool pagina_libre_del_archivo(void * elemento){
					t_pagina* pag_aux = (t_pagina*) elemento;
					return !pag_aux->ocupado && pag_aux->aid == archivo->id;
				}

				t_pagina* pagina = list_find(FRAMES_SWAP, pagina_libre_del_archivo);

				pagina->ocupado = true;
				pagina->pid = pid;
				pagina->id = i;
			}

			archivo->espacio_disponible -= cant_pag * CONFIG.tamanio_pag;

			log_info(LOGGER, "Se reservó %i paginas para el proceso %i",cant_pag,pid);
			return 1;
		}

		log_info(LOGGER, "No hay espacio disponible para el proceso %i",pid);
		return -1;
	}
}

void rta_marcos_max(int socket) {
	t_paquete_swap* paquete = malloc(sizeof(t_paquete_swap));

	paquete->cod_op         = SOLICITAR_MARCOS_MAX;
	paquete->buffer         = malloc(sizeof(t_buffer));
	paquete->buffer->size   = sizeof(uint32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &CONFIG.marcos_max, sizeof(uint32_t));

	int bytes;

	void* a_enviar = serializar_paquete_swap(paquete, &bytes);
	send(socket, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete_swap(paquete);
}

void rta_reservar_espacio(int socket, int rta) {
	t_paquete_swap* paquete = malloc(sizeof(t_paquete_swap));

	paquete->cod_op         = RESERVAR_ESPACIO;
	paquete->buffer         = malloc(sizeof(t_buffer));
	paquete->buffer->size   = sizeof(uint32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream, &rta, sizeof(uint32_t));

	int bytes;

	void* a_enviar = serializar_paquete_swap(paquete, &bytes);
	send(socket, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete_swap(paquete);
}


//--------------------------------------------------- FUNCIONES UTILES ---------------------------------------------------//

int archivo_proceso_existente(int pid) {

	bool existe_proceso_por_pid(void* elemento) {
		t_pagina* pag_aux = (t_pagina*) elemento;
		return pag_aux->pid == pid;
	}

	t_pagina* pag = (t_pagina*)list_find(FRAMES_SWAP, existe_proceso_por_pid);

	if (pag == NULL)
		return -1;

    return pag->aid;
}

t_pagina* pagina_x_pid_y_nro(int pid, int nro_pagina) {

	bool _existe_pag_pid_nro(void * elemento) {
		t_pagina* pagina = (t_pagina*) elemento;
		return pagina->pid == pid && pagina->id == nro_pagina;
	}

	return (t_pagina*)list_find(FRAMES_SWAP, _existe_pag_pid_nro);
}

t_metadata_archivo* obtener_archivo_mayor_espacio_libre() {

	bool _de_mayor_espacio(t_metadata_archivo* un_archivo, t_metadata_archivo* otro_archivo) {
		return un_archivo->espacio_disponible < otro_archivo->espacio_disponible;
	}

	list_sort(METADATA_ARCHIVOS, (void*)_de_mayor_espacio);
	return (t_metadata_archivo*)list_get(METADATA_ARCHIVOS, 0);
}
