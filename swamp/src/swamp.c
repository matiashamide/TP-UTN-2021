/*
 ============================================================================
 Name        : swamp.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "swamp.h"

int main(void) {

	init_swamp();

	int cliente = esperar_cliente(SERVIDOR_SWAP);
	atender_peticiones(cliente);
	/*
	while(1){
		int cliente = esperar_cliente(SERVIDOR_SWAP);
		//mutex_lock para que no sea multiplexado
		atender_peticiones(cliente);
		//mutex_unlock
		//TODO: vamos a tener que encolar los requests o eso lo hace solo el hilo?
	}
	*/
}

//--------------------------------------------------------------------------------------------

t_swamp_config crear_archivo_config_swamp(char* ruta) {
    t_config* swamp_config;
    swamp_config = config_create(ruta);
    t_swamp_config config;

    if (swamp_config == NULL) {
        printf("No se pudo leer el archivo de configuracion de Swamp\n");
        exit(-1);
    }

    config.ip             = config_get_string_value(swamp_config, "IP");
    config.puerto         = config_get_string_value(swamp_config, "PUERTO");
    config.tamanio_swamp  = config_get_int_value(swamp_config, "TAMANIO_SWAP");
    config.tamanio_pag    = config_get_int_value(swamp_config, "TAMANIO_PAGINA");
    config.archivos_swamp = config_get_array_value(swamp_config,"ARCHIVOS_SWAP");
    config.marcos_max     = config_get_int_value(swamp_config, "MARCOS_MAXIMOS");
    config.retardo_swap   = config_get_int_value(swamp_config, "RETARDO_SWAP");

    return config;
}

void init_swamp(){

	//incializamos logger
	LOGGER = log_create("swamp.log", "SWAMP", 0, LOG_LEVEL_INFO);

	//inicializamos config
	CONFIG = crear_archivo_config_swamp("/home/utnso/workspace/tp-2021-2c-DesacatadOS/swamp/src/swamp.config");

	//iniciamos servidor
	SERVIDOR_SWAP = iniciar_servidor(CONFIG.ip, CONFIG.puerto);

	//Creamos archivos correspondientes
	METADATA_ARCHIVOS = list_create();
	FRAMES_SWAP = list_create();
	crear_archivos();
	crear_frames();
}

void atender_peticiones(int cliente){

	t_peticion_swap operacion = recibir_operacion_swap(cliente);

	switch (operacion) {
	case RESERVAR_ESPACIO:;



		break;

	case TRAER_DE_SWAP:;

		break;

	case TIRAR_A_SWAP:;

		void* buffer_pag = malloc(CONFIG.tamanio_pag);

		int pid = recibir_entero(cliente);
		int nro_pagina = recibir_entero(cliente);

		buffer_pag = recibir_pagina(cliente, CONFIG.tamanio_pag);

				//void* buffer = recibir_buffer(sizeof(uint32_t)*2, cliente);
				//TODO: recibir buffer y agarrar el pid ta ta taa @matihamide
				//reservar_espacio(pid, cant_pag);

		printf("\n%i  %i\n ", pid, nro_pagina);

		break;

	default:; break;

	}
}

void crear_archivos() {

	for (int i = 0 ; i < size_char_array(CONFIG.archivos_swamp) ; i++) {

		t_metadata_archivo* md_archivo = malloc(sizeof(t_metadata_archivo));
		md_archivo->id                 = i;
		md_archivo->espacio_disponible = CONFIG.tamanio_swamp;
		md_archivo->addr               = crear_archivo(CONFIG.archivos_swamp[i], CONFIG.tamanio_swamp);
		list_add(METADATA_ARCHIVOS, md_archivo);
	}

}

void crear_frames(){

	int frames_x_archivo = CONFIG.tamanio_swamp/CONFIG.tamanio_pag;

	for (int i = 0 ; i < size_char_array(CONFIG.archivos_swamp); i++){
		for(int j= 0; j < frames_x_archivo; j++){
			t_pagina* pagina = malloc(sizeof(t_pagina));
			pagina->aid = i;
			pagina->offset = j * CONFIG.tamanio_pag;
			pagina->ocupado = false;
			pagina->id = -1;
			pagina->pid =-1;

			list_add(FRAMES_SWAP, pagina);
		}
	}
}

