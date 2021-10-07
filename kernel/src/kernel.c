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

	coordinador_multihilo();

	iniciar_planificador_largo_plazo();




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

    if(!strcmp(config.alg_plani,"SJF")){
    	config.estimacion_inicial = 0;
    	config.alfa = 0;

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

void init_kernel(){
	//Creamos estructura archivo de configuracion
	CONFIG_KERNEL = crear_archivo_config_kernel("/home/utnso/workspace/tp-2021-2c-DesacatadOS/kernel/src/kernel.config");

	//Inicializamos logger
	LOGGER = log_create("kernel.log", "KERNEL", 0, LOG_LEVEL_INFO);

	//Iniciamos servidor
	SERVIDOR_KERNEL = iniciar_servidor(CONFIG_KERNEL.ip_kernel,CONFIG_KERNEL.puerto_kernel);

	//Conexiones
	//SERVIDOR_MEMORIA = crear_conexion(CONFIG_KERNEL.ip_memoria, CONFIG_KERNEL.puerto_memoria);

	//Listas y colas de planifiacion
	LISTA_NEW = list_create();
	LISTA_READY = list_create();
	LISTA_EXEC = list_create();
	LISTA_BLOCKED = list_create();
	LISTA_SUSPENDED_BLOCKED = list_create();
	LISTA_SUSPENDED_READY = list_create();

	//Mutexes
	pthread_mutex_init(&mutex_creacion_PID, NULL);
	pthread_mutex_init(&mutex_lista_new, NULL);
	pthread_mutex_init(&mutex_lista_ready, NULL);

	//Semaforos
	sem_init(&sem_cola_new, 0, 0);
	sem_init(&sem_cola_ready, 0, 0);
	sem_init(&sem_grado_multiprogramacion, 0, CONFIG_KERNEL.grado_multiprogramacion);

	//Variable que asigna PIDs a los nuevos carpinchos
	PID_PROX_CARPINCHO = 0;


}

/*
void coordinador_multihilo(){

	while(1) {

		int* socket = malloc(sizeof(int));
		*socket = esperar_cliente(SERVIDOR_KERNEL);

		pthread_t hilo_atender_carpincho;// = malloc(sizeof(pthread_t));
		pthread_create(&hilo_atender_carpincho , NULL , (void*)atender_carpinchos, socket);
		pthread_detach(hilo_atender_carpincho);

	}
}
*/
void coordinador_multihilo(){

	while(1) {

		int socket = esperar_cliente(SERVIDOR_KERNEL);

		pthread_t hilo_atender_carpincho;// = malloc(sizeof(pthread_t));
		pthread_create(&hilo_atender_carpincho , NULL , (void*)atender_carpinchos, (void*)socket);
		pthread_detach(hilo_atender_carpincho);

	}
}

void iniciar_planificador_largo_plazo() {


	pthread_create(&planificador_largo_plazo, NULL, (void*)algoritmo_planificador_largo_plazo, NULL);
	pthread_detach(planificador_largo_plazo);

}

void atender_carpinchos(int cliente) {

	PCB* pcb_carpincho = malloc(sizeof(PCB));

	pthread_mutex_lock(&mutex_creacion_PID);
	pcb_carpincho->PID = PID_PROX_CARPINCHO;
	PID_PROX_CARPINCHO++;
	pthread_mutex_unlock(&mutex_creacion_PID);

	pasar_a_new(pcb_carpincho);
}

void pasar_a_new(PCB* pcb_carpincho) {

	pthread_mutex_lock(&mutex_lista_new);
	list_add(LISTA_NEW, pcb_carpincho);
	pthread_mutex_unlock(&mutex_lista_new);

	sem_post(&sem_cola_new);

}

