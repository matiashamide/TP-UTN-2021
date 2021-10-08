/*
 * matelib.h
 *
 *  Created on: 5 oct. 2021
 *      Author: utnso
 */

#ifndef MATELIB_H_
#define MATELIB_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <commons/config.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>

//-------------------Variables Globales----------------------/

 char* CONFIG_PATH = "/home/utnso/workspace/tp-2021-2c-DesacatadOS/matelib/matelib.config";
 //int CONEXION;

//-------------------Type Definitions----------------------/
typedef struct mate_instance
{
    void *group_info;
} mate_instance;

typedef struct mate_inner_structure
{
  void *memory;
  sem_t* sem_instance;
  int* socket;
} mate_inner_structure;

typedef struct {
    char* ip_kernel;
    char* puerto_kernel;
    char* ip_memoria;
    char* puerto_memoria;
    //Ver que otras cosas habria que agregar

}t_lib_config;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef enum
{
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
}op_code;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef char *mate_io_resource;

typedef char *mate_sem_name;

typedef int32_t mate_pointer;

// TODO: Docstrings

//------------------General Functions---------------------/

int mate_init(mate_instance *lib_ref, char *config);

int mate_close(mate_instance *lib_ref);

t_lib_config crear_archivo_config_lib(char* ruta);
int crear_conexion(char *ip, char* puerto);
int crear_conexion_kernel(char *ip, char* puerto);
void enviar_mensaje(char* mensaje, int socket_cliente);
void* serializar_paquete(t_paquete* paquete, int *bytes);
void eliminar_paquete(t_paquete* paquete);
void mate_printf();


//-----------------Semaphore Functions---------------------/

int mate_sem_init(mate_instance *lib_ref, mate_sem_name sem, unsigned int value);

int mate_sem_wait(mate_instance *lib_ref, mate_sem_name sem);

int mate_sem_post(mate_instance *lib_ref, mate_sem_name sem);

int mate_sem_destroy(mate_instance *lib_ref, mate_sem_name sem);


//--------------------IO Functions------------------------/

int mate_call_io(mate_instance *lib_ref, mate_io_resource io, void *msg);


//--------------Memory Module Functions-------------------/

mate_pointer mate_memalloc(mate_instance *lib_ref, int size);

int mate_memfree(mate_instance *lib_ref, mate_pointer addr);

int mate_memread(mate_instance *lib_ref, mate_pointer origin, void *dest, int size);

int mate_memwrite(mate_instance *lib_ref, void *origin, mate_pointer dest, int size);


#endif /* MATELIB_H_ */
