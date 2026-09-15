################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
ASM_SRCS += \
../emulador/init_b_2026.asm 

OBJS += \
./emulador/init_b_2026.o 

ASM_DEPS += \
./emulador/init_b_2026.d 


# Each subdirectory must supply rules for building sources it contributes
emulador/%.o: ../emulador/%.asm
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Sourcery Windows GCC Assembler'
	arm-none-eabi-gcc -x assembler-with-cpp -I"C:\Users\sergi\Desktop\UNIZAR\3_INGENIERIA_INFORMATICA\1ER_CUATRIMESTRE\3_PROYECTO_HARDWARE\PRACTICAS\P1_873042_960280\common" -Wall -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=arm7tdmi -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


