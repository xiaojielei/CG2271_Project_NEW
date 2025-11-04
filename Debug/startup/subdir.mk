################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../startup/startup_mcxc444.c 

C_DEPS += \
./startup/startup_mcxc444.d 

OBJS += \
./startup/startup_mcxc444.o 


# Each subdirectory must supply rules for building sources it contributes
startup/%.o: ../startup/%.c startup/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_MCXC444VLH -DCPU_MCXC444VLH_cm0plus -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DSDK_DEBUGCONSOLE_UART -DSDK_OS_FREE_RTOS -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"/Users/joyliu/Desktop/code/CG2271_Project_NEW/board" -I"/Users/joyliu/Desktop/code/CG2271_Project_NEW/source" -I"/Users/joyliu/Desktop/code/CG2271_Project_NEW/drivers" -I"/Users/joyliu/Desktop/code/CG2271_Project_NEW/CMSIS" -I"/Users/joyliu/Desktop/code/CG2271_Project_NEW/CMSIS/m-profile" -I"/Users/joyliu/Desktop/code/CG2271_Project_NEW/utilities" -I"/Users/joyliu/Desktop/code/CG2271_Project_NEW/utilities/debug_console/config" -I"/Users/joyliu/Desktop/code/CG2271_Project_NEW/device" -I"/Users/joyliu/Desktop/code/CG2271_Project_NEW/device/periph2" -I"/Users/joyliu/Desktop/code/CG2271_Project_NEW/utilities/debug_console" -I"/Users/joyliu/Desktop/code/CG2271_Project_NEW/component/serial_manager" -I"/Users/joyliu/Desktop/code/CG2271_Project_NEW/component/lists" -I"/Users/joyliu/Desktop/code/CG2271_Project_NEW/utilities/str" -I"/Users/joyliu/Desktop/code/CG2271_Project_NEW/component/uart" -I"/Users/joyliu/Desktop/code/CG2271_Project_NEW/freertos/freertos-kernel/include" -I"/Users/joyliu/Desktop/code/CG2271_Project_NEW/freertos/freertos-kernel/portable/GCC/ARM_CM0" -I"/Users/joyliu/Desktop/code/CG2271_Project_NEW/freertos/freertos-kernel/template" -I"/Users/joyliu/Desktop/code/CG2271_Project_NEW/freertos/freertos-kernel/template/ARM_CM0" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m0plus -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-startup

clean-startup:
	-$(RM) ./startup/startup_mcxc444.d ./startup/startup_mcxc444.o

.PHONY: clean-startup

