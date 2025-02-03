# pingtester
ESP32 based ping tester

Making a toy to ping an address over wifi, and show the time to travel.
using an ESP32-S3-WROOM-1, 
and adafruit clone 14-segment LED HT16K33 backpacks

Started December 2024, Finished on Feb 2, 2025

To connect it to your wifi, plug the top connector into USB-C power

If it can't find wifi, it'll turn on it's own AP Wi-Fi network called

    PingToy

The LED display will show the Wi-Fi network and Password.

It will also show a web address for you to go to to set it up.
It does not have SSL, so it's plain old http://

The LED display will show the URL.

You can surf to it, set the Wi-Fi network name and password
Also enter the name of the server to ping, no URL on the front, just
server name like "www.google.com"

When the ping tester is running, it also has a web page available
for the same settings.  To find it, run something like this:

    nmap -p 80 192.168.1.0/24

on your computer to find it.


FUTURE:

add in HTTP fetcher to get and search for strings on public web pages
