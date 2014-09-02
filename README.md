ISSNotify
=========

ISS Notification Lamp
An Arduino sketch to activate a light when the International 
Space Station is above the current loctation.

This is an Arduino sketch that communicates with an ENC28J60 ethernet interface to fetch the current date and time, the approximate 
location via IP address reverse lookup, and a list of future passes of
 the International Space Station above the current location. When the
 ISS is above, the Arduino activates an output connected to a lamp.
