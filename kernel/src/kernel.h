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
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>
#include<signal.h>
#include<unistd.h>
#include <dirent.h>
#include<commons/collections/list.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>

//ESTRUCTURAS

typedef struct {
    char* ip_memoria;
    char* puerto_memoria;
    char* alg_plani;
    int estimacion_inicial;
    int alfa;
    char** dispositivos_IO;
    char** duraciones_IO;
    int retardo_cpu;
    int grado_multiprogramacion;
	int grado_multiprocesamiento;
}t_kernel_config;

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


//VARIABLES GLOBALES
t_kernel_config CONFIG_KERNEL;
t_log* LOGGER;
int SERVIDOR_KERNEL;
int SERVIDOR_MEMORIA;

//FUNCIONES
t_kernel_config crear_archivo_config_kernel(char* ruta);
int crear_conexion(char *ip, char* puerto);
int iniciar_servidor(char* IP, char* PUERTO);
int iniciar_servidor_2(char* IP, char* PUERTO);
void init_kernel();
int esperar_cliente(int socket_servidor);
void atender_carpinchos(int* cliente);
void coordinador_multihilo();
int recibir_operacion(int socket_cliente);
char* recibir_mensaje(int socket_cliente);
void* recibir_buffer(uint32_t* size, int socket_cliente);
void enviar_mensaje(char* mensaje, int socket);
void* serializar_paquete(t_paquete* paquete, int* bytes);
void eliminar_paquete(t_paquete* paquete);

#endif /* KERNEL_H_ */


