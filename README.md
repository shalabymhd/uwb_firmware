# UWB Module Firmware
A nice tutorial created by Mohammed Shalaby can be found [here](./doc/stm32_tutorial.md).

## Dependencies

    pip3 install platformio

Also, follow this https://docs.platformio.org/en/latest//faq.html#platformio-udev-rules

Our chip can be found here: https://docs.platformio.org/en/latest/boards/ststm32/genericSTM32F405RG.html 

## Generating PlatformIO-compatible code from STM32CubeMX
Technically, this only needs to be done once. The chip configuration is done through CubeMX, where the project file and relevant information is saved as a `.ioc` file.
A 3rd-party python program called `stm32pio` takes this file, and generates the code such that it is also compatible with platformIO. 
Hence, the first thing you need to do is perform the proper chip configuration in CubeMX, and save the project. Put the `.ioc` file at the top of the project directory.

Install the 3rd-party python package which takes care of the conversion.

    pip3 install stm32pio

Then, while inside this project folder, this project was initialized with 

    stm32pio new -b genericSTM32F405RG.




