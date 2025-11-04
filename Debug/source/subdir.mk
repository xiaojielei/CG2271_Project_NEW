################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/actuator_driver.c \
../source/mtb.c \
../source/music_library.c \
../source/semihost_hardfault.c \
../source/sensor.c 

C_DEPS += \
./source/actuator_driver.d \
./source/mtb.d \
./source/music_library.d \
./source/semihost_hardfault.d \
./source/sensor.d 

OBJS += \
./source/actuator_driver.o \
./source/mtb.o \
./source/music_library.o \
./source/semihost_hardfault.o \
./source/sensor.o 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.c source/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_MCXC444VLH -DCPU_MCXC444VLH_cm0plus -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DSDK_DEBUGCONSOLE_UART -DSDK_OS_FREE_RTOS -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Users\86178\Documents\MCUXpressoIDE_25.6.136\workspace\GP\board" -I"C:\Users\86178\Documents\MCUXpressoIDE_25.6.136\workspace\GP\source" -I"C:\Users\86178\Documents\MCUXpressoIDE_25.6.136\workspace\GP\drivers" -I"C:\Users\86178\Documents\MCUXpressoIDE_25.6.136\workspace\GP\CMSIS" -I"C:\Users\86178\Documents\MCUXpressoIDE_25.6.136\workspace\GP\CMSIS\m-profile" -I"C:\Users\86178\Documents\MCUXpressoIDE_25.6.136\workspace\GP\utilities" -I"C:\Users\86178\Documents\MCUXpressoIDE_25.6.136\workspace\GP\utilities\debug_console\config" -I"C:\Users\86178\Documents\MCUXpressoIDE_25.6.136\workspace\GP\device" -I"C:\Users\86178\Documents\MCUXpressoIDE_25.6.136\workspace\GP\device\periph2" -I"C:\Users\86178\Documents\MCUXpressoIDE_25.6.136\workspace\GP\utilities\debug_console" -I"C:\Users\86178\Documents\MCUXpressoIDE_25.6.136\workspace\GP\component\serial_manager" -I"C:\Users\86178\Documents\MCUXpressoIDE_25.6.136\workspace\GP\component\lists" -I"C:\Users\86178\Documents\MCUXpressoIDE_25.6.136\workspace\GP\utilities\str" -I"C:\Users\86178\Documents\MCUXpressoIDE_25.6.136\workspace\GP\component\uart" -I"C:\Users\86178\Documents\MCUXpressoIDE_25.6.136\workspace\GP\freertos\freertos-kernel\include" -I"C:\Users\86178\Documents\MCUXpressoIDE_25.6.136\workspace\GP\freertos\freertos-kernel\portable\GCC\ARM_CM0" -I"C:\Users\86178\Documents\MCUXpressoIDE_25.6.136\workspace\GP\freertos\freertos-kernel\template" -I"C:\Users\86178\Documents\MCUXpressoIDE_25.6.136\workspace\GP\freertos\freertos-kernel\template\ARM_CM0" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m0plus -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-source

clean-source:
	-$(RM) ./source/actuator_driver.d ./source/actuator_driver.o ./source/mtb.d ./source/mtb.o ./source/music_library.d ./source/music_library.o ./source/semihost_hardfault.d ./source/semihost_hardfault.o ./source/sensor.d ./source/sensor.o

.PHONY: clean-source

