
#include "tlb.h"

void init_tlb(int entradas , char* algoritmo){

	TLB = list_create();
	TLB = crear_estructura(entradas);
	TIEMPO_TLB = 0;
}

t_list* crear_estructura(int cant_entradas){

	t_list* estructuraTLB = list_create();

	for(int i = 0; i < cant_entradas ; i++){

		entrada_tlb* entrada = malloc(sizeof(entrada_tlb));
		entrada->pag = -1;
		entrada->marco = -1;
		entrada->pid = -1;
		entrada->ultimo_uso = -1;

		list_add(estructuraTLB , entrada);
	}

	return estructuraTLB;
}



int buscar_frame(int pid, int pag) {

	int _mismo_pid_y_pag(entrada_tlb* entrada) {
		return (entrada->pid == pid && entrada->pag == pag);
	}

	pthread_mutex_lock(&mutexTLB);
	entrada_tlb* entrada = (entrada_tlb*)list_find(TLB, (void*)_mismo_pid_y_pag);
	pthread_mutex_unlock(&mutexTLB);

	if (entrada != NULL) {
		log_info(LOGGER , "TLB hit : " , "pag %i" ,  entrada->pag ,"pid %i" ,  entrada->pid);
		entrada->ultimo_uso = obtener_tiempo();
		return entrada->marco;
	}

	int frame = buscar_pagina_en_memoria(pid, pag);

	if (string_equals_ignore_case(CONFIG.alg_reemplazo_tlb,"LRU")) {
		reemplazar_LRU(pid, pag, frame);
	}
	if (string_equals_ignore_case(CONFIG.alg_reemplazo_tlb, "FIFO")) {
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

	entrada_tlb* entrada_nueva = malloc(sizeof(entrada_tlb));
		entrada_nueva->pag = pag;
		entrada_nueva->pid = pid;
		entrada_nueva->marco = frame;
		entrada_nueva->ultimo_uso = -1;

	log_info(LOGGER , "TLB miss : reemplazamos por la nueva entrada: " , "pag %i" ,  entrada_nueva->pag ,"pid %i \n" ,  entrada_nueva->pid);

	pthread_mutex_lock(&mutexTLB);
	entrada_tlb* victima = (entrada_tlb*)list_get(TLB , 0);
	list_remove_and_destroy_element(TLB , 0  , free);
	list_add_in_index(TLB , CONFIG.cant_entradas_tlb -1, entrada_nueva);
	pthread_mutex_unlock(&mutexTLB);

	log_info(LOGGER ," entrada victima por LRU : pag %i" , victima->pag ," pid %i \n" , victima->pid);

}

void reemplazar_LRU(int pid, int pag, int frame){

	entrada_tlb* entrada_nueva = malloc(sizeof(entrada_tlb));
		entrada_nueva->pid 	 = pid;
		entrada_nueva->pag   = pag;
		entrada_nueva->marco = frame;
		entrada_nueva->ultimo_uso = obtener_tiempo();

	log_info(LOGGER , "TLB miss : reemplazamos por la nueva entrada: " , "pag %i" ,  entrada_nueva->pag ,"pid %i" ,  entrada_nueva->pid , " ultimo uso %i \n", entrada_nueva ->ultimo_uso);

	int masVieja(entrada_tlb* una_entrada, entrada_tlb* otra_entrada){
	      return (otra_entrada->ultimo_uso > una_entrada->ultimo_uso);
      }

	pthread_mutex_lock(&mutexTLB);
	list_sort(TLB, (void*) masVieja);
	entrada_tlb* victima = (entrada_tlb*)list_get(TLB , 0);
	list_replace_and_destroy_element(TLB , 0 , entrada_nueva , free);
	pthread_mutex_unlock(&mutexTLB);

	log_info(LOGGER ," entrada victima por LRU : pag %i" , victima->pag ," pid %i " , victima->pid , " ultimo uso %i \n", victima ->ultimo_uso);

}


void printear_TLB(int entradas){
	printf("-------------------\n");
	for(int i=0 ; i< entradas ; i++){
		printf("entrada %i :\n " , i);
		printf("  pag   %i     " , ((entrada_tlb*)list_get(TLB , i))->pag  );
		printf("  marco %i     " , ((entrada_tlb*)list_get(TLB , i))->marco  );
		printf("  ultimo uso   %i   \n" , ((entrada_tlb*)list_get(TLB , i))->ultimo_uso);
	}
}

















