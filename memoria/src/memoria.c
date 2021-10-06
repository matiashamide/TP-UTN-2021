/*
 ============================================================================
 Name        : memoria.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "memoria.h"

int main(void) {

	init_memoria();

	//enviar_mensaje("hola como estas", SERVIDOR_MEMORIA);
	//coordinador_multihilo();

	init_tlb(CONFIG.cant_entradas_tlb , CONFIG.alg_reemplazo_tlb);
	printear_TLB(CONFIG.cant_entradas_tlb);

	for(int i = 0 ; i < 5  ; i++){

	sleep(2);

	buscar_frame(i , i+1 );

	printear_TLB(CONFIG.cant_entradas_tlb);

	}
	sleep(2);
	buscar_frame( 2 , 3);
	printear_TLB(CONFIG.cant_entradas_tlb);
	sleep(2);
	buscar_frame(10,10);
	printear_TLB(CONFIG.cant_entradas_tlb);
	return EXIT_SUCCESS;
}


//--------------------------------------------------------------------------------------------

void init_memoria(){

	//Inicializamos logger
	LOGGER = log_create("memoria.log", "MEMORIA", 0, LOG_LEVEL_INFO);

	//Levantamos archivo de configuracion
	CONFIG = crear_archivo_config_memoria("/home/utnso/workspace/tp-2021-2c-DesacatadOS/memoria/src/memoria.config");

	//Iniciamos servidor
	SERVIDOR_MEMORIA = iniciar_servidor(CONFIG.ip_memoria,CONFIG.puerto_memoria);

	//Instanciamos memoria principal
	MEMORIA_PRINCIPAL = malloc(CONFIG.tamanio);

	if (MEMORIA_PRINCIPAL == NULL) {
	   	perror("MALLOC FAIL!\n");
	   	log_info(LOGGER,"No se pudo alocar correctamente la memoria principal.");
        return;
	}

	//Iniciamos paginacion
	iniciar_paginacion();
}


void coordinador_multihilo(){

	while(1){

		int socket = esperar_cliente(SERVIDOR_MEMORIA);

		pthread_t* hilo_atender_carpincho = malloc(sizeof(pthread_t));
		pthread_create(hilo_atender_carpincho , NULL , (void*)atender_carpinchos , (void*)socket);
		pthread_detach(*hilo_atender_carpincho);
	}
}

void atender_carpinchos(int cliente) {

	peticion_carpincho operacion = recibir_operacion(cliente);

	switch (operacion) {
	case MENSAJE:;
		char* mensaje = recibir_mensaje(cliente);
		printf("%s",mensaje);
		fflush(stdout);
	break;
	}
}

void iniciar_paginacion() {

	int cant_frames_ppal = CONFIG.tamanio / CONFIG.tamanio_pagina;

	log_info(LOGGER,"Tengo %d marcos de %d bytes en memoria principal",cant_frames_ppal, CONFIG.tamanio_pagina);

	TABLAS_DE_PAGINAS = list_create();
}

int buscar_pagina_en_memoria(int pid, int pag) {
	int marco;
	return -1;
}

int memalloc(int size , int pid) {
	return 0;
}

t_memoria_config crear_archivo_config_memoria(char* ruta) {
    t_config* memoria_config;
    memoria_config = config_create(ruta);
    t_memoria_config config;

    if (memoria_config == NULL) {
        log_info(LOGGER, "No se pudo leer el archivo de configuracion de memoria\n");
        exit(-1);
    }

    config.ip_memoria = config_get_string_value(memoria_config, "IP");
    config.puerto_memoria = config_get_string_value(memoria_config, "PUERTO");
    config.tamanio = config_get_int_value(memoria_config, "TAMANIO");
    config.tamanio_pagina = config_get_int_value(memoria_config, "TAMANIO_PAGINA");
    config.alg_remp_mmu = config_get_string_value(memoria_config, "ALGORITMO_REEMPLAZO_MMU");
    config.tipo_asignacion = config_get_string_value(memoria_config, "TIPO_ASIGNACION");
    //config.marcos_max = config_get_int_value(memoria_config, "MARCOS_MAXIMOS");  --> ver en que casos esta esta esta config y meter un if
    config.cant_entradas_tlb = config_get_int_value(memoria_config, "CANTIDAD_ENTRADAS_TLB");
    config.alg_reemplazo_tlb = config_get_string_value(memoria_config, "ALGORITMO_REEMPLAZO_TLB");
    config.retardo_acierto_tlb = config_get_int_value(memoria_config, "RETARDO_ACIERTO_TLB");
    config.retardo_fallo_tlb = config_get_int_value(memoria_config, "RETARDO_FALLO_TLB");

    return config;
}

