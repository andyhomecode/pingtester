# pingtester
ESP32 based ping tester

Making a toy to ping an address over wifi, and show the time to travel.

Built with an ESP32-S3-WROOM-1, 
two Adafruit clone 14-segment LED HT16K33 backpacks,
and a slide switch. 

Started December 2024, Finished on Feb 9, 2025

To use it:

1) plug it into USB-C power.

2) push the switch down to put it in SETUP mode

3) note the IP address shown on the LED display, usually 192.168.4.1

3) connect to the wifi network:
	SSID: PingToy
	password: LoganMcNeil

4) open a browswer and go to http://192.168.4.1  (no SSL)

5) enter the configuration you want. Hit Submit. It will reboot.

6) flip switch to RUN.  After that, it'll remember it's settings between power cycles.
