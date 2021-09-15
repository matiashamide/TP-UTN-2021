/*
 ============================================================================
 Name        : kernel.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "kernel.h"


int main(void) {

	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */
	return EXIT_SUCCESS;
}




//--------------------------------------------------------------------------------------------
t_kernel_config crear_archivo_config_kernel(char* ruta) {
    t_config* kernel_config;
    kernel_config = config_create(ruta);
    t_kernel_config config;

    if (kernel_config == NULL) {
        printf("No se pudo leer el archivo de configuracion de Kernel\n");
        exit(-1);
    }


    config.ip = config_get_string_value(kernel_config, "IP");
    config.puerto = config_get_int_value(kernel_config, "PUERTO");
    config.alg_plani = config_get_string_value(kernel_config, "ALGORITMO_PLANIFICACION");

    if(strcmp(config.alg_plani,"SJF")){
    	config.puerto = NULL;
    	config.puerto = NULL;

    }else{
    	config.estimacion_inicial = config_get_int_value(kernel_config, "ESTIMACION_INICIAL");
    	config.alfa = config_get_int_value(kernel_config, "ALFA");

    }
    config.dispositivos_IO = config_get_array_value(kernel_config,"DISPOSITIVOS_IO");
    config.duraciones_IO = config_get_array_value(kernel_config,"DURACIONES_IO");
    config.retardo_cpu = config_get_int_value(kernel_config, "RETARDO_CPU");
    config.grado_multiprogramacion = config_get_int_value(kernel_config, "GRADO_MULTIPROGRAMACION");
    config.grado_multiprocesamiento = config_get_int_value(kernel_config, "GRADO_MULTIPROCESAMIENTO");


    return config;
}





