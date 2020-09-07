/******************************************************************************************
 * SolarBridge - SolarEdge 1.0 (MvdB)
 * Forked from SolarBridge v2.2.1 by Oepi-Loepi
 * 
 * 
 * Version 2.1, corrected for daily totals
 * Version 2.2, problem wih some logins solved
 * Version 2.2.1, minor issue improved. When power is 0, the interval was set to 1 hr so no pulses were set during this time even when power returns
 * 
 * All rights reserved
 * Do not use for commercial purposes
 * 
 * Solar Bridge for SolarEdge solar converters
 * This bridge will get the data from the SolarEdge web interface (API) and create S0 pulses and led flashes
 * Each pulse/flash will suggest 1 Watt
 * 
 * Library versions
 *  ESP board 2.5.2
 *  ArduinoJson 5.13.5
 *  WifiManager 0.15.0
 * 
 * 
 * On GPIO 4 a resistor (330 ohm and red LED are connected in series 
 * PIN D2 ----  Resistor 33Ohm) ---- (Long LED lead ---- LED ---- Short LED Lead) ----- GND
 * 
 * On GPIO 5 a PC817 optocoupler is connected
 * PIN D1 ----  PC817 (anode, pin 1, spot)
 * GND -------  PC817 (cathode, pin 2)
 * 
 * Pin 3 and 4 of the PC817 will be a potential free contact
 * 
 * After uploading the sketch to the Wemos D1 mini, connect to the AutoConnectAP wifi. 
 * Goto 192.168.4.1 in a webbrowser and fill in all data including API Key.
 * 
*/



#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include "ESP8266HTTPClient.h"
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

//define your default values here, if there are different values in config.json, they are overwritten.
char APIkey[33] = "L4QLVQ1LOKCQX2193VSEICXW61NP6B1O";

char static_ip[18] = "192.168.0.58";
char static_gw[18] = "192.168.0.1";
char static_sn[18] = "255.255.255.0";
char static_dn[18] = "8.8.8.8";
bool dhcp = true;

bool ConnectionPossible=false;

char* wifiHostname = "SolarBridge";

String JSESSIONID;
String SERVERID;

bool cookiestep= false;

bool reset1 = false;
bool reset2 = false;

int led = 4;
int meteradapter = 5;


String todayval = "Not Connected Yet";
String monthval= "Not Connected Yet";

unsigned long startMillis;  //some global variables available anywhere in the program
unsigned long currentMillis;
unsigned long period = 2000;  //time between pulses, initial 2000 ms
unsigned long startMillis2;  //some global variables available anywhere in the program
unsigned long currentMillis2;
unsigned long InitialGetdatafrequency = 20000; //only first time to get data from Growatt, then time will be set to Getdatafrequency)
unsigned long Getdatafrequency = 60000; //time between data transfers from GroWatt
unsigned long Getdatafrequencyset = 0; //time between data transfers from GroWatt
unsigned long timebetweenpulses = 2000; //time between pulses calculated (initial)
bool shouldSaveConfig = false;

int PulsesGenerated = 0;  //pulses generated in the periods from last change  (when blink, PulsesGenerated++ and reset after calculation of correction)
float NewDayTotal = -1;
float PrevDayTotal = -1;   //shoud be -1
float ActualPower = 0;
float PowerCorrection = 0;
int NumberofPeriodSinceUpdate = 0;
int EnergyfromDaytotal = 0;
float CorrectedPowerNextPeriod = 0;
int MaxNumberofCorrections = 0;
bool newday = false;    // must be false first time

WiFiServer server(80);
String header_web = "";