void* crear_archivo(char* path, int size) {
	int fd;
	void* addr;

	fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0777);
	if (fd < 0) {
	  exit(EXIT_FAILURE);
	}

	ftruncate(fd, size);

	addr = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);

	if (addr == MAP_FAILED) {
		perror("Error mapping \n");
		exit(1);
	}

	//TODO Falta llenarlos con el \0 y creo que msynquear
	memset(addr, '\0', CONFIG.tamanio_swamp);

	return addr;

}

int reservar_espacio(int pid, int cant_pag) {

	int nro_archivo = archivo_proceso_existente(pid);
	t_metadata_archivo* archivo;


	if(nro_archivo != NULL){
		//Proceso existente
		archivo = (t_metadata_archivo*)list_get(METADATA_ARCHIVOS, nro_archivo);

		if(archivo->espacio_disponible >= cant_pag * CONFIG.tamanio_pag){

			for(int i = 1; i <= cant_pag; i++){

				bool pagina_libre_del_archivo(void * elemento){
					t_pagina* pag_aux = (t_pagina*) elemento;
					return !pag_aux->ocupado && pag_aux->aid == nro_archivo;
				}

				bool _ultima_pagina_proceso(void* segmento1, void* segmento2) {
					t_pagina* seg1 = (t_pagina*) segmento1;
					t_pagina* seg2 = (t_pagina*) segmento2;

					return seg1->id > seg2->id && seg1->pid == pid && seg2->pid == pid;
				}

				t_pagina* pagina = list_find(FRAMES_SWAP, pagina_libre_del_archivo);

				pagina->ocupado = true;
				pagina->pid = pid;
				pagina->id = ((t_pagina*)list_get_maximum(FRAMES_SWAP,(void*) _ultima_pagina_proceso))->id + i;

			}
			archivo->espacio_disponible -= cant_pag * CONFIG.tamanio_pag;

			log_info(LOGGER, "Se reservó %i paginas para el proceso %i",cant_pag,pid);
			return 1;
		}
			log_info(LOGGER, "No hay espacio disponible para el proceso %i",pid);
			return -1;

	}else {
		//Proceso nuevo

		archivo = obtener_archivo_mayor_espacio_libre();

		if(archivo->espacio_disponible >= cant_pag * CONFIG.tamanio_pag){

			for(int i = 0; i < cant_pag; i++){

				bool pagina_libre_del_archivo(void * elemento){
					t_pagina* pag_aux = (t_pagina*) elemento;
					return !pag_aux->ocupado && pag_aux->aid == archivo->id;
				}

				t_pagina* pagina = list_find(FRAMES_SWAP, pagina_libre_del_archivo);

				pagina->ocupado = true;
				pagina->pid = pid;
				pagina->id = i;

			}

			archivo->espacio_disponible -= cant_pag * CONFIG.tamanio_pag;

			log_info(LOGGER, "Se reservó %i paginas para el proceso %i",cant_pag,pid);
			return 1;
		}

		log_info(LOGGER, "No hay espacio disponible para el proceso %i",pid);
		return -1;
	}
}


int archivo_proceso_existente(int pid) {

	bool existe_proceso_por_pid(void* elemento) {
		t_pagina* pag_aux = (t_pagina*) elemento;
		return pag_aux->pid == pid;
	}

    return ((t_pagina*)list_find(FRAMES_SWAP, existe_proceso_por_pid))->aid;
}

t_metadata_archivo* obtener_archivo_mayor_espacio_libre() {
	bool _de_mayor_espacio(t_metadata_archivo* un_archivo, t_metadata_archivo* otro_archivo){
		return un_archivo->espacio_disponible < otro_archivo->espacio_disponible;
	}

	list_sort(METADATA_ARCHIVOS, (void*)_de_mayor_espacio);
	return (t_metadata_archivo*)list_get(METADATA_ARCHIVOS, 0);
}
