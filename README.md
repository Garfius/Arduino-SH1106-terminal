# Arduino SH1106 terminal

Minimal scheme to use any AVR microarchitecture Ardiuno-compatible microcontroller as serial terminal.

# Caracteristics:

Minimal chip is Atmega328P due program space limitations.

Serial speed is set to 57600 for 8mhz compatibility.

Buffer size is screen size.

Limited character encoding, poor microcontroller :(

# Optional:

-M5Stack keyboard input setting via M5StackKeyboardIncluded compiler directive.

-Paging and EEPROM auto page saving setting via boolean class initialization parameter.

# TO-DO

Package as a library and publish to Arduino IDE GUI.

Complete english code translation.
