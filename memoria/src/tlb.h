
#ifndef TLB_H_
#define TLB_H_

#include "memoria.h"

//--ESTRUCTURAS

typedef struct {
    int pag;
    int marco;
    int pid;
    int ultimo_uso;
}entrada_tlb;

//--VARIABLES

int TIEMPO_TLB;
t_list* TLB;

int HIT_TOTALES;
int MISS_TOTALES;

//Semaforos
pthread_mutex_t mutexTiempo;
pthread_mutex_t mutexTLB;

//FUNCIONES

void init_tlb(int cant_entradas ,  char* algoritmo_reemplazo);

void reemplazar_FIFO(int pid, int pag, int frame);
void reemplazar_LRU(int pid, int pag, int frame);
t_list* crear_estructura(int entradas);

void printear_TLB(int entradas);
int obtener_tiempo();
int buscar_frame(int pid, int pag);
int buscar_frame(int pid, int pag);

void print_SIGINT();

#endif /* TLB_H_ */
