/*
 * memoria.h
 *
 *  Created on: 15 sep. 2021
 *      Author: utnso
 */

#ifndef MEMORIA_H_
#define MEMORIA_H_

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
    int tamanio;
    char* alg_remp_mmu;
    char* tipo_asignacion;
	int marcos_max;
	int cant_entradas_tlb;
	char* alg_reemplazo_tlb;
	int retardo_acierto_tlb;
	int retardo_fallo_tlb;

}t_memoria_config;


//FUNCIONES
t_memoria_config crear_archivo_config_memoria(char* ruta);
int crear_conexion(char *ip, char* puerto);
int iniciar_servidor(char* IP, char* PUERTO);



#endif /* MEMORIA_H_ */
