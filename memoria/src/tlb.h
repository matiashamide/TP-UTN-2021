
#ifndef TLB_H_
#define TLB_H_

#include "memoria.h"

//ESTRUCTURAS

typedef struct {
	int id;
    int pag;
    int marco;
    int pid;
}entrada_tlb;

t_list* TLB;

//FUNCIONES

void init_tlb(int cant_entradas ,  char* algoritmo_reemplazo);
//bool esta_en_tlb(t_pagina* una_pagina , int pid);
//void reemplazar_FIFO(t_pagina* nueva_pagina);
//void reemplazar_LRU(t_pagina* pagina_nueva);
t_list* crear_estructura(int entradas);

void printearTLB(int entradas);

#endif /* TLB_H_ */
