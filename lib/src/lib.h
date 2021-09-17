/*
 * lib.h
 *
 *  Created on: 15 sep. 2021
 *      Author: utnso
 */

#ifndef LIB_H_
#define LIB_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include<commons/string.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include<commons/config.h>

//-------------------Type Definitions----------------------/
typedef struct mate_instance
{
    void *group_info;
} mate_instance;

typedef struct mate_inner_structure
{
  void *memory;
  sem_t*sem_instance;
} mate_inner_structure;

typedef struct {
    char* ip_kernel;
    int puerto_kernel;
    char* ip_memoria;
    int puerto_memoria;
    //Ver que otras cosas habria que agregar

}t_lib_config;

typedef char *mate_io_resource;

typedef char *mate_sem_name;

typedef int32_t mate_pointer;

// TODO: Docstrings

//------------------General Functions---------------------/

int mate_init(mate_instance *lib_ref, char *config);

int mate_close(mate_instance *lib_ref);

t_lib_config crear_archivo_config_lib(char* ruta);

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


#endif /* LIB_H_ */
