
#include "tlb.h"

void init_tlb(int entradas , char* algoritmo){

	TLB = list_create();
	TLB = crear_estructura(entradas);

}

t_list* crear_estructura(int cant_entradas){

	t_list* estructuraTLB = list_create();

	for(int i = 0; i < cant_entradas ; i++){

		entrada_tlb* entrada = malloc(sizeof(entrada_tlb));
		entrada->id = i;
		entrada->pag = -1;
		entrada->marco = -1;
		entrada->pid = -1;

		list_add(estructuraTLB , entrada);
	}

	return estructuraTLB;
}

void reemplazar_entrada(entrada_tlb* entrada_nueva, entrada_tlb* entrada_a_reemplazar){

	list_replace_and_destroy_element(TLB , entrada_a_reemplazar->id , (void*)entrada_nueva , (void*)free );
}

void printear_TLB(int entradas){
	printf("-------------------\n");
	for(int i=0 ; i< entradas ; i++){
		printf("entrada %i :\n " , i);
		printf("  pag   %i     " , ((entrada_tlb*)list_get(TLB , i))->pag  );
		printf("  marco %i     " , ((entrada_tlb*)list_get(TLB , i))->marco  );
		printf("  pid   %i   \n" , ((entrada_tlb*)list_get(TLB , i))->pid  );
	}
}

int buscar_frame(int pid, int pag) {

	int _mismo_pid_y_pag(entrada_tlb* entrada) {
		return (entrada->pid == pid && entrada->pag == pag);
	}

	//pthread_mutex_lock(&mutexListaTablas);
	entrada_tlb* entrada = (entrada_tlb*)list_find(TLB, (void*)_mismo_pid_y_pag);
	//pthread_mutex_unlock(&mutexListaTablas);

	if (entrada != NULL) {
		//TLB HITTEAMOS
		entrada->ultimo_uso = obtener_tiempo();
		return entrada->marco;
	}

	//TLB MISSEAR
	int frame = buscar_pagina_en_memoria(pid, pag);

	if (string_equals_ignore_case(CONFIG.alg_reemplazo_tlb,"LRU")) {
		reemplazar_LRU(pid, pag, frame);
	}
	if (string_equals_ignore_case(CONFIG.alg_reemplazo_tlb, "'FIFO")) {
		reemplazar_FIFO(pid, pag, frame);
	}

	return frame;
}

int obtener_tiempo(){
	pthread_mutex_lock(&mutexTiempo);
	int t = TIEMPO_TLB;
	TIEMPO_TLB++;
	pthread_mutex_unlock(&mutexTiempo);
	return t;
}

void reemplazar_FIFO(int pid, int pag, int frame){

}

void reemplazar_LRU(int pid, int pag, int frame){

}
