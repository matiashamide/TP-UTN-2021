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

	init_kernel();

	//int *socket_cliente = malloc(sizeof(int));

	//*socket_cliente = accept(SERVIDOR_KERNEL, NULL, NULL);

	//recibir_peticion_para_continuar(*socket_cliente);
	//dar_permiso_para_continuar(*socket_cliente);

	//printf("%d\n",recibir_operacion_carpincho(*socket_cliente));


	//fflush(stdout);
	coordinador_multihilo();


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


    config.ip_memoria = config_get_string_value(kernel_config, "IP_MEMORIA");
    config.puerto_memoria = config_get_string_value(kernel_config, "PUERTO_MEMORIA");

    config.ip_kernel = config_get_string_value(kernel_config, "IP_KERNEL");
    config.puerto_kernel = config_get_string_value(kernel_config, "PUERTO_KERNEL");

    config.alg_plani = config_get_string_value(kernel_config, "ALGORITMO_PLANIFICACION");

    //if(!strcmp(config.alg_plani,"SJF")){
  //  	config.estimacion_inicial = 0;
  //  	config.alfa = 0;
//
  //  }else{
    	config.estimacion_inicial = config_get_int_value(kernel_config, "ESTIMACION_INICIAL");
    	config.alfa = config_get_int_value(kernel_config, "ALFA");

  //  }
    config.dispositivos_IO = config_get_array_value(kernel_config,"DISPOSITIVOS_IO");
    config.duraciones_IO = config_get_array_value(kernel_config,"DURACIONES_IO");
    config.tiempo_deadlock = config_get_int_value(kernel_config, "TIEMPO_DEADLOCK");
    config.grado_multiprogramacion = config_get_int_value(kernel_config, "GRADO_MULTIPROGRAMACION");
    config.grado_multiprocesamiento = config_get_int_value(kernel_config, "GRADO_MULTIPROCESAMIENTO");


    return config;
}

void init_kernel(){
	//Creamos estructura archivo de configuracion
	CONFIG_KERNEL = crear_archivo_config_kernel("/home/utnso/workspace/tp-2021-2c-DesacatadOS/kernel/src/kernel.config");

	//Inicializamos logger
	LOGGER = log_create("kernel.log", "KERNEL", 0, LOG_LEVEL_INFO);

	//Iniciamos servidor
	SERVIDOR_KERNEL = iniciar_servidor(CONFIG_KERNEL.ip_kernel,CONFIG_KERNEL.puerto_kernel);

	//Conexiones
	//SERVIDOR_MEMORIA = crear_conexion(CONFIG_KERNEL.ip_memoria, CONFIG_KERNEL.puerto_memoria);

	//Listas y colas de planificacion
	LISTA_NEW = list_create();
	LISTA_READY = list_create();
	LISTA_EXEC = list_create();
	LISTA_BLOCKED = list_create();
	LISTA_BLOCKED_SUSPENDED = list_create();
	LISTA_READY_SUSPENDED = list_create();
	LISTA_PROCESADORES = list_create();
	LISTA_SEMAFOROS_MATE = list_create();
	LISTA_DISPOSITIVOS_IO = list_create();

	//Mutexes
	pthread_mutex_init(&mutex_creacion_PID, NULL);
	pthread_mutex_init(&mutex_lista_new, NULL);
	pthread_mutex_init(&mutex_lista_ready, NULL);
	pthread_mutex_init(&mutex_lista_procesadores, NULL);
	pthread_mutex_init(&mutex_lista_semaforos_mate, NULL);
	pthread_mutex_init(&mutex_lista_dispositivos_io, NULL);

	//Semaforos
	sem_init(&sem_algoritmo_planificador_largo_plazo, 0, 0);
	sem_init(&sem_cola_ready, 0, 0);
	sem_init(&sem_grado_multiprogramacion, 0, CONFIG_KERNEL.grado_multiprogramacion);
	sem_init(&sem_grado_multiprocesamiento, 0, CONFIG_KERNEL.grado_multiprocesamiento);

	//Variable que asigna PIDs a los nuevos carpinchos
	PID_PROX_CARPINCHO = 0;

	crear_dispositivos_io();
	crear_procesadores();
	iniciar_planificador_largo_plazo();
	iniciar_planificador_corto_plazo();
	iniciar_algoritmo_deadlock();

}

