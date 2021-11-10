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
	void* addr;
	int espacio_disponible;
}t_metadata_archivo;

typedef struct {
	int pid; //id del proceso
	int aid; //archivo en el que se encuentra
	t_list* pags; //lista de t_pag_virtual
}t_imagen_proceso;

typedef struct {
	int id;
	int offset;
}t_pag_virtual;

//VARIABLES GLOBALES

int SERVIDOR_SWAP;
t_swamp_config CONFIG;
t_log* LOGGER;

t_list* METADATA_ARCHIVOS;


//FUNCIONES

t_swamp_config crear_archivo_config_swamp(char* ruta);
int crear_conexion(char *ip, char* puerto);
void init_swamp();
void atender_peticiones(int cliente);

void* crear_archivo(char* path, int size);
void crear_archivos();

#endif /* SWAMP_H_ */
