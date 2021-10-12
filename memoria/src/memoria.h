/*
 * memoria.h
 *
 *  Created on: 15 sep. 2021
 *      Author: utnso
 */

#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <futiles/futiles.h>
#include "tlb.h"

//ESTRUCTURAS
typedef struct {
    char* ip_memoria;
    char* puerto_memoria;
    int tamanio_memoria;
    int tamanio_pagina;
    char* alg_remp_mmu;
    char* tipo_asignacion;
	int marcos_max;
	int cant_entradas_tlb;
	char* alg_reemplazo_tlb;
	int retardo_acierto_tlb;
	int retardo_fallo_tlb;
}t_memoria_config;

typedef struct
{
    int frame_ppal;
    int frame_virtual;
    int presencia;
    int uso;
    int modificado;
    int lock;
    int tiempo_uso;
    int tamanio_disponible;
    int frag_interna;
}t_pagina;

typedef struct
{
    int PID;
    //int direPcb;
    t_list* paginas;
}t_tabla_pagina;

typedef struct{
	int id;
	bool ocupado;
}t_frame;

typedef struct{
	uint32_t prev_alloc;
	uint32_t next_alloc;
	uint8_t is_free;
}heap_metadata;

//VARIABLES GLOBALES

t_memoria_config CONFIG;
t_log* LOGGER;
t_list* TABLAS_DE_PAGINAS;
t_list* MARCOS_MEMORIA;
int SERVIDOR_MEMORIA;

pthread_mutex_t mutexMemoria;
pthread_mutex_t mutexMarcos;
pthread_mutex_t mutexTablas;

/* Memoria ppal */
void* MEMORIA_PRINCIPAL;


//FUNCIONES

void init_memoria();
t_memoria_config crear_archivo_config_memoria(char* ruta);

void atender_carpinchos(int cliente);
void coordinador_multihilo();

void iniciar_paginacion();

int buscar_pagina_en_memoria(int pid, int pag);
void printearTLB(int entradas);

int memalloc(int size , int pid);
int memfree();
t_list* verificar_solicitar_marcos(int cant_marcos);

#endif /* MEMORIA_H_ */
