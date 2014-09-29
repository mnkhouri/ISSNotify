ISSNotify
=========

ISS Notification Lamp
An Arduino sketch to activate a light when the International Space Station is above the current loctation.

This is an Arduino sketch that communicates with an ENC28J60 ethernet interface to fetch the current date and time, the approximate location via IP address reverse lookup, and a list of future passes of the International Space Station above the current location. When the ISS is above, the Arduino activates an output connected to a lamp.

How to get started:
Download the project files

Connect your ENC28J60 module to the Arduino like this:
* ENC28J60  PATA Cable  Arduino
* VCC       wire 2        3.3v
* GND       wire 1        GND
* SCK       wire 5        Pin 13
* SO        wire 7        Pin 12
* SI        wire 6        Pin 11
* CS        wire 4        Pin 8

The lamp output is on the Arduino's Pin 6

	