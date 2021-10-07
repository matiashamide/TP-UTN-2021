/*
 * planificacion.h
 *
 *  Created on: 2 oct. 2021
 *      Author: utnso
 */

#ifndef PLANIFICACION_H_
#define PLANIFICACION_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>


//ESTRUCTURAS
typedef struct
{
	uint32_t PID;

}PCB;

//VARIABLES GLOBALES
t_list_iterator* iterador_lista_ready;



//SEMAFOROS
sem_t sem_cola_new;
sem_t sem_cola_ready;
sem_t sem_grado_multiprogramacion;

//LISTAS
t_list* LISTA_NEW;
t_list* LISTA_READY;
t_list* LISTA_EXEC;
t_list* LISTA_BLOCKED;
t_list* LISTA_SUSPENDED_BLOCKED;
t_list* LISTA_SUSPENDED_READY;

//MUTEXES
pthread_mutex_t mutex_lista_new;
pthread_mutex_t mutex_lista_ready;
pthread_mutex_t mutex_lista_exec;
pthread_mutex_t mutex_lista_blocked;
pthread_mutex_t mutex_lista_blocked_suspended;
pthread_mutex_t mutex_lista_ready_suspended;

//HILOS
pthread_t planificador_largo_plazo;


void algoritmo_planificador_largo_plazo();

#endif /* PLANIFICACION_H_ */
