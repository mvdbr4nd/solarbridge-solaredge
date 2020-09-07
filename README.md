# solarbridge-solaredge
solarbridge for solaredge on wemos d

Warning this ino (sketch) is not tested at all! If you have a SolarEdge + APIkey you will have to test this sketch yourself. Its based on the growatt version from oepi-loepi which i use myself in combination with growatt API. 

This sketch is used on a WEMOS D mini to hook up with an Eneco Meter Adapter in order to provide solar power production for ZonOpToon without using expensive kwh hardware.

Data from the website (from the convertor) is used to generate pulses (1 pulse / watt) to feed the info in ZonOpToon.
