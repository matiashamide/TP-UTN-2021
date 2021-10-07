/*
 * kernel.h
 *
 *  Created on: 15 sep. 2021
 *      Author: utnso
 */

#ifndef KERNEL_H_
#define KERNEL_H_

#include "planificacion.h"
#include <futiles/futiles.h>

//ESTRUCTURAS

typedef struct {
    char* ip_memoria;
    char* puerto_memoria;
    char* ip_kernel;
    char* puerto_kernel;
    char* alg_plani;
    int estimacion_inicial;
    int alfa;
    char** dispositivos_IO;
    char** duraciones_IO;
    int retardo_cpu;
    int grado_multiprogramacion;
	int grado_multiprocesamiento;
}t_kernel_config;


//VARIABLES GLOBALES
t_kernel_config CONFIG_KERNEL;
t_log* LOGGER;
int SERVIDOR_KERNEL;
int SERVIDOR_MEMORIA;
uint32_t PID_PROX_CARPINCHO;



//MUTEXES
pthread_mutex_t mutex_creacion_PID;


//SEMAFOROS


//FUNCIONES
t_kernel_config crear_archivo_config_kernel(char* ruta);
void init_kernel();
void atender_carpinchos(int cliente);
void coordinador_multihilo();
void pasar_a_new(PCB* pcb_carpincho);
void iniciar_planificador_largo_plazo();

#endif /* KERNEL_H_ */


