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

	while(1){
		int cliente = esperar_cliente(SERVIDOR_SWAP);
		//mutex_lock para que no sea multiplexado
		atender_peticiones(cliente);
		//mutex_unlock
	}
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

	//Ceamos archivos correspondientes
	crear_archivos();
}

void atender_peticiones(int cliente) {

	peticion_swap operacion = recibir_operacion(cliente);

	switch (operacion) {

	case TRAER_DE_SWAP:

	break;

	case TIRAR_A_SWAP:

	break;

	default:
	; break;

	}
}

void crear_archivos(){

	char* vector_de_paths[strlen(CONFIG.archivos_swamp)];

	strcpy(vector_de_paths[0] , CONFIG.archivos_swamp[0]);

	int cant_archivos = strlen(vector_de_paths);

	for(int i = 0 ; vector_de_paths[i] != NULL ; i++){


		strcpy(vector_de_paths[0] , CONFIG.archivos_swamp[0]);
	}

}
