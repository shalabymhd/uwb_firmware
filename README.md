# UWB Module Firmware
A nice tutorial on getting started with STM32 and Eclipsed, created by Mohammed Shalaby, can be found [here](./doc/stm32_tutorial.md).

Historically, and as also covered in that tutorial, development on an STM32 chip consists of first configuring the chip according to your specific PCB in CubeMX, followed by setting up a configuration within the Eclipse IDE to build and debug the project. However, it turns out that Eclipse is simply just generating a `makefile` and running a `make` command to build the project. To upload and debug, the fundamental tool involved is actually `openocd`, and GDB is the debugger.

In this branch, we will be using all those tools directly. That is, we will build, upload, and debug the code without involving any editor, doing it all through the terminal. Then, we can use any editor we want to view and edit the code, as well as getting it to run the terminal commands for us. We will still be using CubeMX to generate the HAL code, and as it turns out, the makefile as well! 

The benefits of this editor-independent approach consist of a much more fundamental understanding of what is happening, as well as the ability for each developer to use whatever IDE they want on the same code base. 

If you would like to start from the absolute beginning, switch to the `blank` branch, which contains nothing other than the `config_stm32f4.ioc` file as well as this README.

## Generating the starter code with CubeMX
Assuming you are on the `blank` branch, you will have only the following in your directory

```
uwb_firmware
├── config_stm32f4.ioc
├── README.md
```

1. Open the CubeMX software.
2. Use __File > Load Project...__ to load the `config_stm32f4.ioc` file.
3. Under the __Project Manager__ tab, in the __Project__ section, you should see a field called `Toolchain / IDE`.  Here, you can choose __Makefile__!
4. Leave everything else as-is, and click on __GENERATE CODE__.

This should populate the current directory with the following files. 

```
uwb_firmware
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
├── Makefile
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
├── README.md
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
├── startup_stm32f405xx.s
└── STM32F405RGTx_FLASH.ld
```
Notice the presence of a `Makefile`. Feel free to open this file and check it out. 


## Compiling the code
In the project directory,

    make all

and thats it. This will create a `./build` directory, with a bunch of stuff, including a `.elf` file, which is the compiled firmware that we will be uploading to our board. 

Steven suggests the following very basic tutorial on using `make`: https://cs.colby.edu/maxwell/courses/tutorials/maketutor/.

## Uploading with OpenOCD
Although OpenOCD can be downloaded explicitly, it is also possible to install it as a regular package

    sudo apt-get install openocd

This will allow take care of creating a symbolic link to the `openocd` command, allowing us to just use the `openocd` command from any directory. Assuming you have built the code with `make`, that you are still in the `uwb_firmware` directory, and that the discovery board is plugged in with the appropriate jumpers removed, we can upload our firmware in one command:

    openocd -f board/stm32f4discovery.cfg -c "program ./build/config_stm32f4.elf verify reset exit"

## Debugging with OpenOCD and GDB

First, make sure the board is connected by USB and start OpenOCD

    openocd -f board/stm32f4discovery.cfg

In a new terminal in the current `uwb_firmware` directory

     arm-none-eabi-gdb ./build/config_stm32f4discovery.elf

and you will enter a GDB command line. The above step assumes that you have installed the `arm-none-eabi-gcc` toolchain as per Mohammed's tutorial. To connect to the openocd server 

    (gdb) target remote localhost:3333

Then at this point you can use whatever GDB commands. You can directly load the firmware from here

    (gdb) load
    
You can then use `list` to see where you are in the code, as well as `continue` or `step`. Theres a way to set breakpoints from the GDB terminal, but at this point, we will move to using the VSCode editor for debugging.

## Setting up the same workflow in VS Code

At a minimum, you just need to open the `uwb_firmware` folder in VS Code and you can start editting the source code with some basic syntax highlighting already. However, it will probably be full of red warnings as a result of Intellisense not finding all the files. We will need to configure intellisense properly.

### Configuring Intellisense
 Create a `./.vscode/` subdirectory. Create a `c_cpp_properties.json` file and insert the following

