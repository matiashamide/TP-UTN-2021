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
	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */
	return EXIT_SUCCESS;
}





//------------------General Functions---------------------/

typedef struct mate_inner_structure
{
  void *memory;
  sem_t*sem_instance;
} mate_inner_structure;

int mate_init(mate_instance *lib_ref, char *config)
{
  lib_ref->group_info = malloc(sizeof(mate_inner_structure));
  return 0;
}

int mate_close(mate_instance *lib_ref)
{
  free(lib_ref->group_info);
  return 0;
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