void crear_dispositivos_io() {
	for(int i = 0; i < size_char_array(CONFIG_KERNEL.dispositivos_IO); i++) {
		t_dispositivo* dispositivo_io = malloc(sizeof(t_dispositivo));

		dispositivo_io->nombre = CONFIG_KERNEL.dispositivos_IO[i];
		dispositivo_io->rafaga = strtol(CONFIG_KERNEL.duraciones_IO[i], NULL, 10);
		dispositivo_io->cola_espera = list_create();
		sem_init(&dispositivo_io->sem, 0, 0);

		list_add(LISTA_DISPOSITIVOS_IO, dispositivo_io);

		pthread_t hilo_dispositivo;

		pthread_create(&hilo_dispositivo, NULL, (void*)ejecutar_io, dispositivo_io);
		printf("Cree un dispositivo\n");
		pthread_detach(hilo_dispositivo);

		//TEST
		printf("Nombre dispositivo %s\n", dispositivo_io->nombre);
		printf("Rafaga dispositivo %d\n", dispositivo_io->rafaga);
		//FIN TEST

	}
}

void coordinador_multihilo(){

	while(1) {

		pthread_t hilo_atender_carpincho;

		int *socket_cliente = malloc(sizeof(int));

		(*socket_cliente) = accept(SERVIDOR_KERNEL, NULL, NULL);

		printf("Se conecto un carpincho usando la mateLib\n");

		pthread_create(&hilo_atender_carpincho, NULL , (void*)atender_carpinchos, socket_cliente);
		pthread_detach(hilo_atender_carpincho);
	}
}

void iniciar_planificador_largo_plazo() {

	pthread_create(&planificador_largo_plazo, NULL, (void*)algoritmo_planificador_largo_plazo, NULL);
	printf("Ya cree el planificador de largo plazo\n");
	pthread_detach(planificador_largo_plazo);
}

void iniciar_planificador_corto_plazo() {

	pthread_create(&planificador_corto_plazo, NULL, (void*)algoritmo_planificador_corto_plazo, NULL);
	printf("Ya cree el planificador de corto plazo\n");
	pthread_detach(planificador_largo_plazo);
}

void iniciar_algoritmo_deadlock() {

	pthread_create(&administrador_deadlock, NULL, (void*)correr_algoritmo_deadlock, NULL);
	printf("Ya cree el algoritmo para correr deadlock\n");
	pthread_detach(administrador_deadlock);
}

void atender_carpinchos(int* cliente) {

	PCB* pcb_carpincho = malloc(sizeof(PCB));

	pthread_mutex_lock(&mutex_creacion_PID);
	pcb_carpincho->PID = PID_PROX_CARPINCHO;
	PID_PROX_CARPINCHO++;
	pthread_mutex_unlock(&mutex_creacion_PID);

	pcb_carpincho->real_anterior = 0;
	pcb_carpincho->estimado_anterior = CONFIG_KERNEL.estimacion_inicial;
	pcb_carpincho->tiempo_espera = 0;
	pcb_carpincho->conexion = (*cliente);
	pcb_carpincho->recursos_usados = list_create();
	//TODO hacerle free a esto

	printf("Conexion %d\n", (*cliente));

	recibir_peticion_para_continuar(pcb_carpincho->conexion);

	pasar_a_new(pcb_carpincho);
}

void pasar_a_new(PCB* pcb_carpincho) {

	pthread_mutex_lock(&mutex_lista_new);
	list_add(LISTA_NEW, pcb_carpincho);
	pthread_mutex_unlock(&mutex_lista_new);

	sem_post(&sem_algoritmo_planificador_largo_plazo);

	algoritmo_planificador_mediano_plazo_blocked_suspended();
}

void crear_procesadores() {

	for(int i = 0; i < CONFIG_KERNEL.grado_multiprocesamiento; i++) {

		t_procesador* estructura_procesador = malloc(sizeof(t_procesador));
		sem_init(&estructura_procesador->sem_exec, 0, 0);
		estructura_procesador->bit_de_ocupado = 0;

		list_add(LISTA_PROCESADORES, estructura_procesador);

		pthread_t hilo_procesador;

		pthread_create(&hilo_procesador, NULL, (void*)ejecutar, estructura_procesador);
		printf("Cree un procesador\n");
		pthread_detach(hilo_procesador);

	}
}





