CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_PROCESSOR cortex-m4)
#SET(CMAKE_VERBOSE_MAKEFILE ON)

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

SET(CMAKE_C_COMPILER "E:/env-windows-v2.0.0/env-windows/tools/bin/../../tools/gnu_gcc/arm_gcc/mingw/bin/arm-none-eabi-gcc.exe")
SET(CMAKE_ASM_COMPILER "E:/env-windows-v2.0.0/env-windows/tools/bin/../../tools/gnu_gcc/arm_gcc/mingw/bin/arm-none-eabi-gcc.exe")
SET(CMAKE_C_FLAGS " -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -ffunction-sections -fdata-sections -Dgcc -O0 -gdwarf-2 -g")
SET(CMAKE_ASM_FLAGS " -c -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -ffunction-sections -fdata-sections -x assembler-with-cpp -Wa,-mimplicit-it=thumb  -gdwarf-2")
SET(CMAKE_C_COMPILER_WORKS TRUE)

SET(CMAKE_CXX_COMPILER "E:/env-windows-v2.0.0/env-windows/tools/bin/../../tools/gnu_gcc/arm_gcc/mingw/bin/arm-none-eabi-g++.exe")
SET(CMAKE_CXX_FLAGS " -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -ffunction-sections -fdata-sections -Dgcc -O0 -gdwarf-2 -g")
SET(CMAKE_CXX_COMPILER_WORKS TRUE)

SET(CMAKE_OBJCOPY "E:/env-windows-v2.0.0/env-windows/tools/bin/../../tools/gnu_gcc/arm_gcc/mingw/bin/arm-none-eabi-objcopy.exe")
SET(CMAKE_SIZE "E:/env-windows-v2.0.0/env-windows/tools/bin/../../tools/gnu_gcc/arm_gcc/mingw/bin/arm-none-eabi-size.exe")

SET(CMAKE_EXE_LINKER_FLAGS " -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -ffunction-sections -fdata-sections -Wl,--gc-sections,-Map=rt-thread.map,-cref,-u,Reset_Handler -T ${CMAKE_SOURCE_DIR}/board/linker_scripts/link.lds -Wl,--gc-sections,--print-memory-usage")

SET(CMAKE_C_STANDARD 11)
SET(CMAKE_CXX_STANDARD 17)

PROJECT(rtthread C CXX ASM)

INCLUDE_DIRECTORIES(
	packages/agile_modbus-v1.1.2/inc
	packages/agile_modbus-v1.1.2/util
	applications
	applications/Inc
	rt-thread/components/libc/compilers/common/include
	rt-thread/components/libc/compilers/newlib
	packages/crclib-v1.02/inc
	rt-thread/components/drivers/include
	board
	board/CubeMX_Config/Inc
	libraries/HAL_Drivers/drivers
	libraries/HAL_Drivers/drivers/config
	libraries/HAL_Drivers
	libraries/HAL_Drivers/CMSIS/Include
	rt-thread/components/finsh
	.
	rt-thread/include
	rt-thread/libcpu/arm/common
	rt-thread/libcpu/arm/cortex-m4
	libraries/STM32F4xx_HAL/STM32F4xx_HAL_Driver/Inc
	libraries/STM32F4xx_HAL/CMSIS/Device/ST/STM32F4xx/Include
	rt-thread/components/libc/posix/io/epoll
	rt-thread/components/libc/posix/io/eventfd
	rt-thread/components/libc/posix/io/poll
	rt-thread/components/libc/posix/ipc
	packages/rs485-v1.06/inc
	rt-thread/components/utilities/ulog
)

ADD_DEFINITIONS(
	-DRT_USING_LIBC
	-DRT_USING_NEWLIBC
	-DSTM32F407xx
	-DUSE_HAL_DRIVER
	-D_POSIX_C_SOURCE=1
	-D__RTTHREAD__
)

