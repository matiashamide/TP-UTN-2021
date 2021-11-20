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
    char* ip_swap;
    char* puerto_swap;
    int   tamanio_memoria;
    int   tamanio_pagina;
    char* alg_remp_mmu;
    char* tipo_asignacion;
	int   marcos_max;
	int   cant_entradas_tlb;
	char* alg_reemplazo_tlb;
	int   retardo_acierto_tlb;
	int   retardo_fallo_tlb;
}t_memoria_config;

typedef struct {
	int pid;
	int id;
    int frame_ppal;
    int presencia;
    int uso;
    int modificado;
    int lock;
    int tiempo_uso;
    //int espacio_disponible;
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

typedef struct{
	int direc_logica;
	bool flag_ultimo_alloc;
}t_alloc_disponible;

// VARIABLES GLOBALES

int SERVIDOR_MEMORIA;
t_memoria_config CONFIG;
t_log* LOGGER;

//// ---- estructuras administrativas
t_list* TABLAS_DE_PAGINAS;  // Lista de t_tabla_pagina's
t_list* FRAMES_MEMORIA;     // Lista de t_frame's

int CONEXION_SWAP;
int MAX_MARCOS_SWAP;
int POSICION_CLOCK;

//// ---- memoria principal
void* MEMORIA_PRINCIPAL;

//// ---- semaforos
pthread_mutex_t mutex_memoria;
pthread_mutex_t mutex_frames;
pthread_mutex_t mutex_tablas_dp;
pthread_mutex_t mutex_swamp;

//// ---- errores MATELIB
int MATE_FREE_FAULT  = -5;
int MATE_READ_FAULT  = -6;
int MATE_WRITE_FAULT = -7;

// FUNCIONES

//// ---- funciones de inicializacion
void init_memoria();
void iniciar_paginacion();
t_memoria_config crear_archivo_config_memoria(char* ruta);

//// ---- funciones de coordinacion de carpinchos
void atender_carpinchos(int cliente);
void coordinador_multihilo();

//// ---- funciones principales
int   memalloc(int pid,int size);
int   memfree(int pid,int pos_alloc);
int memread(int pid,int dir_logica , void* destination);
int   memwrite(int pid,int dir_logica,void* contenido,int size);

//// ---- funciones allocs / headers
void* obtener_contenido_alloc(int pid, int dir_logica , int pag_donde_empieza_el_alloc);
int escribir_contenido(int pid,int dir_logica, void * contenido, int pag_en_donde_empieza_el_alloc, int size);
t_alloc_disponible* obtener_alloc_disponible(int pid, int size, uint32_t posicion_heap_actual);
int obtener_pos_ultimo_alloc(int pid);
void guardar_header(int pid, int nro_pagina, int offset, heap_metadata* header);
heap_metadata* desserializar_header(int pid, int nro_pag, int offset_header);

//// ---- funciones secundarias (buscar / solicitar pagina o frame)
int buscar_pagina(int pid, int pag);
int solicitar_frame_en_ppal(int pid);

//// ---- funciones algoritmos de reemplazo
int ejecutar_algoritmo_reemplazo(int pid);
int reemplazar_con_LRU(int pid);
int reemplazar_con_CLOCK(int pid);

//// ---- funciones para listas administrativas
bool hay_frames_libres_mp(int frames_necesarios);
t_list* paginas_en_mp();

//// ---- funciones de interaccion con SWAMP
int solicitar_marcos_max_swap();
int reservar_espacio_en_swap(int pid, int cant_pags);
int traer_pagina_a_mp(t_pagina* pag);
void tirar_a_swap(t_pagina* pagina);
void* traer_de_swap(uint32_t pid, uint32_t nro_pagina);

//// ---- funciones de estado
bool esta_libre_frame(t_frame* marco);
int esta_en_mp(t_pagina* pag);
int esta_en_swap(t_pagina* pag);
int no_lock(t_pagina* pag);
void lockear(t_pagina* pag);
void unlockear(t_pagina* pag);
void set_modificado(t_pagina* pag);

//// ---- funciones signal
void signal_metricas();
void signal_dump();
void signal_clean_tlb();

#endif /* MEMORIA_H_ */
