# libcrazyradio #

So I have a bunch of nrf24 based sensors which works great, using
bitbanged spi modules like these:

http://www.amazon.com/Arduino-NRF24L01-2-4GHz-Wireless-Transceiver/dp/B00O9O868G

And they work great.  But then I found that the crazyflie copter
folks had a usb radio with nrf24 and a nice antenna.

http://www.amazon.com/SeeedStudio-Crazyradio-2-4Ghz-Dongle-Antenna/dp/B00VYA3A2U

The antenna gives them great range, and it allows me to use any linux
machine with usb to talk to nrf24 radios without having to have
gpio pins available.

Truthfully, you could do this with an avr and v-usb or a atmega32u4 or
something, but I had a crazyflie copter, so it made some sense.

Anyway, this is a C library for that usb device.

## RX/TX Mode ##

So originally, the firmware for the crazyradio only did TX, since
that's all it needed to do to send commands to the crazyflie radio.

But some folks put together RX support for the radio firmware as well
(thanks internet!), so you must have updated firmware to run this.

I'm not sure what version of firmware got PRX support, but since
it's pretty simple to update the firmware, I've set the minimum
firmware requirements to version 99.53 or better.

Firmware is here: https://github.com/bitcraze/crazyradio-firmware

