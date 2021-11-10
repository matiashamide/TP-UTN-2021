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
		//TODO: vamos a tener que encolar los requests o eso lo hace solo el hilo?
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
	METADATA_ARCHIVOS = list_create();
	crear_archivos();
}

void atender_peticiones(int cliente) {

	peticion_swap operacion = recibir_operacion(cliente);

	switch (operacion) {
	case RESERVAR_ESPACIO:
		break;

	case TRAER_DE_SWAP:
		break;

	case TIRAR_A_SWAP:
		break;

	default:; break;

	}
}

void crear_archivos() {

	int cant_archivos = size_char_array(CONFIG.archivos_swamp);

	for (int i = 0 ; size_char_array(CONFIG.archivos_swamp) ; i++) {

		t_metadata_archivo* md_archivo = malloc(sizeof(t_metadata_archivo));
		md_archivo->id                 = i;
		md_archivo->espacio_disponible = CONFIG.archivos_swamp;
		md_archivo->addr               = crear_archivo(CONFIG.archivos_swamp[i], CONFIG.tamanio_swamp);
		list_add(METADATA_ARCHIVOS, md_archivo);
	}

}

void* crear_archivo(char* path, int size) {
	int fd;
	void* addr;

	fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0777);
	if (fd < 0) {
	  exit(EXIT_FAILURE);
	}

	ftruncate(fd, size);

	addr = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);

	if (addr == MAP_FAILED) {
		perror("Error  mapping \n");
		exit(1);
	}

	//TODO Falta llenarlos con el /0 y creo que msynquear

	return addr;

}
