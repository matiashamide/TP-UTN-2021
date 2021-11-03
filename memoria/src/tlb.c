
#include "tlb.h"

void init_tlb(int entradas , char* algoritmo){

	TLB = list_create();
	TLB = crear_estructura(entradas);
	TIEMPO_TLB = 0;

	TLB_HITS = list_create();
	TLB_MISS = list_create();
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



int buscar_pag_tlb(int pid, int pag) {

	int _mismo_pid_y_pag(entrada_tlb* entrada) {
		return (entrada->pid == pid && entrada->pag == pag);
	}

	pthread_mutex_lock(&mutexTLB);
	entrada_tlb* entrada = (entrada_tlb*)list_find(TLB, (void*)_mismo_pid_y_pag);
	pthread_mutex_unlock(&mutexTLB);

	if (entrada != NULL) {
		registrar_evento(pid, 0);
		log_info(LOGGER , "TLB HIT: " , "pag %i" ,  entrada->pag ,"pid %i" ,  entrada->pid);
		entrada->ultimo_uso = obtener_tiempo();
		return entrada->marco;
	}

	registrar_evento(pid, 1);

	int frame = buscar_pagina(pid, pag);


	if(frame < 0){

	if (string_equals_ignore_case(CONFIG.alg_reemplazo_tlb,"LRU")) {
		reemplazar_LRU(pid, pag, frame);
	}
	if (string_equals_ignore_case(CONFIG.alg_reemplazo_tlb, "FIFO")) {
		reemplazar_FIFO(pid, pag, frame);
	}
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


//0 --> HIT
//1 --> MISS
void registrar_evento(int pid, int event){

	int _proceso_con_id(tlb_event* evento) {
		return (evento->pid == pid);
	}

	t_list* event_list = event ? TLB_HITS : TLB_MISS;

	tlb_event* nodo_proceso = (tlb_event*)list_find(event_list, (void*)_proceso_con_id);

	if (nodo_proceso != NULL) {
		nodo_proceso->contador++;
	} else {
		tlb_event* nodo_nuevo = malloc(sizeof(tlb_event));
		nodo_nuevo->pid = pid;
		nodo_nuevo->contador = 1;
		list_add(event_list, nodo_nuevo);
	}
}

void printear_TLB(int entradas){
	printf("-------------------\n");
	for(int i=0 ; i < entradas ; i++){
		printf("entrada      %i :\n " , i);
		printf("pag          %i     " , ((entrada_tlb*)list_get(TLB , i))->pag  );
		printf("marco        %i     " , ((entrada_tlb*)list_get(TLB , i))->marco  );
		printf("ultimo uso   %i   \n" , ((entrada_tlb*)list_get(TLB , i))->ultimo_uso);
	}
}

void generar_metricas_tlb(){
	printf("-----------------------------------------\n");
	printf("METRICAS TLB:\n");

	printf("[HITS TOTALES]: %i \n", list_size(TLB_HITS));
	printf("[HITS POR PROCESO]");

	for (int i=0 ; i < list_size(TLB_HITS) ; i++) {
		tlb_event* nodo = (tlb_event*)list_get(TLB_HITS, i);
		printf("Proceso %i ---> %i HITS", nodo->pid, nodo->contador);
	}

	printf("[MISS TOTALES]: %i \n", list_size(TLB_MISS));
	printf("[MISS POR PROCESO]");

	for (int i=0 ; i < list_size(TLB_HITS) ; i++) {
		tlb_event* nodo = (tlb_event*)list_get(TLB_HITS, i);
		printf("Proceso %i ---> %i HITS", nodo->pid, nodo->contador);
	}
}

void dumpear_tlb(){

}

void limpiar_tlb(){
	list_clean_and_destroy_elements(TLB, free);
	list_clean_and_destroy_elements(TLB_HITS, free);
	list_clean_and_destroy_elements(TLB_MISS, free);
}
















