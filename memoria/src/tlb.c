#include "tlb.h"

void init_tlb(int cant_entradas, char* algoritmo) {

	crear_entradas_tlb(cant_entradas);
	TIEMPO_TLB = 0;

	TLB_HITS = list_create();
	TLB_MISS = list_create();

	log_info(LOGGER , "TLB inicializada correctamente");
}

void crear_entradas_tlb(int cant_entradas){

	TLB = list_create();

	for(int i = 0; i < cant_entradas ; i++){

		t_entrada_tlb* entrada = malloc(sizeof(t_entrada_tlb));

		entrada->pag        = -1;
		entrada->pid        = -1;
		entrada->frame      = -1;
		entrada->ultimo_uso = -1;

		list_add(TLB, entrada);
	}
}

int buscar_pag_tlb(int pid, int pag) {

	int _mismo_pid_y_pag(t_entrada_tlb* entrada) {
		return (entrada->pid == pid && entrada->pag == pag);
	}

	pthread_mutex_lock(&mutexTLB);
	t_entrada_tlb* entrada = (t_entrada_tlb*)list_find(TLB, (void*)_mismo_pid_y_pag);
	pthread_mutex_unlock(&mutexTLB);

	if (entrada != NULL) {
		//TLB HIT
		registrar_evento(pid, 1);
		log_info(LOGGER , "TLB HIT: " , "pag %i" ,  entrada->pag ,"pid %i" ,  entrada->pid);
		entrada->ultimo_uso = obtener_tiempo();
		return entrada->frame;
	}

	//TLB MISS
	registrar_evento(pid, 0);
	//log_info(LOGGER, "TLB MISS: ", "pag %i" , pag ,"pid %i", pid);
	return -1;
}

int obtener_tiempo(){
	pthread_mutex_lock(&mutexTiempo);
	int t = TIEMPO_TLB;
	TIEMPO_TLB++;
	pthread_mutex_unlock(&mutexTiempo);
	return t;
}

void reemplazar_FIFO(int pid, int pag, int frame){

	t_entrada_tlb* entrada_nueva = malloc(sizeof(t_entrada_tlb));
	entrada_nueva->pag        = pag;
	entrada_nueva->pid        = pid;
	entrada_nueva->frame      = frame;
	entrada_nueva->ultimo_uso = -1;

	log_info(LOGGER , "TLB miss : reemplazamos por la nueva entrada: " , "pag %i" ,  entrada_nueva->pag ,"pid %i \n" ,  entrada_nueva->pid);

	pthread_mutex_lock(&mutexTLB);
	t_entrada_tlb* victima = (t_entrada_tlb*)list_get(TLB , 0);
	list_remove_and_destroy_element(TLB , 0  , free);
	list_add_in_index(TLB , CONFIG.cant_entradas_tlb -1, entrada_nueva);
	pthread_mutex_unlock(&mutexTLB);

	log_info(LOGGER ," entrada victima por LRU : pag %i" , victima->pag ," pid %i \n" , victima->pid);

}

void actualizar_tlb(int pid, int pag, int frame){
	if (string_equals_ignore_case(CONFIG.alg_reemplazo_tlb,"LRU")) {
		reemplazar_LRU(pid, pag, frame);
	}
	if (string_equals_ignore_case(CONFIG.alg_reemplazo_tlb, "FIFO")) {
		reemplazar_FIFO(pid, pag, frame);
	}
}

void reemplazar_LRU(int pid, int pag, int frame){

	t_entrada_tlb* entrada_nueva = malloc(sizeof(t_entrada_tlb));
	entrada_nueva->pid 	      = pid;
	entrada_nueva->pag        = pag;
	entrada_nueva->frame      = frame;
	entrada_nueva->ultimo_uso = obtener_tiempo();

	log_info(LOGGER , "TLB miss : reemplazamos por la nueva entrada: " , "pag %i" ,  entrada_nueva->pag ,"pid %i" ,  entrada_nueva->pid , " ultimo uso %i \n", entrada_nueva ->ultimo_uso);

	int masVieja(t_entrada_tlb* una_entrada, t_entrada_tlb* otra_entrada){
		return (otra_entrada->ultimo_uso > una_entrada->ultimo_uso);
    }

	pthread_mutex_lock(&mutexTLB);

	list_sort(TLB, (void*) masVieja);
	t_entrada_tlb* victima = (t_entrada_tlb*)list_get(TLB , 0);
	list_replace_and_destroy_element(TLB, 0, entrada_nueva, free);

	pthread_mutex_unlock(&mutexTLB);

	log_info(LOGGER ," entrada victima por LRU : pag %i" , victima->pag ," pid %i " , victima->pid , " ultimo uso %i \n", victima ->ultimo_uso);

}


