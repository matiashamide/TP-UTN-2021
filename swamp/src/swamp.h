/*
 * swamp.h
 *
 *  Created on: 15 sep. 2021
 *      Author: utnso
 */

#ifndef SWAMP_H_
#define SWAMP_H_

#include <futiles/futiles.h>

//ESTRUCTURAS

typedef struct {
	char*  ip;
	char*  puerto;
	int    tamanio_swamp;
	int    tamanio_pag;
	char** archivos_swamp;
	int    marcos_max;
	int    retardo_swap;
}t_swamp_config;

typedef struct {
	int id;
	int fd;
	int espacio_disponible;
}t_metadata_archivo;

typedef struct {
	int  aid;      // Archivo en el que se encuentra
	int  pid;      // ID del proceso
	int  id_pag;	// Numero de pagina de memoria
	int  offset;   // Byte en donde arranca la pagina
	bool ocupado;
}t_frame;


//VARIABLES GLOBALES

int SERVIDOR_SWAP;
t_swamp_config CONFIG;
t_log* LOGGER;

t_list* FRAMES_SWAP;        // LISTA DE t_frame's
t_list* METADATA_ARCHIVOS;  // LISTA DE t_metadata_archivo's


//FUNCIONES

void init_swamp();
t_swamp_config crear_archivo_config_swamp(char* ruta);

void crear_frames();
void crear_archivos();
int crear_archivo(char* path, int size);

void atender_peticiones(int cliente);

t_metadata_archivo* obtener_archivo_mayor_espacio_libre();
int archivo_proceso_existente(int pid);
t_frame* frame_de_pagina(int pid, int nro_pagina);
t_metadata_archivo* obtener_archivo_con_id(int aid);

void eliminar_proceso_swap(int pid);

int reservar_espacio(int pid , int cant_paginas);
void rta_marcos_max(int socket);
void rta_reservar_espacio(int socket, int rta);

t_list* frames_libres_del_archivo(int aid);
t_list* frames_libres_del_proceso(int aid, int pid);
t_frame* ultima_pagina_proceso(int pid);
t_list* frames_del_proceso(int pid);

#endif /* SWAMP_H_ */
