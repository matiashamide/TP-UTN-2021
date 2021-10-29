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

    config.ip = config_get_string_value(swamp_config, "IP");
    config.puerto = config_get_string_value(swamp_config, "PUERTO");
    config.tamanio_swamp = config_get_int_value(swamp_config, "TAMANIO_SWAP");
    config.tamanio_pag = config_get_int_value(swamp_config, "TAMANIO_PAGINA");
    config.archivos_swamp = config_get_array_value(swamp_config,"ARCHIVOS_SWAP");
    config.marcos_max = config_get_int_value(swamp_config, "MARCOS_MAXIMOS");
    config.retardo_swap = config_get_int_value(swamp_config, "RETARDO_SWAP");

    return config;
}

int crear_conexion(char *ip, char* puerto)
{
   struct addrinfo hints;
   struct addrinfo *server_info;

   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;

   getaddrinfo(ip, puerto, &hints, &server_info);

   int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

   if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
      printf("No se pudo conectar\n");

   freeaddrinfo(server_info);

   return socket_cliente;
}

void init_swamp(){

	//incializamos logger
	LOGGER = log_create("swamp.log", "SWAMP", 0, LOG_LEVEL_INFO);

	//inicializamos config
	CONFIG = crear_archivo_config_swamp("/home/utnso/workspace/tp-2021-2c-DesacatadOS/swamp/src/swamp.config");

	//iniciamos servidor
	SERVIDOR_SWAP = iniciar_servidor(CONFIG.ip, CONFIG.puerto);
}

void atender_peticiones(int cliente) {

	peticion_carpincho operacion = recibir_operacion(cliente);

	switch (operacion) {

	case SWAP_IN:

	break;

	case SWAP_OUT:

	break;

	default:
	; break;

	}
}
