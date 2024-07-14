ReadMe.txt for the ST STM32F407 start project.

This project was built for SEGGER Embedded Studio V6.34a.

Supported hardware:
===================
The sample project for the STM32F407 is prepared to run on
an Olimex STM32-P407 board. Using different target hardware
may require modifications.

Configurations:
===============
- Debug:
  This configuration is prepared for download into internal
  Flash using J-Link. An embOS debug and profiling library
  is used.
  To use SEGGER SystemView with this configuration, configure
  SystemViewer for STM32F407ZG as target device and SWD at
  2000 kHz as target interface.

- Release:
  This configuration is prepared for download into internal
  Flash using J-Link. An embOS release library is used.
