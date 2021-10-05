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
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>
#include<signal.h>
#include<unistd.h>
#include <dirent.h>
#include<commons/collections/list.h>
#include<commons/config.h>
#include<commons/log.h>
#include<commons/string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <futiles/futiles.h>







//ESTRUCTURAS


typedef struct {
    char* ip_memoria;
    char* puerto_memoria;
    int tamanio;
    char* alg_remp_mmu;
    char* tipo_asignacion;
	int marcos_max;
	int cant_entradas_tlb;
	char* alg_reemplazo_tlb;
	int retardo_acierto_tlb;
	int retardo_fallo_tlb;

}t_memoria_config;


typedef enum {
	INICIALIZAR_SEM,
	ESPERAR_SEM,
	POST_SEM,
	DESTROY_SEM,
	CALL_IO,
	MEMALLOC,
	MEMREAD,
	MEMFREE,
	MEMWRITE,
	MENSAJE
}peticion_carpincho;


typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	peticion_carpincho codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef struct{

}t_pagina;



//VARIABLES GLOBALES

t_memoria_config CONFIG;
t_log* LOGGER;
int SERVIDOR_MEMORIA;

//FUNCIONES

t_memoria_config crear_archivo_config_memoria(char* ruta);
int crear_conexion(char *ip, char* puerto);
int iniciar_servidor(char* IP, char* PUERTO);
void init_memoria();
void* serializar_paquete(t_paquete* paquete, int* bytes);
void eliminar_paquete(t_paquete*);
int esperar_cliente(int socket);
int recibir_operacion(int socket);
char* recibir_mensaje(int socket_cliente);
void* recibir_buffer(uint32_t* size, int socket_cliente);
void atender_carpinchos(int cliente);
void enviar_mensaje(char* mensaje, int servidor);
void coordinador_multihilo();
void atender_carpinchos(int cliente);

#endif /* MEMORIA_H_ */
