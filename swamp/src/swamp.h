/*
 * swamp.h
 *
 *  Created on: 15 sep. 2021
 *      Author: utnso
 */

#ifndef SWAMP_H_
#define SWAMP_H_

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>
#include<signal.h>
#include<unistd.h>
#include <dirent.h>
#include<commons/collections/list.h>
#include<commons/config.h>
#include<commons/string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>


//ESTRUCTURAS

typedef struct {
    char* ip;
    int puerto;
    int tamanio_swamp;
    int tamanio_pag;
    char** archivos_swamp;
    int marcos_max;
    int retardo_swap;

}t_swamp_config;


//FUNCIONES
t_swamp_config crear_archivo_config_swamp(char* ruta);
int crear_conexion(char *ip, char* puerto);


#endif /* SWAMP_H_ */