//1 --> HIT
//0 --> MISS
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
		nodo_nuevo->pid       = pid;
		nodo_nuevo->contador  = 1;
		list_add(event_list, nodo_nuevo);
	}
}

void printear_TLB(int entradas){
	printf("-------------------\n");
	for(int i = 0 ; i < entradas ; i++){
		printf("entrada      %i :\n " , i);
		printf("pag          %i     " , ((t_entrada_tlb*)list_get(TLB, i))->pag       );
		printf("marco        %i     " , ((t_entrada_tlb*)list_get(TLB, i))->frame     );
		printf("ultimo uso   %i   \n" , ((t_entrada_tlb*)list_get(TLB, i))->ultimo_uso);
	}
}

void generar_metricas_tlb(){
	printf("-----------------------------------------\n");
	printf("METRICAS TLB:\n");

	printf("[HITS TOTALES]: %i \n", list_size(TLB_HITS));
	printf("[HITS POR PROCESO]");

	for (int i = 0 ; i < list_size(TLB_HITS) ; i++) {
		tlb_event* nodo = (tlb_event*)list_get(TLB_HITS, i);
		printf("Proceso %i ---> %i HITS", nodo->pid, nodo->contador);
	}

	printf("[MISS TOTALES]: %i \n", list_size(TLB_MISS));
	printf("[MISS POR PROCESO]");

	for (int i=0 ; i < list_size(TLB_HITS) ; i++) {
		tlb_event* nodo = (tlb_event*)list_get(TLB_HITS, i);
		printf("Proceso %i ---> %i HITS", nodo->pid, nodo->contador);
	}
	exit(1);
}

void dumpear_tlb(){

	FILE* file;
    char* file_name = nombrar_dump_file();

    log_info(LOGGER,"Creo un archivo de dump con el nombre %s\n", file_name);

    char* time = temporal_get_string_time("%d/%m/%y %H:%M:%S");

    file = fopen(file_name, "w");

    fprintf(file, "\n\n");
    fprintf(file, "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
    fprintf(file, "Dump: %s\n\n",time);

    mostrar_entradas_tlb(file);

    fprintf(file, "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n");

    fclose(file);

    free(file_name);
    free(time);
}

void mostrar_entradas_tlb(FILE* file){

    for(int i = 0; i < list_size(TLB); i++){
        mostrar_entrada(i,file);
    }
}

//Entrada:0	Estado:Ocupado	Carpincho: 1	Pagina: 0	Marco: 2
void mostrar_entrada(int nro_entrada, FILE* file) {

	t_entrada_tlb* entrada = list_get(TLB, nro_entrada);
	t_frame* frame = list_get(FRAMES_MEMORIA, entrada->frame);

	fprintf(file, "Entrada: %d        Estado: %s        Carpincho: %d        Pagina: %d        Marco: %d \n",nro_entrada, frame->ocupado, entrada->pid, entrada->pag, entrada->frame);
}

char* nombrar_dump_file(){
	//nombre: Dump_<Timestamp>.dmp

    char* nombre = "Dump_";
	char* time = temporal_get_string_time("%d-%m-%y_%H-%M-%S");//"MiArchivo_210614235915.txt" # 14/06/2021 a las 23:59:15
	char* extension = ".dmp";

	char nombre_archivo[strlen(time) + strlen(extension) + strlen(nombre) + 1];

	sprintf(nombre_archivo,"%s%s%s",nombre,time,extension);

	char* nombre_final = malloc(sizeof(nombre_archivo));
	strcpy(nombre_final, nombre_archivo);

	free(time);

	return nombre_final;
}

void limpiar_tlb(){

	void limpiar_entrada(void* elemento){
		t_entrada_tlb* entrada = (t_entrada_tlb*)elemento;

		entrada->frame      = -1;
		entrada->pag        = -1;
		entrada->pid        = -1;
		entrada->ultimo_uso = -1;
	}

	list_iterate(TLB , limpiar_entrada);
}
