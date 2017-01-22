ARDUINO_DIR   = /home/bart/Applications/arduino-1.6.9
ARDMK_DIR     = /home/bart/git/Arduino-Makefile
AVR_TOOLS_DIR = /home/bart/Applications/arduino-1.6.9/hardware/tools/avr

BOARD_TAG     = pro
MCU           = atmega328p
F_CPU         = 8000000L
AVRDUDE_ARD_BAUDRATE = 57600

ARDUINO_LIBS  = qpn_avr TM1637-1.1.0 EEPROM Arduino-CmdMessenger

include /home/bart/git/Arduino-Makefile/Arduino.mk
