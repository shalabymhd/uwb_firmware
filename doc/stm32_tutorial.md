# Hardware
* **STM32F3discovery**: 
    * [User Manual](https://www.st.com/resource/en/user_manual/dm00063382-discovery-kit-with-stm32f303vc-mcu-stmicroelectronics.pdf).
    * Consists of
        1) ST-LINK (for programming and debugging MCUs).
        2) STM32F303VCT6 MCU. 
* **Custom-made UWB board**:
    * Consists of 
        1) DW1000 UWB module.
        2) STM32F405RG MCU.
        3) MPU-9250 9-DOF IMU.

# Setting-Up the Toolchain

This is based on Carmine Noviello's **Mastering STM32** book. Most of the software discussed in that book is now outdated, so the purpose of this document is to provide an updated version. Mainly, this document targets Ubuntu 20.04 users, and all the other software is as per the most recent compatible versions on April 9th 2021.

Before we can start developing applications for the STM32 platform, we need a complete toolchain. A toolchain is a set of programs, compilers and tools that allows us:
* to develop and navigate the source files of our application,
* to compile the source code using a cross-platform compiler, and
* to upload and debug our application on the target development board.

To accomplish these activities, we essentially need:
* an IDE with integrated source editor and navigator, 
* a cross-platform compiler able to compile source code for the ARM Cortex-M platform on our Linux machine,
* a debugger that allows us to execute step-by-step debugging of firmware on the target board, and
* a tool that allows interaction with the integrated hardware debugger in our Discovery board (the ST-LINK interface).

This documentation assumes that the user will be using Eclipse as the IDE of choice. One interesting feature of Eclipse is that it is not required to be installed in a specific path
on the Hard Disk. This allows the user to decide where to put the whole toolchain.
As per the book, we will assume that the whole toolchain is installed inside the
`∼/STM32Toolchain` folder on the Hard Disk (that is, a STM32Toolchain directory inside your
Home folder). You are free to place it elsewhere, but rearrange paths in the instructions
accordingly.

## Install i386 Run-Time Libraries on a 64-bit Ubuntu

If your Ubuntu is a 64-bit release, then you need to install some compatibility libraries that allow to run 32-bit applications. To do so, simply run the following commands in the terminal.

```
$ sudo dpkg --add-architecture i386
$ sudo apt-get update
$ sudo apt-get install libc6:i386 libncurses5 libncurses5:i386 libstdc++6:i386
```

## Java Installation

A recent version of Java is required (the book recommends Java 8 or newer, on Ubuntu 20.04, I tested using Java 11). Check the installed version of Java using 

```
$ java -version
```

If you need to install Java, Version 11.0.10 can be installed using

```
$ sudo apt install openjdk-11-jre-headless
```

## Eclipse Installation

