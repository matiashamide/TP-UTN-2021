/*
 ============================================================================
 Name        : lib.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "lib.h"

int main(void) {
	 mate_instance* lib;
	 int status = mate_init(lib, CONFIG_PATH);

	 if(status != 0) {
	        printf("Algo salio mal");
	        exit(-1);
	    }


	return EXIT_SUCCESS;
}





//------------------General Functions---------------------/

int mate_init(mate_instance *lib_ref, char *config)
{
	t_lib_config lib_config = crear_archivo_config_lib(config);

    //Conexiones
	int conexion_kernel = crear_conexion(lib_config.ip_kernel, lib_config.puerto_kernel);

	if(conexion_kernel == (-1)) {

		        printf("No se pudo conectar con el Kernel, inicio de conexion con Memoria");

		        int conexion_memoria = crear_conexion(lib_config.ip_memoria, lib_config.puerto_memoria);

		        if(conexion_memoria == (-1)) {

		        		        printf("Error al conectar con Memoria");
		        		        return 1;
		    }
	}

  //Reserva memoria del tamanio de estructura administrativa
  lib_ref->group_info = malloc(sizeof(mate_inner_structure));

  if(lib_ref->group_info == NULL){
	  return 1;
  }

  return 0;
}

int mate_close(mate_instance *lib_ref)
{
  free(lib_ref->group_info);
  return 0;
}

t_lib_config crear_archivo_config_lib(char* ruta) {
    t_config* lib_config;
    lib_config = config_create(ruta);
    t_lib_config config;

    if (lib_config == NULL) {
        printf("No se pudo leer el archivo de configuracion de Lib\n");
        exit(-1);
    }

    config.ip_kernel = config_get_string_value(lib_config, "IP_KERNEL");
    config.puerto_kernel = config_get_int_value(lib_config, "PUERTO_KERNEL");
    config.ip_memoria = config_get_string_value(lib_config, "IP_MEMORIA");
    config.puerto_memoria = config_get_int_value(lib_config, "PUERTO_MEMORIA");

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


//-----------------Semaphore Functions---------------------/

int mate_sem_init(mate_instance *lib_ref, mate_sem_name sem, unsigned int value) {
  if (strncmp(sem, "SEM1", 4))
  {
    return -1;
  }
  ((mate_inner_structure *)lib_ref->group_info)->sem_instance = malloc(sizeof(sem_t));
  sem_init(((mate_inner_structure *)lib_ref->group_info)->sem_instance, 0, value);
  return 0;
}

int mate_sem_wait(mate_instance *lib_ref, mate_sem_name sem) {
  if (strncmp(sem, "SEM1", 4))
  {
    return -1;
  }
  return sem_wait(((mate_inner_structure *)lib_ref->group_info)->sem_instance);

}

int mate_sem_post(mate_instance *lib_ref, mate_sem_name sem) {
  if (strncmp(sem, "SEM1", 4))
  {
    return -1;
  }
  return sem_post(((mate_inner_structure *)lib_ref->group_info)->sem_instance);
}

int mate_sem_destroy(mate_instance *lib_ref, mate_sem_name sem) {
  if (strncmp(sem, "SEM1", 4))
  {
    return -1;
  }
  int res = sem_destroy(((mate_inner_structure *)lib_ref->group_info)->sem_instance);
  free(((mate_inner_structure *)lib_ref->group_info)->sem_instance);
  return res;
}

//--------------------IO Functions------------------------/

int mate_call_io(mate_instance *lib_ref, mate_io_resource io, void *msg)
{
  printf("Doing IO %s...\n", io);
  usleep(10 * 1000);
  if (!strncmp(io, "PRINTER", 7))
  {
    printf("Printing content: %s\n", (char *)msg);
  }
  printf("Done with IO %s\n", io);
  return 0;
}

//--------------Memory Module Functions-------------------/

mate_pointer mate_memalloc(mate_instance *lib_ref, int size)
{
  ((mate_inner_structure *)lib_ref->group_info)->memory = malloc(size);
  return 0;
}

int mate_memfree(mate_instance *lib_ref, mate_pointer addr)
{
  if (addr != 0)
  {
    return -1;
  }
  free(((mate_inner_structure *)lib_ref->group_info)->memory);
  return 0;
}

int mate_memread(mate_instance *lib_ref, mate_pointer origin, void *dest, int size)
{
  if (origin != 0)
  {
    return -1;
  }
  memcpy(dest, ((mate_inner_structure *)lib_ref->group_info)->memory, size);
  return 0;
}

int mate_memwrite(mate_instance *lib_ref, void *origin, mate_pointer dest, int size)
{
  if (dest != 0)
  {
    return -1;
  }
  memcpy(((mate_inner_structure *)lib_ref->group_info)->memory, origin, size);
  return 0;
}