//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  pinMode(meteradapter, OUTPUT);
  digitalWrite(meteradapter, HIGH);
  startMillis = millis();  //initial start time
  startMillis2 = millis();  //initial start time
  Getdatafrequencyset = InitialGetdatafrequency;

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(APIkey, json["APIkey"]);

          if(json["dhcp"]) {
            Serial.println("Setting up wifi from dhcp config");
            dhcp=true;
          } else{
            Serial.println("Setting up wifi from Static IP config");
          }        

          if(json["ip"]) {
            Serial.println("Last known ip from config");
            strcpy(static_ip, json["ip"]);
            strcpy(static_gw, json["gateway"]);
            strcpy(static_sn, json["subnet"]);
            Serial.println(static_ip);
          } else {
            Serial.println("no custom ip in config");
          }
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
 
  WiFiManagerParameter custom_APIkey("APIkey", "APIkey", APIkey, 32);
  WiFiManagerParameter custom_text("<p>Select Checkbox for DHCP  ");
  WiFiManagerParameter custom_text2("</p><p>DHCP will be effective after reset (power off/on)</p>");
  WiFiManagerParameter custom_text3("</p><p>So first wait for a minute after saving and then reboot by removing power.</p>");
  WiFiManagerParameter custom_dhcp("dhcp", "dhcp on", "T", 2, "type=\"checkbox\" ");

  WiFiManager wifiManager;
  WiFi.hostname(wifiHostname);

  wifiManager.setSaveConfigCallback(saveConfigCallback);

  if (!dhcp){
    IPAddress _ip,_gw,_sn;
    _ip.fromString(static_ip);
    _gw.fromString(static_gw);
    _sn.fromString(static_sn);
  
    wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  }else{
    wifiManager.autoConnect("AutoConnectAP");
  }
  
  wifiManager.addParameter(&custom_APIkey);
  wifiManager.addParameter(&custom_text);
  wifiManager.addParameter(&custom_dhcp);
  wifiManager.addParameter(&custom_text2);
  wifiManager.addParameter(&custom_text3);

  wifiManager.setMinimumSignalQuality();
  
  if (!wifiManager.autoConnect("AutoConnectAP")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  Serial.println("connected...yeey :)");

  if (!dhcp){
    IPAddress _dn;
    _dn.fromString(static_dn);
    WiFi.hostname(wifiHostname);
    WiFi.mode(WIFI_STA);
    WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), _dn);
    WiFi.begin();
  }

  delay(2000);

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());
  Serial.println(WiFi.dnsIP());
  
  dhcp = (strncmp(custom_dhcp.getValue(), "T", 1) == 0);

  strcpy(APIkey, custom_APIkey.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["APIkey"] = APIkey;
    json["dhcp"] = dhcp;

    json["ip"] = WiFi.localIP().toString();
    json["gateway"] = WiFi.gatewayIP().toString();
    json["subnet"] = WiFi.subnetMask().toString();
    
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.prettyPrintTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  delay(1000);
  WiFi.begin();
  server.begin();
}


