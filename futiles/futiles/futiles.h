/*
 * futiles.h
 *
 *  Created on: 15 sep. 2021
 *      Author: utnso
 */

#ifndef FUTILES_H_
#define FUTILES_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <math.h>

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
} peticion_carpincho;

typedef struct {
	int size;
	void* stream;
} t_buffer;

typedef struct {
	peticion_carpincho codigo_operacion;
	t_buffer* buffer;
} t_paquete;


void printeame_un_cuatro();
int crear_conexion(char *ip, char* puerto);
int iniciar_servidor(char* IP, char* PUERTO);
void* serializar_paquete(t_paquete* paquete, int* bytes);
void eliminar_paquete(t_paquete*);
char* recibir_mensaje(int socket_cliente);
void* recibir_buffer(uint32_t* size, int socket_cliente);
int esperar_cliente(int socket);
int recibir_operacion(int socket);
void enviar_mensaje(char* mensaje, int servidor);

#endif /* FUTILES_H_ */
