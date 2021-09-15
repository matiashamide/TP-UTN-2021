/*
 * kernel.h
 *
 *  Created on: 15 sep. 2021
 *      Author: utnso
 */

#ifndef KERNEL_H_
#define KERNEL_H_

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


//ESTRUCTURAS

typedef struct {
    char* ip;
    int puerto;
    char* alg_plani;
    int estimacion_inicial;
    int alfa;
    t_list* dispositivos_IO;
    t_list* duraciones_IO;
    int retardo_cpu;
    int grado_multiprogramacion;
	int grado_multiprocesamiento;
}t_kernel_config;


//FUNCIONES
t_kernel_config crear_archivo_config_kernel(char* ruta);

#endif /* KERNEL_H_ */