void webserver(){
    // Set web server port number to 80 and create a webpage
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        //Serial.write(c);                    // print it out the serial monitor
        header_web += c;
        if (c == '\n') {                    // if the byte is a newline character
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<title>SolarBridge</title>");
            client.println("<meta name=\"author\" content=\"SolarBridge by oepi-loepi\">");
            client.println("<style>html {font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println("p.small {line-height: 1; font-size:70%;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 10px 20px;");
            client.println("text-decoration: none; font-size: 24px; margin: 2px; cursor: pointer;}</style>");
            
            
            client.println("<script type=\"text/JavaScript\">");
            client.println("<!--");
            client.println("function TimedRefresh( t ) {setTimeout(\"location.reload(true);\", t);}");
            client.println("//   -->");
            client.println("</script>");
            client.println("</head>");
            client.println("<body onload=\"JavaScript:TimedRefresh(60000);\">");    //reloadtime of SolarBridge webpage
            client.println("<h1>SolarBridge</h1>");
            client.println("<hr>");
            client.println("<p></p>");
             
            if ((header_web.indexOf("GET /") >= 0) && (header_web.indexOf("GET /reset/") <0)) {

              reset1 = false;
              reset2 = false;

              client.println("<p class=\"small\">IP: " +WiFi.localIP().toString() + "<br>");
              client.println("Gateway: " +WiFi.gatewayIP().toString()+ "<br>");
              client.println("Subnet: " +WiFi.subnetMask().toString() + "<br><br>");
  
              client.println("APIkey: " + String(APIkey) + "<br>");
              
              String YesNo = "No";
              if (ConnectionPossible){
                  YesNo = "Yes";
              }else{
                  YesNo = "No";
              };
              client.println("Connection Posssible: " + YesNo + "<br>");
              client.println("1000 imp/kW</p>");
  
              client.println("<hr>");
              client.println("<h1>Actual: " + String(ActualPower) + " Watt<br>");
              client.println("Today: " + todayval + "<br>");
              client.println("Month: " + monthval + "</h1>"); 
              client.println("<hr>");    
              client.println("<p> </p>");
              
              client.println("<p class=\"small\">EnergyfromDaytotal: "+ String(EnergyfromDaytotal)  + "<br>");
              client.println("PulsesGenerated: "+ String(PulsesGenerated)  + "<br>");
              client.println("NumberofPeriodSinceUpdate: "+ String(NumberofPeriodSinceUpdate)  + "<br>");
              client.println("PowerCorrection: "+ String(PowerCorrection,2)  + "<br>");
              client.println("CorrectedPowerNextPeriod: "+ String(CorrectedPowerNextPeriod,2)  + "<br>");
              client.println("Is a newday: "+ String(newday)  + "<br>");
              client.println("timebetweenpulses: "+ String(timebetweenpulses)  + "</p>");

              client.println("<p><a href=\"/reset/req\"><button class=\"button\">Reset</button></a></p>");
              client.println("<p>When reset is pressed, all settings are deleted and AutoConnect is restarted<br>");
              client.println("Please connect to wifi network AutoConnectAP and after connect go to: 192.168.4.1 for configuration page </p>");
              client.println("</body></html>");
            }
            
            if (header_web.indexOf("GET /reset/req") >= 0) {
              reset1 = true;
              reset2 = false;
              Serial.print("Reset1 :");
              Serial.print(reset1);
              Serial.print(", Reset2 :");
              Serial.println(reset2);
              client.println("<p>Are you sure?</p>");
              client.println("<p> </p>");
              client.println("<p><a href=\"/reset/ok\"><button class=\"button\">Yes</button></a><a href=\"/../..\"><button class=\"button\">No</button></a></p>");
              client.println("</body></html>");
            }

            if (header_web.indexOf("GET /reset/ok") >= 0) {
              reset2 = true;
              Serial.print("Reset1 :");
              Serial.print(reset1);
              Serial.print(", Reset2 :");
              Serial.println(reset2);
              client.println("<p>Reset carried out</p>");
              client.println("<p>Please connect to wifi network AutoConnectAP and goto 192.168.4.1 for configuration</p>");
              client.println("</body></html>");
            }
            
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header_web = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }  
}


void getdata(){
          //get new data from the solaredge server.
          
          HTTPClient http;
          const char * headerkeys[] = {"User-Agent","Set-Cookie","Cookie","Date","Content-Type","Connection"} ;
          size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
          char apiurl[96] = "url";
        
          sprintf(apiurl, "https://monitoringapi.solaredge.com/sites/1,4/overview?api_key=%d", APIkey);
          
          http.begin(apiurl);

          http.setReuse(true);
          http.setUserAgent("Dalvik/2.1.0 (Linux; U; Android 9; ONEPLUS A6003 Build/PKQ1.180716.001");
          http.addHeader("Content-type", "application/x-www-form-urlencoded");

          http.collectHeaders(headerkeys,headerkeyssize);
        
          int code = http.POST("language=1"); //Send the request
          if ((code=200) || (code = 301) || (code = 302)){

                     String payload = http.getString();; //Get the request response payload
                     Serial.println(payload); //Print the response payload
                        
                        const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
                        DynamicJsonBuffer jsonBuffer(capacity);
                        
                        //Serial.println("JSON covert..");
                        JsonObject& root = jsonBuffer.parseObject(payload);
                        if (!root.success()) {
                            Serial.println("parseObject() failed");
                        }
                        else {
                            ActualPower  = root["overview"][0]["currentpower"]["power"];
                            float MonthValue  = root["overview"][0]["lastMonthData"]["enery"];
                            NewDayTotal = root["overview"][0]["lastDayData"]["todayValue"];

                            todayval = String(NewDayTotal)+ " KWh";
                            monthval = String(MonthValue)+ " KWh";

                            Serial.print("PrevDayTotal :");
                            Serial.println(PrevDayTotal);
                            Serial.print("NewDayTotal :");
                            Serial.println(NewDayTotal);
    

                            if (NewDayTotal == 0) {             //it will be a new day or a new counting period.
                              Serial.println("A new day has started.");
                              newday = true;
                              PrevDayTotal=0;
                              PulsesGenerated = 0;
                              NumberofPeriodSinceUpdate=0;
                              MaxNumberofCorrections= 0;
                              CorrectedPowerNextPeriod = 0;  
                              PowerCorrection= 0;
                            }
    
                            if (!newday){
                              Serial.println("No new day registered yet so impossible to correct to current totals.");
                            }
                            
                            NumberofPeriodSinceUpdate++;
                            EnergyfromDaytotal = (int)(NewDayTotal *1000);  //increase energy read from solarEdge from the daytotals (in Wh)
                            if ((NewDayTotal != PrevDayTotal) && (PrevDayTotal != -1) && (newday==true)){
                                   
                                Serial.print("EnergyfromDaytotal :");
                                Serial.println(EnergyfromDaytotal);
                        
                                Serial.print("PulsesGenerated :");
                                Serial.println(PulsesGenerated); //energy accounted for by pulses since last update (in Wh)

                                float EnergytobeCorrected = EnergyfromDaytotal - PulsesGenerated;
                                
                                Serial.print("EnergytobeCorrected :");
                                Serial.println(EnergytobeCorrected); //energy accounted for by pulses since last update (in Wh)

                                Serial.print("NumberofPeriodSinceUpdate :");
                                Serial.println(NumberofPeriodSinceUpdate); 
                        
                                PowerCorrection = (EnergytobeCorrected/NumberofPeriodSinceUpdate); //Correction of Power in W
                                Serial.print("PowerCorrection :");
                                Serial.println(PowerCorrection);

                                MaxNumberofCorrections = NumberofPeriodSinceUpdate;
                        
                                NumberofPeriodSinceUpdate=0;             //reset the number of periods since last update
                                PrevDayTotal = NewDayTotal;
                            }

                            if ((PrevDayTotal == -1) && (NewDayTotal > 0)){
                                          Serial.print("PrevDayTotal UPDATED! :");
                              PrevDayTotal = NewDayTotal;
                                          Serial.println(PrevDayTotal);
                            }

                            Serial.print("ActualPower :");
                            Serial.println(ActualPower);

                            if (ActualPower > 1) {
                                  if (NumberofPeriodSinceUpdate <= MaxNumberofCorrections){
                                    CorrectedPowerNextPeriod =   ActualPower +  PowerCorrection;
                                  }
                                  else{
                                    CorrectedPowerNextPeriod =   ActualPower +  PowerCorrection;
                                  }
                                  Serial.print("CorrectedPowerNextPeriod :");
                                  Serial.println(CorrectedPowerNextPeriod);
                
                                  timebetweenpulses = 3600000/CorrectedPowerNextPeriod; //calculation of the interval
                                  Serial.print("timebetweenpulses :");
                                  Serial.println(timebetweenpulses);
                
                            }
                            else{
                                   timebetweenpulses = 360000; //1 hour interval when ActualPower = 0
                                   CorrectedPowerNextPeriod = 0;
                                   PowerCorrection = 0;
                            }
                            Getdatafrequencyset = Getdatafrequency;
                        }
              }                
        http.end();  //Close connection
}


void blinkled() {
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(meteradapter, HIGH);
  delay(100);               // wait for some time
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  digitalWrite(meteradapter, LOW);
}



void loop() {

  webserver();
 
  if ((reset1) && (reset2)) { //all reset pages have been acknowledged and reset parameters have been set from the webpages
      delay(2000);
      Serial.println("Reset requested");
      Serial.println("deleting configuration file");
      SPIFFS.remove("/config.json");
      WiFiManager wifiManager;
      wifiManager.resetSettings();
      delay(500);
      ESP.reset();
  }
  
  currentMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)
  if (currentMillis - startMillis >= period) { //test whether the pulsetime has eleapsed

    Serial.print("PulsesGen.: ");Serial.print(PulsesGenerated);
    Serial.print(" ** EnergyDay: ");Serial.print(EnergyfromDaytotal);
    Serial.print(" ** Act.Power: ");Serial.print(ActualPower);
    Serial.print(" ** N.Periods :");Serial.print(NumberofPeriodSinceUpdate);
    Serial.print(" ** PowerCorre.: ");Serial.print(PowerCorrection);
    Serial.print(" ** Corr.Power: ");Serial.print(CorrectedPowerNextPeriod);
    Serial.print(" ** timebetw.pulses: ");Serial.print(timebetweenpulses);
    Serial.print(" ** newday: ");Serial.println(newday);

    blinkled();
    PulsesGenerated++;
    period = timebetweenpulses;
    startMillis = currentMillis;  //IMPORTANT to save the start time of the current LED state.
  }

  currentMillis2 = millis();  //get the current "time" (actually the number of milliseconds since the program started)
  if (currentMillis2 - startMillis2 >= Getdatafrequencyset) { //test whether the period has elapsed to get new data from the server
    if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
          getdata();
    }
     startMillis2 = currentMillis2;  //IMPORTANT to save the start time of the current LED state.
  }
}
