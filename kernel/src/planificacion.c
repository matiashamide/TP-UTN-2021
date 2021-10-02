#include "planificacion.h"

void planificador_largo_plazo() {

	while(1) {
		sem_wait(&sem_cola_new);
		sem_wait(&sem_grado_multiprogramacion);

//TODO aca hacer algoritmo a largo plazo (pop new, push ready)

	}


}
