# UWB Module Firmware
A nice tutorial on getting started with STM32 and Eclipsed, created by Mohammed Shalaby, can be found [here](./doc/stm32_tutorial.md).

In this branch, we will attempt to make the workflow editor-independent. This will involve

1. Creating a `makefile` and running the make command to compile the code.
2. Launching `openocd`
3. Connecting GDB to openocd.

All of which can be done from the terminal.

## Generating the starter code with CubeMX
Inside the directory `./cubemx-out/` there is a `.ioc` file. This file can be loaded inside the CubeMX software, after which one simply has to click on the __GENERATE CODE__ button and it will populate the entire `./cubemx-out/` directory with the following files. 

```
cubemx-out
├── config_stm32f4.ioc
├── config_stm32f4.xml
├── Drivers
│   ├── CMSIS
│   │   ├── Device
│   │   │   └── ST
│   │   │       └── STM32F4xx
│   │   │           ├── Include
│   │   │           │   ├── stm32f405xx.h
│   │   │           │   ├── stm32f4xx.h
│   │   │           │   └── system_stm32f4xx.h
│   │   │           └── Source
│   │   │               └── Templates
│   │   └── Include
│   │       ├── cmsis_armcc.h
│   │       ├── cmsis_armclang.h
│   │       ├── cmsis_compiler.h
│   │       ├── cmsis_gcc.h
│   │       ├── cmsis_iccarm.h
│   │       ├── cmsis_version.h
│   │       ├── core_armv8mbl.h
│   │       ├── core_armv8mml.h
│   │       ├── core_cm0.h
│   │       ├── core_cm0plus.h
│   │       ├── core_cm1.h
│   │       ├── core_cm23.h
│   │       ├── core_cm33.h
│   │       ├── core_cm3.h
│   │       ├── core_cm4.h
│   │       ├── core_cm7.h
│   │       ├── core_sc000.h
│   │       ├── core_sc300.h
│   │       ├── mpu_armv7.h
│   │       ├── mpu_armv8.h
│   │       └── tz_context.h
│   └── STM32F4xx_HAL_Driver
│       ├── Inc
│       │   ├── Legacy
│       │   │   └── stm32_hal_legacy.h
│       │   ├── stm32f4xx_hal_cortex.h
│       │   ├── stm32f4xx_hal_def.h
│       │   ├── stm32f4xx_hal_dma_ex.h
│       │   ├── stm32f4xx_hal_dma.h
│       │   ├── stm32f4xx_hal_exti.h
│       │   ├── stm32f4xx_hal_flash_ex.h
│       │   ├── stm32f4xx_hal_flash.h
│       │   ├── stm32f4xx_hal_flash_ramfunc.h
│       │   ├── stm32f4xx_hal_gpio_ex.h
│       │   ├── stm32f4xx_hal_gpio.h
│       │   ├── stm32f4xx_hal.h
│       │   ├── stm32f4xx_hal_i2c_ex.h
│       │   ├── stm32f4xx_hal_i2c.h
│       │   ├── stm32f4xx_hal_pcd_ex.h
│       │   ├── stm32f4xx_hal_pcd.h
│       │   ├── stm32f4xx_hal_pwr_ex.h
│       │   ├── stm32f4xx_hal_pwr.h
│       │   ├── stm32f4xx_hal_rcc_ex.h
│       │   ├── stm32f4xx_hal_rcc.h
│       │   ├── stm32f4xx_hal_spi.h
│       │   ├── stm32f4xx_hal_tim_ex.h
│       │   ├── stm32f4xx_hal_tim.h
│       │   └── stm32f4xx_ll_usb.h
│       └── Src
│           ├── stm32f4xx_hal.c
│           ├── stm32f4xx_hal_cortex.c
│           ├── stm32f4xx_hal_dma.c
│           ├── stm32f4xx_hal_dma_ex.c
│           ├── stm32f4xx_hal_exti.c
│           ├── stm32f4xx_hal_flash.c
│           ├── stm32f4xx_hal_flash_ex.c
│           ├── stm32f4xx_hal_flash_ramfunc.c
│           ├── stm32f4xx_hal_gpio.c
│           ├── stm32f4xx_hal_i2c.c
│           ├── stm32f4xx_hal_i2c_ex.c
│           ├── stm32f4xx_hal_pcd.c
│           ├── stm32f4xx_hal_pcd_ex.c
│           ├── stm32f4xx_hal_pwr.c
│           ├── stm32f4xx_hal_pwr_ex.c
│           ├── stm32f4xx_hal_rcc.c
│           ├── stm32f4xx_hal_rcc_ex.c
│           ├── stm32f4xx_hal_spi.c
│           ├── stm32f4xx_hal_tim.c
│           ├── stm32f4xx_hal_tim_ex.c
│           └── stm32f4xx_ll_usb.c
├── Inc
│   ├── FreeRTOSConfig.h
│   ├── main.h
│   ├── stm32f4xx_hal_conf.h
│   ├── stm32f4xx_it.h
│   ├── usbd_cdc_if.h
│   ├── usbd_conf.h
│   ├── usbd_desc.h
│   └── usb_device.h
├── Middlewares
│   ├── ST
│   │   └── STM32_USB_Device_Library
│   │       ├── Class
│   │       │   └── CDC
│   │       │       ├── Inc
│   │       │       │   └── usbd_cdc.h
│   │       │       └── Src
│   │       │           └── usbd_cdc.c
│   │       └── Core
│   │           ├── Inc
│   │           │   ├── usbd_core.h
│   │           │   ├── usbd_ctlreq.h
│   │           │   ├── usbd_def.h
│   │           │   └── usbd_ioreq.h
│   │           └── Src
│   │               ├── usbd_core.c
│   │               ├── usbd_ctlreq.c
│   │               └── usbd_ioreq.c
│   └── Third_Party
│       └── FreeRTOS
│           └── Source
│               ├── CMSIS_RTOS
│               │   ├── cmsis_os.c
│               │   └── cmsis_os.h
│               ├── croutine.c
│               ├── event_groups.c
│               ├── include
│               │   ├── atomic.h
│               │   ├── croutine.h
│               │   ├── deprecated_definitions.h
│               │   ├── event_groups.h
│               │   ├── FreeRTOS.h
│               │   ├── list.h
│               │   ├── message_buffer.h
│               │   ├── mpu_prototypes.h
│               │   ├── mpu_wrappers.h
│               │   ├── portable.h
│               │   ├── projdefs.h
│               │   ├── queue.h
│               │   ├── semphr.h
│               │   ├── StackMacros.h
│               │   ├── stack_macros.h
│               │   ├── stream_buffer.h
│               │   ├── task.h
│               │   └── timers.h
│               ├── list.c
│               ├── portable
│               │   ├── GCC
│               │   │   └── ARM_CM4F
│               │   │       ├── port.c
│               │   │       └── portmacro.h
│               │   └── MemMang
│               │       └── heap_4.c
│               ├── queue.c
│               ├── stream_buffer.c
│               ├── tasks.c
│               └── timers.c
├── Src
│   ├── freertos.c
│   ├── main.c
│   ├── stm32f4xx_hal_msp.c
│   ├── stm32f4xx_hal_timebase_tim.c
│   ├── stm32f4xx_it.c
│   ├── syscalls.c
│   ├── system_stm32f4xx.c
│   ├── usbd_cdc_if.c
│   ├── usbd_conf.c
│   ├── usbd_desc.c
│   └── usb_device.c
├── startup
│   └── startup_stm32f405xx.s
└── STM32F405RGTx_FLASH.ld
```


