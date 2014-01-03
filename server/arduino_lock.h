#ifndef __ARDUINO_LOCK_H
#define __ARDUINO_LOCK_H

#include <stdio.h>
#include <unistd.h> //for usleep
#include "arduino-serial-lib/arduino-serial-lib.h"

#define BAUDRATE 9600

void arduino_connect(char* port);
void arduino_close();

void arduino_unlock();;
void arduino_blink();

#endif
