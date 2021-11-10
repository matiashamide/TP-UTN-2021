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
	int pid;
	void* addr;
	int espacio_disponible;
}t_metadata_archivo;


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

#endif /* SWAMP_H_ */
