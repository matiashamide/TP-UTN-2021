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
#include <dirent.h>
#include <netdb.h>
#include <math.h>
#include <fcntl.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef enum {
	PERMISO_CONTINUACION,
	INICIALIZAR_SEM,
	ESPERAR_SEM,
	POST_SEM,
	DESTROY_SEM,
	CALL_IO,
	MEMALLOC,
	MEMREAD,
	MEMFREE,
	MEMWRITE,
	MENSAJE,
	CLOSE
} peticion_carpincho;

typedef struct {
	int size;
	void* stream;
} t_buffer;

typedef struct {
	peticion_carpincho codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef enum{
	RESERVAR_ESPACIO,
	TIRAR_A_SWAP, //de swap a memoria
	TRAER_DE_SWAP, //de memoria a swap
	SOLICITAR_PAGINA_SWAP

} t_peticion_swap;

typedef struct {
	t_peticion_swap cod_op;
	t_buffer* buffer;
} t_paquete_swap;


void printeame_un_cuatro();
int crear_conexion(char *ip, char* puerto);
int iniciar_servidor(char* IP, char* PUERTO);
void* serializar_paquete(t_paquete* paquete, int* bytes);
void eliminar_paquete(t_paquete*);
char* recibir_mensaje(int socket_cliente);
void* recibir_buffer(int* size, int socket_cliente);
int esperar_cliente(int socket);
int recibir_operacion(int socket);
void enviar_mensaje(char* mensaje, int servidor);


// Envios MEMORIA - SWAP
void enviar_pagina(t_peticion_swap sentido_swapeo, int tam_pagina, void* pagina, int socket_cliente, uint32_t pid, uint32_t nro_pagina);
void* serializar_paquete_pagina(t_paquete_swap* paquete, int* bytes);
void eliminar_paquete_pagina(t_paquete_swap*);
int recibir_operacion_swap(int socket_cliente);
uint32_t recibir_entero(int cliente);
void* recibir_pagina(int cliente, int tam_pagina);

#endif /* FUTILES_H_ */
