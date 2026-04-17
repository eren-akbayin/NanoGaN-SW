################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Parameters/parameters_list.c 

OBJS += \
./Parameters/parameters_list.o 

C_DEPS += \
./Parameters/parameters_list.d 


# Each subdirectory must supply rules for building sources it contributes
Parameters/%.o Parameters/%.su Parameters/%.cyclo: ../Parameters/%.c Parameters/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H723xx -DTX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../AZURE_RTOS/App -I../Middlewares/ST/threadx/common/inc/ -I../Middlewares/ST/threadx/ports/cortex_m7/gnu/inc/ -I../Middlewares/ST/CMSIS/DSP/Include -I"C:/dev/NanoGaN-SW/Parameters" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Parameters

clean-Parameters:
	-$(RM) ./Parameters/parameters_list.cyclo ./Parameters/parameters_list.d ./Parameters/parameters_list.o ./Parameters/parameters_list.su

.PHONY: clean-Parameters

