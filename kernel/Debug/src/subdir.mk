################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/kernel.c \
../src/mensajesMateLib.c \
../src/planificacion.c 

OBJS += \
./src/kernel.o \
./src/mensajesMateLib.o \
./src/planificacion.o 

C_DEPS += \
./src/kernel.d \
./src/mensajesMateLib.d \
./src/planificacion.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2021-2c-DesacatadOS/futiles" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