SET(PROJECT_SOURCES
	packages/agile_modbus-v1.1.2/src/agile_modbus_rtu.c
	packages/agile_modbus-v1.1.2/util/agile_modbus_slave_util.c
	packages/agile_modbus-v1.1.2/src/agile_modbus.c
	packages/agile_modbus-v1.1.2/src/agile_modbus_tcp.c
	applications/user_modbus.c
	applications/user_can.c
	applications/main.c
	rt-thread/components/libc/compilers/common/cctype.c
	rt-thread/components/libc/compilers/common/cstdlib.c
	rt-thread/components/libc/compilers/common/cstring.c
	rt-thread/components/libc/compilers/common/ctime.c
	rt-thread/components/libc/compilers/common/cunistd.c
	rt-thread/components/libc/compilers/common/cwchar.c
	rt-thread/components/libc/compilers/newlib/syscalls.c
	packages/crclib-v1.02/src/crc32_ex.c
	packages/crclib-v1.02/src/crc8_ex.c
	packages/crclib-v1.02/src/crc16_ex_sample.c
	packages/crclib-v1.02/src/crc16.c
	packages/crclib-v1.02/src/crc16_ex.c
	packages/crclib-v1.02/src/crc8.c
	packages/crclib-v1.02/src/crc32.c
	rt-thread/components/drivers/can/can.c
	rt-thread/components/drivers/core/device.c
	rt-thread/components/drivers/ipc/completion_comm.c
	rt-thread/components/drivers/ipc/completion_up.c
	rt-thread/components/drivers/ipc/condvar.c
	rt-thread/components/drivers/ipc/dataqueue.c
	rt-thread/components/drivers/ipc/pipe.c
	rt-thread/components/drivers/ipc/ringblk_buf.c
	rt-thread/components/drivers/ipc/ringbuffer.c
	rt-thread/components/drivers/ipc/waitqueue.c
	rt-thread/components/drivers/ipc/workqueue.c
	rt-thread/components/drivers/pin/pin.c
	rt-thread/components/drivers/serial/serial.c
	board/CubeMX_Config/Src/stm32f4xx_hal_msp.c
	board/board.c
	libraries/HAL_Drivers/drivers/drv_can.c
	libraries/HAL_Drivers/drivers/drv_gpio.c
	libraries/HAL_Drivers/drivers/drv_usart.c
	libraries/HAL_Drivers/drv_common.c
	libraries/STM32F4xx_HAL/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f407xx.s
	rt-thread/components/finsh/msh.c
	rt-thread/components/finsh/cmd.c
	rt-thread/components/finsh/shell.c
	rt-thread/components/finsh/msh_parse.c
	rt-thread/src/clock.c
	rt-thread/src/components.c
	rt-thread/src/cpu_up.c
	rt-thread/src/idle.c
	rt-thread/src/ipc.c
	rt-thread/src/irq.c
	rt-thread/src/klibc/kstdio.c
	rt-thread/src/klibc/kstring.c
	rt-thread/src/kservice.c
	rt-thread/src/mem.c
	rt-thread/src/mempool.c
	rt-thread/src/object.c
	rt-thread/src/scheduler_comm.c
	rt-thread/src/scheduler_up.c
	rt-thread/src/thread.c
	rt-thread/src/timer.c
	rt-thread/libcpu/arm/common/atomic_arm.c
	rt-thread/libcpu/arm/common/div0.c
	rt-thread/libcpu/arm/common/showmem.c
	rt-thread/libcpu/arm/cortex-m4/context_gcc.S
	rt-thread/libcpu/arm/cortex-m4/cpuport.c
	libraries/STM32F4xx_HAL/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
	libraries/STM32F4xx_HAL/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
	libraries/STM32F4xx_HAL/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
	libraries/STM32F4xx_HAL/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
	libraries/STM32F4xx_HAL/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_crc.c
	libraries/STM32F4xx_HAL/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
	libraries/STM32F4xx_HAL/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
	libraries/STM32F4xx_HAL/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
	libraries/STM32F4xx_HAL/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
	libraries/STM32F4xx_HAL/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
	libraries/STM32F4xx_HAL/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cryp.c
	libraries/STM32F4xx_HAL/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rng.c
	libraries/STM32F4xx_HAL/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cec.c
	libraries/STM32F4xx_HAL/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_usart.c
	libraries/STM32F4xx_HAL/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_can.c
	libraries/STM32F4xx_HAL/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
	libraries/STM32F4xx_HAL/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
	libraries/STM32F4xx_HAL/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cryp_ex.c
	packages/rs485-v1.06/src/rs485_sample_slave.c
	packages/rs485-v1.06/src/rs485_sample_master.c
	packages/rs485-v1.06/src/rs485.c
	packages/rs485-v1.06/src/rs485_test.c
	packages/rt_vsnprintf_full-latest/rt_vsnprintf.c
	rt-thread/components/utilities/ulog/backend/console_be.c
	rt-thread/components/utilities/ulog/ulog.c
)

LINK_DIRECTORIES(
)

LINK_LIBRARIES(
	c
	m
)

ADD_EXECUTABLE(${CMAKE_PROJECT_NAME}.elf ${PROJECT_SOURCES})
ADD_CUSTOM_COMMAND(TARGET ${CMAKE_PROJECT_NAME}.elf POST_BUILD 
	COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_PROJECT_NAME}.elf rtthread.bin
	COMMAND ${CMAKE_SIZE} ${CMAKE_PROJECT_NAME}.elf
)

# if custom.cmake is exist, add it
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/custom.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/custom.cmake)
endif()
