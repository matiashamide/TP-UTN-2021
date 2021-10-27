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

// ESTRUCTURAS

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

typedef struct {
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

typedef struct {
    int PID;
    //int direPcb;
    t_list* paginas;
}t_tabla_pagina;

typedef struct {
	int id;
	bool ocupado;
}t_frame;

typedef struct{
	uint32_t prev_alloc;
	uint32_t next_alloc;
	uint8_t is_free;
}__attribute__((packed))heap_metadata;

// VARIABLES GLOBALES

int SERVIDOR_MEMORIA;
t_memoria_config CONFIG;
t_log* LOGGER;

t_list* TABLAS_DE_PAGINAS;
t_list* MARCOS_MEMORIA;

//// ---- memoria principal
void* MEMORIA_PRINCIPAL;

//// ---- semaforos
pthread_mutex_t mutexMemoria;
pthread_mutex_t mutexMarcos;
pthread_mutex_t mutexTablas;


// FUNCIONES

//// ---- funciones de inicializacion y coordinacion servidor multihilo
void init_memoria();
void iniciar_paginacion();

t_memoria_config crear_archivo_config_memoria(char* ruta);

void atender_carpinchos(int cliente);
void coordinador_multihilo();


//// ---- funciones principales
int memalloc(int size, int pid);
int memalloc2(int pid , int size);
int memfree();
void* memread();
int memwrite();

//// ---- funciones senales
void signal_metricas();
void signal_dump();
void signal_clean_tlb();

int alocar_en_swap(int pid, int size);
int guardar_paginas_en_memoria(int pid, int marcos_necesarios, t_list* paginas, void* contenido);
int obtener_alloc_disponible(int pid, int size);
int guardar_en_swap(int pid, void* contenido);
t_list* obtener_marcos(int cant_marcos);
int buscar_pagina_en_memoria(int pid, int pag);
heap_metadata* desserializar_header(void* buffer);
void serializar_paginas_en_memoria(t_list* paginas, void* contenido);
void* traer_marquinhos_del_proceso(int pid);



void printearTLB(int entradas);

#endif /* MEMORIA_H_ */
