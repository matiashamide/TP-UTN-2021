/*
 * mensajesMateLib.h
 *
 *  Created on: 3 nov. 2021
 *      Author: utnso
 */

#ifndef MENSAJESMATELIB_H_
#define MENSAJESMATELIB_H_

#include "planificacion.h"

void dar_permiso_para_continuar(int conexion);
void init_sem(PCB*);
void wait_sem(PCB*);
void post_sem(PCB*);
void destroy_sem(PCB*);
void call_IO(PCB*);
void memalloc(PCB*);
void memread(PCB*);
void memfree(PCB*);
void memwrite(PCB*);


#endif /* MENSAJESMATELIB_H_ */
