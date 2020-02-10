# esp8266-garagedoor-alexa
Alexa home automation using esp8266 board w/ wifi to control an electric garage-door opener

Most of this code is directly adapted from the kakopapa/sinric github site.  
https://github.com/kakopappa/sinric

I added code for controlling the servo which will switch on the kettle. 

You will need to follow the directions on the README to setup the sinric (free) account 
then create a device for the electric door to control.

This sketch will control a mosfet board using alexa.   It uses an wemos mini d1 board which is an esp8266 
with wifi adapter built in.   You can also use a relay for this project if you invert the logic to the mosfet board for the relay (HIGH -> LOW, LOW -> HIGH)
and it would work the same.

