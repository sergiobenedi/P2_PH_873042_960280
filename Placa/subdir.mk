################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../8led.c \
../button.c \
../led.c \
../main.c \
../reversi8_2026.c \
../timer.c \
../timer2.c 

ASM_SRCS += \
../reversi8_arm.asm \
../reversi8_thumb.asm 

OBJS += \
./8led.o \
./button.o \
./led.o \
./main.o \
./reversi8_2026.o \
./reversi8_arm.o \
./reversi8_thumb.o \
./timer.o \
./timer2.o 

C_DEPS += \
./8led.d \
./button.d \
./led.d \
./main.d \
./reversi8_2026.d \
./timer.d \
./timer2.d 

ASM_DEPS += \
./reversi8_arm.d \
./reversi8_thumb.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Sourcery Windows GCC C Compiler'
	arm-none-eabi-gcc -I"C:/Users/sergi/Desktop/UNIZAR/3_INGENIERIA_INFORMATICA/1ER_CUATRIMESTRE/3_PROYECTO_HARDWARE/PRACTICAS/P1_873042_960280/common" -Os -Wall -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -mapcs-frame -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=arm7tdmi -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../%.asm
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Sourcery Windows GCC Assembler'
	arm-none-eabi-gcc -x assembler-with-cpp -I"C:/Users/sergi/Desktop/UNIZAR/3_INGENIERIA_INFORMATICA/1ER_CUATRIMESTRE/3_PROYECTO_HARDWARE/PRACTICAS/P1_873042_960280/common" -Wall -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=arm7tdmi -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


