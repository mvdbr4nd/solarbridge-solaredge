# SolarBridge - Growatt for platformio version

solarbridge for solaredge on wemos d mini (or ESP8266)
This project is based on the solarbridge - growatt version and is changed to use the solaredge API. To build this project please use PlatformIO which will take care of all required libraries automaticly. This sketch is used on a WEMOS D mini (ESP8266) to hook up with an Eneco Meter Adapter in order to provide solar power production for ZonOpToon without using expensive kwh hardware.
Data from the website (from the convertor) is used to generate pulses (1 pulse / watt) to feed the info in ZonOpToon.


## Forked from: SolarBridge v2.2.1 by Oepi-Loepi:
- Version 2.1, corrected for daily totals
- Version 2.2, problem wih some logins solved
- Version 2.2.1, minor issue improved. When power is 0, the interval was set to 1 hr so no pulses were set during this time even when power returns
All rights reserved
Do not use for commercial purposes


## How to build this project
* This project is dependending on several libs which are defined in the platformio.ini file and will be downloaded automaticly by platformio. 

## Workings

This bridge will get the data from the GroWatt web interface (API) and create S0 pulses and led flashes
Each pulse/flash will suggest 1 Watt

## Wiring

    On GPIO 5 a PC817 optocoupler is connected
    PIN D1 ----  PC817 (anode, pin 1, spot)
    GND -------  PC817 (cathode, pin 2)
                 PC817 p3 ------ Inner          3,5mm stereo jack plug
                 PC817 p4 ------ Outer (2x)     
 
### Optional
On GPIO 4 a resistor (330 ohm and red LED are connected in series:

    PIN D2 ----  Resistor 33Ohm) ---- (Long LED lead ---- LED ---- Short LED Lead) ----- GND

_by setting Led to BUILT_INLED you can use the internal led on the print of the wemos instead of this external LED_

### Instructions

After uploading the sketch to the Wemos D1 mini, connect to the AutoConnectAP wifi. 
Goto 192.168.4.1 in a webbrowser and fill in all data including GroWatt credentials.

## Notes
- Settings are saved on the Flash (wemos mini) and will reloaded from startup. 
- Resetting the bridge will clear all settings (WIFI and Username Password).