The next step is to install the Eclipse version for C/C++ developers. The list of Eclipse Releases can be found [here](https://www.eclipse.org/downloads/packages/release). We are interested in the Eclipse version found in `YYYY-MM > R Packages > Eclipse IDE for C/C++ Developers > Linux x86_64`. The tested version is the 2020-12 version, which can be installed [here](https://www.eclipse.org/downloads/download.php?file=/technology/epp/downloads/release/2020-12/R/eclipse-cpp-2020-12-R-linux-gtk-x86_64.tar.gz&mirror_id=1135). You can leave a small donation if you'd like. 

In the terminal, navigate to where the tar file was downloaded, and run the following command to extract the contents of the compressed file to the toolchain directory. If a different release is downloaded than than the 2020-12 version, make sure to adjust the name of the file accordingly.

```
$ tar -xf eclipse-cpp-2020-12-R-linux-gtk-x86_64.tar.gz -C ~/STM32Toolchain/
```

At the end of the process you will find the folder `∼/STM32Toolchain/eclipse`
containing the whole IDE. Now we can execute for the first time the Eclipse IDE. Go inside the `∼/STM32Toolchain/eclipse` folder and run the eclipse file. After a while, Eclipse will ask you for the preferred folder where all Eclipse projects are stored (this is called the *workspace*).
You are free to choose the folder you prefer, or leave the suggested one. As with the book, I will assume that the Eclipse workspace is located inside the `∼/STM32Toolchain/projects` folder. Arrange the instructions accordingly if you choose another location. Once this is sorted out, launch Eclipse.

## Eclipse Plug-Ins Installation

Once Eclipse is started, we can proceed to install some relevant plug-ins. The first plug-in we need to install is the *C/C++ Development Tools SDK*, also known as *Eclipse CDT*. CDT provides a fully functional C and C++ Integrated Development Environment for the Eclipse platform. 

In Eclipse, go to `Help > Install new software...`, and from the *work with* drop-down menu, choose the **CDT** repository. Check the **CDT Main Features** box, and under *CDT Optional Features*, check the **C/C++ GDB Hardware Debugging Developer Resources** box. Click **Next** and follow the instructions to install the plug-ins. Restart Eclipse when requested.

Now we have to install the [GNU MCU plug-ins for Eclipse](https://eclipse-embed-cdt.github.io/). In Eclipse, go to `Help > Install new software...`, then click the **Add...** button and fill the fields in the following way:

* **Name**: GNU MCU Eclipse Plug-ins
* **Location**: http://gnu-mcu-eclipse.netlify.com/v4-neon-updates

Click on the **Add** button. Now, make sure that **GNU MCU Eclipse Plug-ins** is chosen in the *work with* drop-down menu, and under **GNU ARM & RISC-V C/C++ Cross Development Tools**, check the following packages:
* GNU MCU C/C++ ARM Cross Compiler,
* GNU MCU C/C++ Documentation (Placeholder),
* GNU MCU C/C++ Generic Cortex-M Project Template,
* GNU MCU C/C++ J-Link Debugging,
* GNU MCU C/C++ OpenOCD Debugging,
* GNU MCU C/C++ Packs (Experimental),
* GNU MCU C/C++ PyOCD Debugging,
* GNU MCU C/C++ STM32Fx Project Templates.

As before, click **Next**, follow the steps, and allow Eclipse to restart when prompted. You might get a "security" warning as this is an unlicensed package developed by a hobbyist in Bucharest named Liviu Ionescu, but you can ignore this warning.

## GCC ARM Embedded Installation

The next step is installing the GCC suite for ARM Cortex-M and Corex-R microcontrollers. This is a set of tools (macro preprocessor, compiler, assembler, linker and debugger) designed to cross-compile the code we will create for the STM32 platform. This is a very important step, as it allows generating machine code for Cortex-M processors even though we are compiling on a linux-based x86 machine.

The GCC suite's releases can be found [here](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads), and for Ubuntu 20.04, we need a package with the description *Linux x86_64 Tarball*. The version tested is the 2020-q4 version, which can be installed [here](https://developer.arm.com/-/media/Files/downloads/gnu-rm/10-2020q4/gcc-arm-none-eabi-10-2020-q4-major-x86_64-linux.tar.bz2?revision=ca0cbf9c-9de2-491c-ac48-898b5bbc0443&la=en&hash=68760A8AE66026BCF99F05AC017A6A50C6FD832A).

In the terminal, navigate to where the tar file was downloaded, and run the following command to extract the contents of the compressed file to the toolchain directory. If a different release is downloaded than than the 2020-q4 version, make sure to adjust the name of the file accordingly.

```
$ tar -xf gcc-arm-none-eabi-10-2020-q4-major-x86_64-linux.tar.bz2 -C ~/STM32Toolchain/
```

The extracted folder, by default, is named `gcc-arm-none-eabi-10-2020-q4-major`. This is not
convenient, because when GCC is updated to a newer version we need to change the settings
for each Eclipse project. So, rename it to simply `gcc-arm`.

## Updating ST-LINK Firmware 

This step is important to ensure that the firmware on the ST-LINK programmer/debugger is up-to-date. First, install `libusb-1.0` on your computer using

```
$ sudo apt-get install libusb-1.0
```

Download the latest ST-Link driver from the ST website [here](https://www.st.com/en/development-tools/stsw-link007.html). You will have to create an account. The most recent version available at the time was V2.37.27.

In the terminal, navigate to where the zip file was downloaded, and run the following command to extract the contents of the compressed file to the toolchain directory. If a different release is downloaded than V2.37.27, make sure to adjust the name of the file accordingly.

```
$ unzip en.stsw-link007_V2-37-27.zip -d ~/STM32Toolchain/
```

Connect the ST-LINK board through USB to your computer, and run the following commands to navigate to and open the firmware upgrader.

```
$ cd ~/STM32Toolchain/stsw-link007/AllPlatforms 
$ sudo java -jar STLinkUpgrade.jar
```

You should be able to see your connected ST-LINK. If not, click on **Refresh device list**. Click on **Open in update mode**, and then **Upgrade**. You should see LD2 blink repeatedly in red on the board. Once completed, close the upgrader and unplug your board.

## OpenOCD Installation

[OpenOCD](http://openocd.org/) is a tool that allows uploading the firmware to the board and doing the step-by-step debugging. A new version (version 0.11.0) has been released in March 2021; however, this is yet to be tested in this framework. We will rather use version 0.10.0-5 released in 2017, which can be downloaded for Ubuntu 20.04 [here](https://github.com/ilg-archived/openocd/releases/download/v0.10.0-5-20171110/gnu-mcu-eclipse-openocd-0.10.0-5-20171110-1117-debian64.tgz).

In the terminal, navigate to where the tar file was downloaded, and run the following commands to extract the contents of the compressed file to the toolchain directory and do some clean-up. If a different release is downloaded than than version 0.10.0-5, make sure to adjust the name of the file accordingly.

```
$ tar -xf gnu-mcu-eclipse-openocd-0.10.0-5-20171110-1117-debian64.tgz -C ~/STM32Toolchain/
$ cd ~/STM32Toolchain/
$ mv gnu-mcu-eclipse/openocd/0.10.0-5-20171110-1117/ .
$ rm -rf gnu-mcu-eclipse/
$ mv 0.10.0-5-20171110-1117/ openocd
```

By default, Linux does not not allow unprivileged users to access an USB device using `libusb`. We therefore need to configure the Universal DEVice manager (UDEV) to grant root privileges to the ST-LINK interface. To do so, let us create a file named `stlink.rules` inside the `/etc/udev/rules.d` directory and add a line inside it:

```
$ sudo cp ~/STM32Toolchain/openocd/contrib/60-openocd.rules /etc/udev/rules.d/
$ sudo udevadm control --reload-rules
```

### Testing the Board Using OpenOCD

In the terminal, navigate to the scripts folder in OpenOCD using

```
$ cd ~/STM32Toolchain/openocd/scripts/
```

Assuming you are using the F3 Discovery board as the programmer/debugger, ensure that a jumper is placed at the JP3 port and 2 jumpers occupy both ports at CN4 (where it says ST-LINK DISCOVERY). This ensures that the ST-LINK is connected with the STM32F303 chip on the board. When we are programming/debugging the custom UWB boards, the 2 jumpers at CN4 are removed to disconnect the chip onboard.

Connect the F3 Discovery board using USB, and then run the following command.

```
$ ../bin/openocd -f board/stm32f3discovery.cfg
```

You should see something like this:

```
GNU MCU Eclipse 64-bits Open On-Chip Debugger 0.10.0+dev-00254-g2ec04e4e (2017-11-10-11:27)
Licensed under GNU GPL v2
For bug reports, read
	http://openocd.org/doc/doxygen/bugs.html
adapter speed: 1000 kHz
adapter_nsrst_delay: 100
Info : The selected transport took over low-level target control. The results might differ compared to plain JTAG/SWD
none separate
srst_only separate srst_nogate srst_open_drain connect_deassert_srst
Info : Listening on port 6666 for tcl connections
Info : Listening on port 4444 for telnet connections
Info : Unable to match requested speed 1000 kHz, using 950 kHz
Info : Unable to match requested speed 1000 kHz, using 950 kHz
Info : clock speed 950 kHz
Info : STLINK v2 JTAG v37 API v2 SWIM v0 VID 0x0483 PID 0x3748
Info : using stlink api v2
Info : Target voltage: 2.903546
Info : stm32f3x.cpu: hardware has 6 breakpoints, 4 watchpoints
Info : Listening on port 3333 for gdb connections
```

The LD2 light on the board should be blinking red and green, which means that communication has been established with your computer. This means that everything looks good, you can now click **Ctrl + C** and disconnect your board.

## ST Tools Installation

### STM32CubeMX

STM32CubeMX is a graphical tool used to generate setup files in C programming language for
an STM32 MCU, according the hardware configuration of our board. Download the most recent Linux version of STM32CubeMX [here](https://www.st.com/en/development-tools/stm32cubemx.html#get-software). At the time of writing, this would be version 6.2.1.

In the terminal, navigate to where the zip file was downloaded, and run the following commands to extract the contents of the compressed file and run the installer. If a different release is downloaded than version 6.2.1, make sure to adjust the name of the file accordingly.

```
$ unzip en.stm32cubemx-lin_v6-2-1.zip
$ ./SetupSTM32CubeMX-6.2.1
```

Follow the steps in the installation wizard, and when prompted for an installation path, choose `~/STM32Toolchain/STM32CubeMX`. When installed, STM32CubeMX can be launched by double clicking the STM32CubeMX icon inside that folder.

### STM32CubeProgrammer

Another useful tool is the *STM32CubeProgrammer*, which allows uploading firmware on the MCU using the dedicated ST-LINK programmer. The most recent Linux version can be downloaded [here](https://www.st.com/content/st_com/en/products/development-tools/software-development-tools/stm32-software-development-tools/stm32-programmers/stm32cubeprog.html). The most recent version at the time of writing was version 2.7.0.

In the terminal, navigate to where the zip file was downloaded, and run the following commands to extract the contents of the compressed file and run the installer. If a different release is downloaded than version 2.7.0, make sure to adjust the name of the file accordingly.

```
$ unzip en.stm32cubeprg-lin_v2-7-0.zip
$ ./SetupSTM32CubeProgrammer-2.7.0.linux
```

Follow the steps in the installation wizard, and when prompted fo ran installation path, choose `~/STM32Toolchain/STM32CubeProgrammer`. STM32CubeProgrammer is installed as a Desktop application and can be launched as any other standard application.


# Getting Familiar with the Toolchain

## Using the GNU MCU Eclipse plug-in to blink the LEDs on the Discovery board

In this section, the goal is to get a blinking LED on the F3 discovery board using the GNU MCU Eclipse plug-in, which provides an easy alternative to programming without the need to dig into the low-level complexities we will have to deal with in the future. This is, therefore, just an introdcutory tutorial to get familiar with the development environment. In addition, we will, focus on the F3 discovery board, and leave the UWB boards aside for now.

In Eclipse, go to `File > New > C/C++ Project`. Choose **C Managed Build**, and click on **Next**. Choose a project name (e.g., `stm32f3discovery-blink`), and for *Project type*, given that we are using the F3 discovery board, choose **STM32F3xx C/C++ Project** under *Executable*. Under *Toolchains*, **ARM Cross GCC** should be chosen by default. Click **Next**. Change the *Trace output* to **None (no trace output)**, and make sure everything else is as follows before pressing **Next**:

* **Chip family**: STM32F30x/31x
* **Flash size (kB)**: 256
* **RAM size (kB)**: 40
* **CCM RAM size (kB)**: 8
* **External clock (Hz)**: 8000000
* **Content**: Blinky (blink a led)
* **Use system calls**: Freestanding (no POSIX system calls)
* **Trace output**: None (no trace output)
* **Check some warnings**: checked
* **Check most warnings**: unchecked
* **Enable-Werror**: unchecked
* **Use -Og on debug**: checked
* **Use newlib nano**: checked
* **Exclude unused**: checked
* **Use link optimizations**: unchecked

Leave everything as is in the next two windows, and when you have to choose the *GCC toolchain path*, fill the two fields as follows:

* **Toolchain name**: GNU Tools for ARM Embedded Processors (arm-none-eabi-gcc)
* **Toolchain path**: ~/STM32Toolchain/gcc-arm/bin

Click **finish**, and close the Welcome screen if you still have it open.

The GNU MCU plug-in generated some folders and files, and we will first take a look at these files.

* **Includes**: this folder shows all folders that are part of the GCC Include Folders.
* **src**: this Eclipse folder contains the source files that make up our application. One of these files is `main.c`, which contains the `int main(int argc, char* argv[])` routine.
* **system**: this Eclipse folder contains header and source files of many relevant libraries (e.g., the *ST HAL* and the *CMSIS* package).
* **include**: this folder contains the header files of our main application.
* **ldscripts**: this folder contains some relevant files that make our application work on the MCU.

There are 8 LEDs that can be programmed by the user. From Section 6.4 in the [User Manual](https://www.st.com/resource/en/user_manual/dm00063382-discovery-kit-with-stm32f303vc-mcu-stmicroelectronics.pdf), we can see the following LED-pin connection pairs.

* **LD3 (red)**: PIN9 of the GPIOE port (PE9).
* **LD4 (blue)**: PIN8 of the GPIOE port (PE8).
* **LD5 (orange)**: PIN10 of the GPIOE port (PE10).
* **LD6 (green)**: PIN15 of the GPIOE port (PE15).
* **LD7 (green)**: PIN11 of the GPIOE port (PE11).
* **LD8 (orange)**: PIN14 of the GPIOE port (PE14).
* **LD9 (blue)**: PIN12 of the GPIOE port (PE12).
* **LD10 (red)**: PIN13 of the GPIOE port (PE13).

In the file `include > BlinkLed.h`, lines 41 and 42 specify the LED we want to blink, where the following decoding is used for the different ports:

```
Port numbers: 0=A, 1=B, 2=C, 3=D, 4=E, 5=F, 6=G, ...
```

Therefore, for LD8 for example, which is connected to PE14, lines 41 and 42 should read

```C
#define BLINK_PORT_NUMBER               (4)
#define BLINK_PIN_NUMBER                (14)
```

Save, and then click on `Project > Build Project`. You should see in the console that your project has successfully built. Connect the F3 discovery board to your computer through USB. Make sure to use the ST-LINK connection, and not the USER connection. 

Open STM32CubeProgrammer that you previously installed on your computer, which is a tool to upload firmware on the MCU. You should see on the right-hand side the serial number of your board, which indicates that the board has been identified, after which you click **Connect**. Your console should say *Data read successfully*.

Click on the **Erase & programming** icon (the second green icon on the left). Then, click on the **Browse** button in the *File programming* section and select the file `∼/STM32Toolchain/projects/stm32f3discovery-blink/Debug/stm32f3discovery-blink.hex`. Check the **Verify programming** and **Run after programming** flags and click on **Start Programming** button to start flashing. At the end of the flashing procedure your orange LD8 LED should start blinking. Congratulations on achieving the first basic step of the STM32 world! You can now right click on the project in the *Project Explorer* in Eclipse, and click on **Close Project**.

## Using the STM32CubeMX Tool to blink the LEDs on the UWB board

The GNU MCU plug-in allowed us to blink the LEDs of the F3 discovery board without having to dig into more complex topics such as the *Hardware Abstraction Layer* (HAL). However, the applications of this are very limited, and in order to develop more advanced applications, we will now address how to blink the LED on the UWB board by generating code using the STM32CubeMX tool and modifying the code by using the HAL API available on the STM MCUs.

First of all, we need to connect the UWB board to the F3 discovery board, and disconnect the ST-LINK and the MCU onboard the F3 discovery board. Using the programming cable, connect one side to the J2 port (or PROG) on the UWB board, and the other side to the CN3 port (or SWD). Make sure that the black cable is the one near the top-left corner of the discovery board. To disconnect the onboard MCU and ST-LINK, remove the two jumpers at the CN4 port.

Launch the previously installed STM32CubeMX tool. Go to `File > New Project...`, and under *MCU/MPU selector*, search and choose the **STM32F405RG** chip, then click on **Start Project**. You should now see a graphical representation of the MCU chip and its different pins. There is a lot going here, and even though the necessary basics are covered here and over the next few sections, more information can be found at the [official ST documentation for the STM32CubeMX tool](https://www.st.com/resource/en/user_manual/dm00104712-stm32cubemx-for-stm32-configuration-and-initialization-c-code-generation-stmicroelectronics.pdf).

What you see currently is the *Pinout view*, and it is divided in two parts. The right side contains the MCU representation with the selected peripherals and GPIOs, and it is called the *Chip view*. On the left side we have the list, in form of a tree view, of all peripherals (hardware parts) and middleware libraries (software parts) that can be used with the selected MCU. This is called the *IP tree pane*.

The Chip view allows to easily navigate inside the MCU configuration, and it is a really convenient
way to configure the microcontroller. Pins colored in bright green are enabled. This means that CubeMX will generate the needed code to configure that pin according to its functionalities. A pin is colored in orange when the corresponding peripheral is not enabled, but will be initialized. Yellow pins are power source pins, and their configuration cannot be changed. BOOT and RESET pins are colored in khaki, and their configuration cannot be changed. When you move the pointer over a pin, information about the pin and how it is configured are displayed. Clicking on a pin shows the different options a pin can be configured as. For example, PD2 can be configured to receive UART #5 signals (`UART5_RX`), or as a general-purpose input (`GPIO_Input`).

To blink the LED on the UWB board's MCU, we need to figure out which pin is connected to the LED labelled D4 (or infos). From the schematic print of these boards, found in `doc/Schematic\ Prints.pdf` of the repository, we can see that the LED D4 is connected to pin **PC7** (PIN7 of port C). Therefore, on the ST Chip view, click on the pin PC7 and set it to `GPIO_Output`. This pin should become green.

On the *Project Manager* tab, set the project name to be `stm32f405rg-blink`, and the *project location* to be in a new folder `~/STM32Toolchain/cubemx-out`. Set the *Toolchain/IDE* field to be **SW4STM32**, and leave everything else as default. Once done, click on the **GENERATE CODE** button on the top-right side. If prompted to download additional packages, click **Yes**. Once complete, you should see C code generated inside the `~/STM32Toolchain/cubemx-out/stm32f405rg-blink` directory.

We are now going to create an Eclipse project that will host the files generated by CubeMX. In Eclipse, go to `File > New > C/C++ Project`, then choose **C Managed Build** and click on **Next**.  Choose the project name to be `stm32f405rg-blink`, and the *project type* to be **Hello World ARM Cortex-M C/C++ Project**. In the next window, fill the fields as follows.

* **Processor Core**: Cortex-M4
* **Clock (Hz)**: 8000000
* **Flash size (kB)**: 1024
* **RAM size (kB)**: 128
* **Use system calls**: Freestanding (no POSIX system calls)
* **Trace output**: None (no trace output)
* **Check some warnings**: checked
* **Check most warnings**: unchecked
* **Enable-Werror**: unchecked
* **Use -Og on debug**: checked
* **Use newlib nano**: checked
* **Use link optimizations**: unchecked

In the next window, leave everything as default, except that the last field *Vendor CMSIS name* must be changed from **DEVICE** to **stm32f4xx**. Click **Next**, leave the next window as is, and the last window must have the *Toolchain name* and *Toolchain path* specified as before:

* **Toolchain name**: GNU Tools for ARM Embedded Processors (arm-none-eabi-gcc)
* **Toolchain path**: ~/STM32Toolchain/gcc-arm/bin

Click on **Finish** to generate the code. Once again, we have used the GNU MCU Eclipse plug-in to generate the project, but this time there are some files we do not need, since we will use the ones generated by CubeMX tool. We will therefore delete the following files:

```
~/STM32Toolchain/projects/stm32f405rg-blink/src/main.c
~/STM32Toolchain/projects/stm32f405rg-blink/src/Timer.c
~/STM32Toolchain/projects/stm32f405rg-blink/system/src/cmsis/system_stm32f4xx.c
~/STM32Toolchain/projects/stm32f405rg-blink/system/src/cmsis/vectors_stm32f4xx.c
~/STM32Toolchain/projects/stm32f405rg-blink/include/Timer.h
```

Another thing we need to change is lines 43 and 51 in `ldscripts/mem.ld`. The flash memory is mapped from the address `0x08000000` for all STM32 devices, so line 43 must be changed from 

```
FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 1024K
```

to 

```
FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1024K  
```

Moreover, line 51 should be changed from

```
CCMRAM (xrw) : ORIGIN = 0x10000000, LENGTH = 0
```

to

```
CCMRAM (xrw) : ORIGIN = 0x10000000, LENGTH = 64K
```

Everything else should be kept the same. 

We now need to import some of the CubeMX-generated code into the Eclipse project. In the following list, copy all the contents of the left-hand side directory into the right-hand side directory, with replacement if prompted to do so.

* `∼/STM32Toolchain/cubemx-out/stm32f405rg-blink/Core/Inc` ----> `∼/STM32Toolchain/projects/stm32f405rg-blink/include`
* `∼/STM32Toolchain/cubemx-out/stm32f405rg-blink/Core/Src` ----> `∼/STM32Toolchain/projects/stm32f405rg-blink/src`
* `∼/STM32Toolchain/cubemx-out/stm32f405rg-blink/Drivers/STM32F4xx_HAL_Driver/Inc` ----> `∼/STM32Toolchain/projects/stm32f405rg-blink/system/include/stm32f4xx`
* `∼/STM32Toolchain/cubemx-out/stm32f405rg-blink/Drivers/STM32F4xx_HAL_Driver/Src` ----> `∼/STM32Toolchain/projects/stm32f405rg-blink/system/src/stm32f4xx`
* `∼/STM32Toolchain/cubemx-out/stm32f405rg-blink/Drivers/CMSIS/Include` ----> `∼/STM32Toolchain/projects/stm32f405rg-blink/system/include/cmsis`
* `∼/STM32Toolchain/cubemx-out/stm32f405rg-blink/Drivers/CMSIS/Device/ST/STM32F4xx/Include` ----> `∼/STM32Toolchain/projects/stm32f405rg-blink/system/include/cmsis`

Lastly, copy the file `∼/STM32Toolchain/cubemx-out/startup/startup_stm32f4xxxx.s` inside the `∼/STM32Toolchain/projects/system/src/cmsis` directory, and rename the file to `startup_stm32f4xxxx.S` (capitalize the suffix). Right-click on the Project in the *Project Explorer* in Eclipse, and click on **Refresh**. You should see all the new files in their respective folders. Right-click on the project again and click on **Properties**, and navigate to `C/C++ Build > Settings`. Under *Tool Settings*, go to `GNU ARM Cross C Compiler > Preprocessor`, and add to the *Defined symbols* the value `STM32F405xx`. This lets the builder know which HAL files to look for. 

Another incompatibility issue using this framework is that a function `_sbrk` gets defined twice. In order to overcome this, open the source file `∼/STM32Toolchain/projects/stm32f405rg-blink/src/syscalls.c`. Comment out the definition of the `_sbrk` function, which should be on lines 117 to 138. 

You are now ready to try out building the project. Click on `Project > Build Project`, and you should have no errors (and no warnings, in fact). 

Now, we need to modify the source file `~/STM32Toolchain/projects/stm32f405rg-blink/main.c` such that we get the LED on the UWB board blinking. To do so, we need to use the HAL API. The [HAL user manual for STM32F4](https://www.st.com/resource/en/user_manual/dm00105879-description-of-stm32f4-hal-and-ll-drivers-stmicroelectronics.pdf) provides a list of all the HAL functionalities, and in this simple tutorial we are concerned only with 2 functions, `HAL_GPIO_TogglePin` and `HAL_Delay`. The former toggles the specified pins, and the latter introduces a delay or a "pause" while executing your script with the input being the length of the delay in milliseconds. 

When you open the main source file, `~/STM32Toolchain/projects/stm32f405rg-blink/main.c`, you will notice a few things, The general-purpose input/output pins are being initialized by defining and calling a function `MX_GPIO_Init`, which sets `GPIO_PIN_7` of GPIOC to be of mode `GPIO_MODE_OUTPUT_PP`. Additionally, you will see throughout the script that CubeMX comments regions to be "user code" areas, which are where you would input your code. The infinite loop (line 94) is what the MCU executes indefinitely, and is where you would input your code to perform some task. In this case, we want to toggle the pin with the LED repeatedly using the HAL API. Your code should look something like this:

```C
/* Infinite loop */
/* USER CODE BEGIN WHILE */
while (1)
{
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
    HAL_Delay(1000);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
}
```

You can now rebuild your project. Your ST-LINK board should be connected to the UWB board, and you can now connect the ST-LINK board to the computer using USB. Your UWB board should also be powered through the J1 port (the micro-USB port), but not necessarily through your computer. As before, use the STM32CubeProgrammer to flash the file `∼/STM32Toolchain/projects/stm32f405rg-blink/Debug/stm32f405rg-blink.hex` on your MCU, and you should see your LED light blink at 1 second intervals! Big yay!

# Introduction to Debugging

The previously installed Open On-Chip Debugger (OpenOCD) aims to provide debugging and in-system programming for embedded target devices. It does so with the assistance of a hardware debug adapter, which provides the right kind of electrical signaling to the target being debugged. In our case, this adapter is the ST-LINK debugger provided by the Discovery board. Every debug adapter uses a transport protocol that mediates between the hardware under debugging (i.e., the MCU) and the host software (i.e., OpenOCD).

## Trying out OpenOCD

Before we configure Eclipse to use OpenOCD in our project, test our OpenOCD by running the following commands in the terminal:

```
$ cd ~/STM32Toolchain/openocd/scripts
$ ../bin/openocd -f board/stm32f4discovery.cfg
```

If everything went the right way, you should see messages similar to 

```
GNU MCU Eclipse 64-bits Open On-Chip Debugger 0.10.0+dev-00254-g2ec04e4e (2017-11-10-11:27)
Licensed under GNU GPL v2
For bug reports, read
	http://openocd.org/doc/doxygen/bugs.html
Info : The selected transport took over low-level target control. The results might differ compared to plain JTAG/SWD
adapter speed: 2000 kHz
adapter_nsrst_delay: 100
none separate
srst_only separate srst_nogate srst_open_drain connect_deassert_srst
Info : Listening on port 6666 for tcl connections
Info : Listening on port 4444 for telnet connections
Info : Unable to match requested speed 2000 kHz, using 1800 kHz
Info : Unable to match requested speed 2000 kHz, using 1800 kHz
Info : clock speed 1800 kHz
Info : STLINK v2 JTAG v37 API v2 SWIM v0 VID 0x0483 PID 0x3748
Info : using stlink api v2
Info : Target voltage: 2.909220
Info : stm32f4x.cpu: hardware has 6 breakpoints, 4 watchpoints
Info : Listening on port 3333 for gdb connections
```

At the same time, the LED LD1 on the Discovery board should start blinking GREEN and RED alternatively. Notice that we use the F4 discovery configuration since our chip is from the F4 family, even though our debugger is from the F3 family. 

## Configuring and Debugging in Eclipse

Going back to Eclipse, first ensure that you have a project open. Then click on `Run > External Tools > External Tools Configurations...`. Highlight **Program** in the list on the left, and click on the **New launch configuration** icon on the top left. Now, fill the following fields in this way:

* **Name**: write the name you like for this configuration; it is suggested to use `OpenOCD_F4`.
* **Location**: choose the location of the OpenOCD executable `∼/STM32Toolchain/openocd/bin/openocd`.
* **Working directory**: choose the location of the OpenOCD scripts directory `∼/STM32Toolchain/openocd/scripts`.
* **Arguments**: write the command line arguments for OpenOCD, that is `-f board/stm32f4discovery.cfg`.

Click on **Apply**, then **Close**. You can test your configuration by clicking on the **Launch in 'Run' mode** button on the top left, and you should see in your Console window the same output as in the terminal when you run OpenOCD. Your ST-Link debugger should also be blinking Green and Red. Click on **Terminate**, which is the little red square above the Console tab.

Now we are ready to create a Debug Configuration to use GDB in conjunction with OpenOCD. **This operation must be repeated every time we create a new project**. Go to `Run->Debug Configurations...` menu. Highlight the **GDB OpenOCD Debugging** entry in the list view on the left and click on the **New launch configuration** icon on the top left. Eclipse fills automatically all the needed fields in the *Main* tab. However, if you are using a project with several build configurations, you need to click on the **Search Project** button and choose the ELF file for the active build configuration.

Next, go in the *Debugger* tab and uncheck the entry **Start OpenOCD locally**, since we have created the specific OpenOCD external tool configuration. Then go to the *Startup* tab and add `set remotetimeout 20` to the white box in the *Initialization Commands* section. Lastly, in the *Common* tab, choose the **Shared file** option, and the **Debug** option. Leave everything else as is, click on **Apply**, then **Close**.

Now the debugging environment has been configured, and we can try it out on our MCU. To start a new debug session, first launch the `OpenOCD_F4` run configuration we created, then click on the arrow near the *Debug* icon on the Eclipse toolbar (the little green bug) and choose the debug configuration `stm32f405rg-blink Debug` we created. You will be prompted to switch to the "Debug Perspective", where you should click on **Yes** (probably worth choosing the "Remember my decision" option too).

Look around the Debug Perspective to familiarize yourself with it. Note that there was an automatic breakpoint placed in `main.c` right before the execution of the `HAL_Init()` function. Let's place another breakpoint inside the while loop where we placed our blinking code. This can be done by right clicking on the narrow bar on the left-hand side of the script window, and clicking on **Toggle Breakpoint**. To resume, press F8 on your keyboard, and then resume again, and again, each time noticing how the LED on your board switches on and off. You are officially capable of toggling breakpoints and debugging on the UWB board's MCU using the ST-LINK debugger!

You can now terminate, and switch back to the "C/C++ Perspective" by clicking on the icon that has a "C inside a window" near the top right.

# Exchanging messages between the embedded firmware and the host computer 

## ARM Semihosting

ARM semihosting is a distinctive feature of the Cortex-M platform, and it is extremely useful for
testing and debugging purposes. It is a mechanism that allows target boards (e.g., the Discovery board) to “exchange messages” from the embedded firmware to a host computer running a debugger. This
mechanism enables some functions in the C library, such as `printf()` and `scanf()`, to use the screen
and keyboard of the host instead of having a screen and keyboard on the target system. This is useful
because development hardware often does not have all the input and output facilities of the final
system. Semihosting enables the host computer to provide these facilities. 

When generating the project we are currently working with, we were asked to choose a *Trace output*, for which we chose **None (no trace output)**. This means that we chose to turn off the semihosting feature. Usually, when creating a fully-debugged and tested project to be uploaded to your microcontroller for field implementation, you will have the semihosting feature turned off as it can be computationally demanding. However, in the development stage, having semihosting can facilitate debugging and testing. Therefore, when creating a project for development, you would choose your *Trace Output* option to be **Semihosting DEBUG channel**. Given that we did not do this, we need to enable semihosting manually.

In Eclipse, right-click on the project in the *Project Explorer*, and click on **Properties**. Go to `C/C++ Build > Settings`, and then `GNU ARM Cross C Compiler > Preprocessor`. In the *Defined Symbols* box, click on **Add...** on the top-right corner, and add a new global macro `OS_USE_TRACE_SEMIHOSTING_DEBUG`. Click on **Apply and Close**.

Now we have a project ready to use semihosting. The tracing routines are available inside the
`system/src/diag/Trace.c` file. They are:
* **trace_printf()**: it is the equivalent of C `printf()` function. It allows to format strings with a variable number of parameters, and it adopts the same string formatting convention of the C
programming languange.
* **trace_puts()**: writes a string to the debug console terminating it with a newline char `\n`
automatically.
* **trace_putchar()**: writes one char to the debug console.
* **trace_dump_args()**: it is a convenient routine that automatically does pretty printing of
command line arguments.

In the source file `main.c`, import under *Private includes* the `Trace.h` header file. This should look something like this:

```C
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
    #include "diag/Trace.h"
/* USER CODE END Includes */
```

Then, add a message to be output in the main function. Note that semihosting implementation in OpenOCD is designed so that every string must be terminated with the newline character `\n` before the string appears on the OpenOCD console. Your code should look something like this:

```C
/* USER CODE BEGIN 1 */
    char msg[] = "Meow.\n";
/* USER CODE END 1 */
```


```C
/* USER CODE BEGIN 2 */
    trace_printf(msg);
/* USER CODE END 2 */
```

As before, build the project, run the OpenOCD, then the Debugger. Resume from the initial breakpoint, and you should the message appearing in your Console. 

We can also use C built-in functions (like `printf()` and `scanf()`) rather than the ones provided in `Trace.c`. First of all, delete `∼/STM32Toolchain/projects/stm32f405rg-blink/src/syscalls.c`, which conflicts with other source files. Then, in Eclipse, right-click on the project in the *Project Explorer*, and click on **Properties**. Go to `C/C++ Build > Settings`, and then under *Optimization*, uncheck the box **Assume freestanding environment (-ffreestanding)**. Then go to `GNU ARM Cross C Compiler > Preprocessor`. In the *Defined Symbols* box, click on the previously defined `OS_USE_TRACE_SEMIHOSTING_DEBUG` macro, and on the top-right corner click on **Edit...**. Replace `OS_USE_TRACE_SEMIHOSTING_DEBUG` with `OS_USE_SEMIHOSTING`. Now, in `main.c`, you can remove the import of `Trace.h`, and replace `trace_printf(msg)` with `printf(msg)`. Go to `Project > Clean...` and click on **Clean**, then check that it works. 

You can even experiment with other C functions like `scanf()`. For example,

```C
/* USER CODE BEGIN 1 */
    char msg[20], name[20];
/* USER CODE END 1 */
```

```C
/* USER CODE BEGIN 2 */
    printf("What's your cat's name?: \r\n");
    scanf("%s", name);
    sprintf(msg, "Hello %s!\r\n", name);
    printf(msg);
/* USER CODE END 2 */
```

Semihosting has plenty of drawbacks. It has a significant impact on performance, and can oftentimes lead to your MCU stalling. A more robust approach using one of the STM32 USARTs is discussed next.

## USART (TODO)

### UART Configuration Using CubeMX (TODO)

### UART Communication in Polling Mode (TODO)

### UART Communication in Interrupt Mode (TODO)

### Input/Output Retargeting (TODO)


# Introduction to uwb_robots (TODO)

(structure, I2C, SPI, USB print, where to find further documentation for the board/scripts)

# Running uwb_robots (TODO)

## Pulling the code from GitLab (TODO)

### Some necessary modifications for compatibility with the newer toolchain (TODO)

## Testing your setup by blinking the LED (TODO)

## Testing your IMU (TODO)

## Ranging between two tags using TWR (TODO)

## Logging data using Python (TODO)