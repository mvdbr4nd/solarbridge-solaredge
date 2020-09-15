# solarbridge-solaredge
solarbridge for solaredge on wemos d

This bridge is created based on the growatt version from oepi-loepi which i use myself in combination with growatt API. This fork is changed to support the solaredge api in the same way as the original growatt version. 

This sketch is used on a WEMOS D mini to hook up with an Eneco Meter Adapter in order to provide solar power production for ZonOpToon without using expensive kwh hardware.

Data from the website (from the convertor) is used to generate pulses (1 pulse / watt) to feed the info in ZonOpToon.

you will need:
- WEMOS D MINI
- Arduino IDE
- USB cable 
- connect the Wemos GPIO pins to a JACK 3.5mm
- Toon + Eneco MeterAdpater ready for ZonOpToon
- Check the source in the ino for the wiring + lib version to be used
