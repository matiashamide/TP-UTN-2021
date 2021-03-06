/*
 * kernel.h
 *
 *  Created on: 15 sep. 2021
 *      Author: utnso
 */

#ifndef KERNEL_H_
#define KERNEL_H_

#include "planificacion.h"

//ESTRUCTURAS


//VARIABLES GLOBALES
int SERVIDOR_KERNEL;
int SERVIDOR_MEMORIA;
t_config* CONFIG;
uint32_t PID_PROX_CARPINCHO;



//MUTEXES
pthread_mutex_t mutex_creacion_PID;


//SEMAFOROS


//FUNCIONES
t_kernel_config crear_archivo_config_kernel(char* ruta);
void init_kernel();
void atender_carpinchos(int* cliente);
void coordinador_multihilo();
void pasar_a_new(PCB* pcb_carpincho);
void iniciar_planificador_largo_plazo();
void iniciar_planificador_corto_plazo();
void iniciar_algoritmo_deadlock();
void crear_procesadores();
void crear_dispositivos_io();
void cerrar_kernel();

#endif /* KERNEL_H_ */


