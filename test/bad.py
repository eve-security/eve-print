#! /usr/bin/env python

import serial
import time

ser = serial.Serial('/dev/ttyACM0', 9600)
time.sleep(2)
ser.write('b')
time.sleep(1)
ser.close()
