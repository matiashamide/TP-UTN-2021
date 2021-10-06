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

	//Creamos estructura archivo de configuracion
	CONFIG = crear_archivo_config_memoria("/home/utnso/workspace/tp-2021-2c-DesacatadOS/memoria/src/memoria.config");

	//Inicializamos logger
	LOGGER = log_create("memoria.log", "MEMORIA", 0, LOG_LEVEL_INFO);

	//Iniciamos servidor
	SERVIDOR_MEMORIA = iniciar_servidor(CONFIG.ip_memoria,CONFIG.puerto_memoria);

	//Instanciamos memoria principal
	MEMORIA_PRINCIPAL = malloc(CONFIG.tamanio);
	if (MEMORIA_PRINCIPAL == NULL) {
		//NO SE RESERVO LA MEMORIA
	   	perror("MALLOC FAIL!\n");
        return;
	}

	//Iniciamos paginacion
	iniciar_paginacion();

}


t_memoria_config crear_archivo_config_memoria(char* ruta) {
    t_config* memoria_config;
    memoria_config = config_create(ruta);
    t_memoria_config config;

    if (memoria_config == NULL) {
        printf("No se pudo leer el archivo de configuracion de Memoria\n");
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


void coordinador_multihilo(){

	while(1){

		int socket = esperar_cliente(SERVIDOR_MEMORIA);


		pthread_t* hilo_atender_carpincho = malloc(sizeof(pthread_t));
		pthread_create(hilo_atender_carpincho , NULL , (void*)atender_carpinchos , (void*)socket);
		pthread_detach(*hilo_atender_carpincho);

	}
}

void atender_carpinchos(int cliente){

	peticion_carpincho operacion = recibir_operacion(cliente);

	switch (operacion) {

	case MENSAJE:;
			char* mensaje = recibir_mensaje(cliente);
				printf("%s",mensaje);
				fflush(stdout);
				break;

	}
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

int iniciar_servidor(char* IP, char* PUERTO)
{
   int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(IP, PUERTO, &hints, &servinfo);

    for (p=servinfo; p != NULL; p = p->ai_next)
    {
        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_servidor);
            continue;
        }
        break;
    }

   listen(socket_servidor, SOMAXCONN);

    freeaddrinfo(servinfo);

    printf("Listo para escuchar a mi cliente\n");

    return socket_servidor;
}


int esperar_cliente(int socket_servidor)
{
  struct sockaddr_in dir_cliente;
  int tam_direccion = sizeof(struct sockaddr_in);

  int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);


  printf("Se conecto un cliente\n");

  return socket_cliente;
}

int recibir_operacion(int socket_cliente)
{
   int cod_op;
   if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) != 0){
      return cod_op;
   }
   else
   {
      close(socket_cliente);
      return -1;
   }
}


char* recibir_mensaje(int socket_cliente)
{
   uint32_t size;
   char* buffer = recibir_buffer(&size, socket_cliente);
   return buffer;
}

void enviar_mensaje(char* mensaje, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

void* recibir_buffer(uint32_t* size, int socket_cliente)
{
   void * buffer;

   recv(socket_cliente, size, sizeof(uint32_t), MSG_WAITALL);
   buffer = malloc(*size);
   recv(socket_cliente, buffer, *size, MSG_WAITALL);

   return buffer;
}

void* serializar_paquete(t_paquete* paquete, int* bytes)
{

   int size_serializado = sizeof(peticion_carpincho) + sizeof(uint32_t) + paquete->buffer->size;
   void *buffer = malloc(size_serializado);

   int offset = 0;

   memcpy(buffer + offset, &(paquete->codigo_operacion), sizeof(int));
   offset+= sizeof(int);
   memcpy(buffer + offset, &(paquete->buffer->size), sizeof(uint32_t));
   offset+= sizeof(uint32_t);
   memcpy(buffer + offset, paquete->buffer->stream, paquete->buffer->size);
   offset+= paquete->buffer->size;

   (*bytes) = size_serializado;
   return buffer;
}

void iniciar_paginacion() {

	int cant_frames_ppal = CONFIG.tamanio / CONFIG.tamanio_pagina;

	log_info(LOGGER,"Tengo %d marcos de %d bytes en memoria principal",cant_frames_ppal, CONFIG.tamanio_pagina);

	TABLAS_DE_PAGINAS = list_create();


}


void eliminar_paquete(t_paquete* paquete) {
   free(paquete->buffer->stream);
   free(paquete->buffer);
   free(paquete);
}

int buscar_pagina_en_memoria(int pid, int pag){
	int marco;


	return -1;
}


