# KIV-RTOS
An educational operating system for bare-metal Raspberry Pi Zero W (BCM2835-based board).

This software is developed as a demonstration project for operating systems course on University of West Bohemia, Department of Computer Science and Engineering (KIV/OS).

### Features
- CMake-based buildsystem
- written in C/C++ and ARM assembly
- bootloader with support of image transfer via UART (to avoid "SD-card dances")
- basic kernel memory management
- support for tasks
- basic time-slicing scheduler
- drivers
-- GPIO
-- UART
-- AUX
-- timer
- C++ development support

### Planned features
- time slicing process scheduler (EDF)
- spinlocks
- semaphores, mutexes, condition variables
- eMMC driver
- FAT32 support
- WiFi driver
- I2C OLED display driver
- named pipes

## License

This software is distributed under MIT license. See attached `LICENSE` file for details.
