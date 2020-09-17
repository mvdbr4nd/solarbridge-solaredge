# solarbridge-solaredge
solarbridge for solaredge on wemos d mini (or ESP8266)

This project is based on the solarbridge - growatt version and is changed to use the solaredge API. To build this project please use PlatformIO which will take care of all required libraries automaticly. 

This sketch is used on a WEMOS D mini to hook up with an Eneco Meter Adapter in order to provide solar power production for ZonOpToon without using expensive kwh hardware.
Data from the website (from the convertor) is used to generate pulses (1 pulse / watt) to feed the info in ZonOpToon.

Updated to use the Arduino JSON6 and latest WifiManager
