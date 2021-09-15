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
	char* string = string_new();
	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */
	return EXIT_SUCCESS;
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

    config.ip = config_get_string_value(swamp_config, "IP");
    config.puerto = config_get_int_value(swamp_config, "PUERTO");
    config.tamanio_swamp = config_get_int_value(swamp_config, "TAMANIO_SWAP");
    config.tamanio_pag = config_get_int_value(swamp_config, "TAMANIO_PAGINA");
    config.archivos_swamp = config_get_array_value(swamp_config,"ARCHIVOS_SWAP");
    config.marcos_max = config_get_int_value(swamp_config, "MARCOS_MAXIMOS");
    config.retardo_swap = config_get_int_value(swamp_config, "RETARDO_SWAP");

    return config;
}

