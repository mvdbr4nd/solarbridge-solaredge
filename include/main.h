#include <Arduino.h>
#include <ESP8266WiFi.h>          
#include <DNSServer.h>
#include <WiFiManager.h>         
#include <ArduinoJson.h> 

// global variables
extern char siteid[60];
extern char apikey[120];

extern bool ConnectionPossible;

extern bool reset1;
extern bool reset2;

extern unsigned long timebetweenpulses; 
extern unsigned long Getdatafrequencyset;
extern unsigned long Getdatafrequency;

extern String todayval;
extern String monthval;

extern int PulsesGenerated;
extern float NewDayTotal;
extern float PrevDayTotal;
extern float ActualPower;
extern float PowerCorrection;
extern int NumberofPeriodSinceUpdate;
extern int EnergyfromDaytotal;
extern float CorrectedPowerNextPeriod;
extern int MaxNumberofCorrections;
extern bool newday;    

extern WiFiServer server;