```json
{
	"version": 4,
	"configurations":
	[
		{	
			"name": "Linux: Embedded Development",
			"intelliSenseMode": "gcc-arm",
			"cStandard": "c99",
			"cppStandard": "c++17",
			"compilerPath": "/opt/gcc-arm-none-eabi-9-2020-q2-update/bin/arm-none-eabi-gcc"
			,
			"defines":
			[
				"USE_HAL_DRIVER", 
				"STM32F405xx"
			],
			"includePath":
			[
				"${workspaceFolder}/**",
				"/opt/gcc-arm-none-eabi-9-2020-q2-update/lib/gcc/arm-none-eabi/9.3.1/include",
				"Inc", 
				"Drivers/STM32F4xx_HAL_Driver/Inc", 
				"Drivers/STM32F4xx_HAL_Driver/Inc/Legacy", 
				"Middlewares/Third_Party/FreeRTOS/Source/include", 
				"Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS", 
				"Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F", 
				"Middlewares/ST/STM32_USB_Device_Library/Core/Inc", 
				"Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc", 
				"Drivers/CMSIS/Device/ST/STM32F4xx/Include", 
				"Drivers/CMSIS/Include"
			]
		}
	]
}
```
Now you should have the full amazing code navigation/editting functionality of VS Code, including syntax highlighting, Go to Definition, Go to References, code peeking and more. 

### Building from VS Code
VS Code provides functionality to run whatever terminal command as a "task" from within the editor. In the `./.vscode/` folder, create a `tasks.json` file with the following

```json
{
	"version": "2.0.0",
	"tasks":
	[
		{
			"label": "Build Firmware",
			"group":
			{
				"kind": "build",
				"isDefault": true
			},
			"type": "shell",
			"command": "make all",
			"args":
			[
				
			],
			"problemMatcher":
			[
				"$gcc"
			],
			"presentation":
			{
				"focus": true
			}
		}
	]
}

```
The line `"isDefault": true` sets this task is the default build task. This means that all we need to do is press `CTRL + SHIFT + B` to run the same make command as before.

Alternatively, press `CTRL + SHIFT + P` to open the command palette, and select __Run Task__, it will then ask you which one to choose. 

### Uploading from VS Code
Just as before, we just need to create a VS Code task to run the upload command for us. Open `tasks.json` and add the following task just after the build task (seperated by a comma)

```json
{
    "label": "Upload Firmware",
    "type": "shell",
    "command": "openocd",
    "args":
    [
        "-f","board/stm32f4discovery.cfg",
        "-c","'program build/config_stm32f4.elf verify reset exit'"
        
    ]
}

```
And thats it! You can run this from the command palette by pressing `CTRL + SHIFT + P` and typing in __Run Task__, after which the suggestions will prompt you for which task to choose.

## Debugging with VS Code
For this step, the easiest thing to do is to install the [Cortex-Debug](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug) extension for VS Code. 

Then, create a `launch.json` file inside the `.vscode` folder with the following contents.

```json
{
	"version": "0.2.0",
	"configurations":
	[
		{
			"name": "Build and Debug",
			"type": "cortex-debug",
			"request": "launch",
			"servertype": "openocd",
			"cwd": "${workspaceFolder}",
			"executable": "build/config_stm32f4.elf",
			"configFiles":
			[
				"/usr/share/openocd/scripts/board/stm32f4discovery.cfg"
			],
			"preLaunchTask": "Build Firmware"
		}
	]
}
```

Create a `settings.json` file inside the `.vscode` folder with the following contents.

```json
{
	"cortex-debug.armToolchainPath": "",
	"cortex-debug.openocdPath": "/usr/bin/openocd"
}
```


You should now be able to go to the debug tab in VS Code and see a `Build and Debug` option.


## Steven's Magic Links

Makefile tutorial: https://cs.colby.edu/maxwell/courses/tutorials/maketutor/

FreeRTOS tutorial: https://freertos.org/fr-content-src/uploads/2018/07/161204_Mastering_the_FreeRTOS_Real_Time_Kernel-A_Hands-On_Tutorial_Guide.pdf 
