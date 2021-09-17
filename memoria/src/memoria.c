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

	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */

	t_memoria_config config = crear_archivo_config_memoria("/home/utnso/workspace/tp-2021-2c-DesacatadOS/memoria/src/memoria.config");

	printf("el puerto es: %d \n", config.puerto);
	printf("el ip es: %s\n", config.ip);
	printf("el alg de remplazo es: %s\n", config.alg_reemplazo_tlb);



	return EXIT_SUCCESS;
}




//--------------------------------------------------------------------------------------------
t_memoria_config crear_archivo_config_memoria(char* ruta) {
    t_config* memoria_config;
    memoria_config = config_create(ruta);
    t_memoria_config config;

    if (memoria_config == NULL) {
        printf("No se pudo leer el archivo de configuracion de Memoria\n");
        exit(-1);
    }


    config.ip = config_get_string_value(memoria_config, "IP");
    config.puerto = config_get_int_value(memoria_config, "PUERTO");
    config.tamanio = config_get_int_value(memoria_config, "TAMANIO");
    config.alg_remp_mmu = config_get_string_value(memoria_config, "ALGORITMO_REEMPLAZO_MMU");
    config.tipo_asignacion = config_get_string_value(memoria_config, "TIPO_ASIGNACION");
    //config.marcos_max = config_get_int_value(memoria_config, "MARCOS_MAXIMOS");  --> ver en que casos esta esta esta config y meter un if
    config.cant_entradas_tlb = config_get_int_value(memoria_config, "CANTIDAD_ENTRADAS_TLB");
    config.alg_reemplazo_tlb = config_get_string_value(memoria_config, "ALGORITMO_REEMPLAZO_TLB");
    config.retardo_acierto_tlb = config_get_int_value(memoria_config, "RETARDO_ACIERTO_TLB");
    config.retardo_fallo_tlb = config_get_int_value(memoria_config, "RETARDO_FALLO_TLB");

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
