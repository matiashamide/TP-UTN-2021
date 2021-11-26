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

	while(1) {
		atender_peticiones(cliente);
	}
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
	FRAMES_SWAP       = list_create();

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
    config.marcos_max     = config_get_int_value (swamp_config, "MARCOS_POR_CARPINCHO");
    config.retardo_swap   = config_get_int_value(swamp_config, "RETARDO_SWAP");

    return config;
}

void crear_frames() {

	int frames_x_archivo = CONFIG.tamanio_swamp/CONFIG.tamanio_pag;

	for (int i = 0 ; i < size_char_array(CONFIG.archivos_swamp) ; i++) {

		for (int j= 0 ; j < frames_x_archivo ; j++) {
			t_frame* frame = malloc(sizeof(t_frame));

			frame->aid     = i;
			frame->offset  = j * CONFIG.tamanio_pag;
			frame->ocupado = false;
			frame->id_pag  = -1;
			frame->pid     = -1;

			list_add(FRAMES_SWAP, frame);
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
	t_frame* frame = malloc(sizeof(t_frame));
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

		frame = frame_de_pagina(pid, nro_pagina);
		archivo = obtener_archivo_con_id(frame->aid);

		addr = mmap(NULL, CONFIG.tamanio_pag, PROT_WRITE, MAP_SHARED, archivo->fd, 0);

		if (addr == MAP_FAILED) {
			perror("Error mapping \n");
			exit(1);
		}

		memcpy(addr + frame->offset, buffer_pag, CONFIG.tamanio_pag);
		msync(addr, CONFIG.tamanio_pag, 0);

		munmap(addr, CONFIG.tamanio_swamp);
		free(buffer_pag);

		break;

	case TRAER_DE_SWAP:;

		pid        = recibir_entero(cliente);
		nro_pagina = recibir_entero(cliente);

		log_info(LOGGER, "[SWAMP]: Mandando pagina %i del proceso %i a MP", nro_pagina, pid);

		frame = frame_de_pagina(pid, nro_pagina);
		archivo = obtener_archivo_con_id(frame->aid);

		addr = mmap(NULL, CONFIG.tamanio_pag, PROT_WRITE, MAP_SHARED, archivo->fd, 0);

		if (addr == MAP_FAILED) {
			perror("Error mapping \n");
			exit(1);
		}

		memcpy(buffer_pag, addr + frame->offset, CONFIG.tamanio_pag);
		munmap(addr, CONFIG.tamanio_swamp);

		enviar_pagina(TRAER_DE_SWAP, CONFIG.tamanio_pag, buffer_pag, cliente, pid, nro_pagina);

		break;

	case LIBERAR_PAGINA:;

		pid = recibir_entero(cliente);
		nro_pagina = recibir_entero(cliente);
		log_info(LOGGER, "[SWAMP]: Liberando pagina %i del proceso %i", nro_pagina, pid);

		frame   = frame_de_pagina(pid, nro_pagina);
		archivo = obtener_archivo_con_id(frame->aid);

		archivo->espacio_disponible += CONFIG.tamanio_pag;

		//Si la asignacion es fija, no desasignarle el marco al proceso
		if (CONFIG.marcos_max < 0) {
			frame->pid = -1;
		}

		frame->id_pag   = -1;
		frame->ocupado = false;

	break;

	case SOLICITAR_MARCOS_MAX:;

		log_info(LOGGER, "[SWAMP->MP]: Mandando los marcos_max a MP...");
		send(cliente, &CONFIG.marcos_max, sizeof(uint32_t), 0);

	break;

	case KILL_PROCESO:;

		pid = recibir_entero(cliente);
		log_info(LOGGER, "[SWAMP]: No entiendo la peticion, rey.");
		eliminar_proceso_swap(pid);
	break;

	default: log_info(LOGGER, "[SWAMP]: No entiendo la peticion, rey.");
	break;

	}

	free(frame);
	free(archivo);
	free(buffer_pag);
	free(addr);

	return;
}

int reservar_espacio(int pid, int cant_paginas) {

	int nro_archivo = archivo_proceso_existente(pid);
	t_metadata_archivo* archivo;

	//CASO 1: Proceso existente
	if (nro_archivo != -1) {

		archivo = (t_metadata_archivo*)list_get(METADATA_ARCHIVOS, nro_archivo);

		if (archivo->espacio_disponible >= cant_paginas * CONFIG.tamanio_pag) {

			for (int i = 1; i <= cant_paginas; i++) {

				t_frame* frame;

				if (CONFIG.marcos_max > 0) {
					frame = list_get(frames_libres_del_proceso(nro_archivo, pid), i);
				} else {
					frame = list_get(frames_libres_del_archivo(nro_archivo), i);
				}

				frame->ocupado = true;
				frame->pid     = pid;
				frame->id_pag  = ultima_pagina_proceso(pid)->id_pag + i;

			}

			archivo->espacio_disponible -= cant_paginas * CONFIG.tamanio_pag;

			log_info(LOGGER, "Se reservó %i paginas para el proceso %i",cant_paginas,pid);
			return 1;
		}

		log_info(LOGGER, "No hay espacio disponible para el proceso %i",pid);
		return -1;

	} else {

		//CASO 2: Proceso nuevo

		archivo = obtener_archivo_mayor_espacio_libre();
		int cant_frames_requeridos = cant_paginas;

		//Si hay asignacion fija, reservo todos los marcos max para el proceso
		if (CONFIG.marcos_max > 0) {
			cant_frames_requeridos = CONFIG.marcos_max;
		}

		if (archivo->espacio_disponible >= cant_frames_requeridos * CONFIG.tamanio_pag) {

			//Reserva de frames del proceso
			for (int f = 0 ; f < cant_frames_requeridos ; f++) {

				t_frame* frame = list_get(frames_libres_del_archivo(archivo->id), f);
				frame->pid = pid;
			}

			//Ocupar frames con las paginas del proceso

			for (int p = 0 ; p < cant_paginas ; p++) {

				t_frame* frame_a_ocupar = list_get(frames_libres_del_proceso(archivo->id, pid), p);

				frame_a_ocupar->id_pag  = p;
				frame_a_ocupar->ocupado = true;
			}

			archivo->espacio_disponible -= cant_frames_requeridos * CONFIG.tamanio_pag;

			log_info(LOGGER, "Se reservó %i frames y %i paginas para el proceso %i", cant_frames_requeridos, cant_paginas, pid);
			return 1;
		}

		log_info(LOGGER, "No hay espacio disponible para el proceso %i",pid);
		return -1;
	}
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

void eliminar_proceso_swap(int pid) {

	int fd = archivo_proceso_existente(pid);

	if (fd == -1) {
		return;
	}

	t_list* frames_proceso = frames_del_proceso(pid);
	t_frame* frame;

	for (int i = 0 ; i < list_size(frames_proceso) ; i++) {

		frame = list_get(frames_proceso, i);

		frame->pid     = -1;
		frame->id_pag  = -1;
		frame->ocupado = false;
	}
}


//--------------------------------------------------- FUNCIONES UTILES ---------------------------------------------------//

int archivo_proceso_existente(int pid) {

	bool existe_proceso_por_pid(void* elemento) {
		t_frame* frame_aux = (t_frame*) elemento;
		return frame_aux->pid == pid;
	}

	t_frame* frame = (t_frame*)list_find(FRAMES_SWAP, existe_proceso_por_pid);

	if (frame == NULL)
		return -1;

    return frame->aid;
}

t_frame* frame_de_pagina(int pid, int nro_pagina) {

	bool _existe_pag_pid_nro(void * elemento) {
		t_frame* frame = (t_frame*) elemento;
		return frame->pid == pid && frame->id_pag == nro_pagina;
	}

	return (t_frame*)list_find(FRAMES_SWAP, _existe_pag_pid_nro);
}

t_metadata_archivo* obtener_archivo_con_id(int aid) {

	bool existe_archivo_con_id(void* elemento) {
		t_metadata_archivo* archivo_aux = (t_metadata_archivo*) elemento;
		return archivo_aux->id == aid;
	}

	return (t_metadata_archivo*) list_find(METADATA_ARCHIVOS, existe_archivo_con_id);
}

t_metadata_archivo* obtener_archivo_mayor_espacio_libre() {

	bool _de_mayor_espacio(t_metadata_archivo* un_archivo, t_metadata_archivo* otro_archivo) {
		return un_archivo->espacio_disponible < otro_archivo->espacio_disponible;
	}

	list_sort(METADATA_ARCHIVOS, (void*)_de_mayor_espacio);
	return (t_metadata_archivo*)list_get(METADATA_ARCHIVOS, 0);
}

t_list* frames_libres_del_archivo(int aid) {

	bool frame_libre_del_archivo(void * elemento) {
		t_frame* frame_aux = (t_frame*) elemento;
		return !frame_aux->ocupado && frame_aux->aid == aid && frame_aux->pid == -1;
	}

	return list_filter(FRAMES_SWAP, frame_libre_del_archivo);
}

t_list* frames_libres_del_proceso(int aid, int pid) {

	bool frame_libre_del_proceso(void * elemento) {
		t_frame* frame_aux = (t_frame*) elemento;
		return frame_aux->pid == pid && !frame_aux->ocupado && frame_aux->aid == aid;
	}

	return list_filter(FRAMES_SWAP, frame_libre_del_proceso);
}

t_frame* ultima_pagina_proceso(int pid) {

	bool _ultima_pagina_proceso(void* un_frame, void* otro_frame) {
		t_frame* f1 = (t_frame*) un_frame;
		t_frame* f2 = (t_frame*) otro_frame;

		return f1->id_pag > f2->id_pag && f1->pid == pid && f2->pid == pid;
	}

	return list_get_maximum(FRAMES_SWAP, (void*)_ultima_pagina_proceso);
}

t_list* frames_del_proceso(int pid) {

	bool _frame_del_pid(void * elemento) {
		t_frame* frame = (t_frame*) elemento;
		return frame->pid == pid;
	}

	return list_filter(FRAMES_SWAP, _frame_del_pid);
}
