#include <main.h>
#include <solaredge.h>
#include <webserver.h>

// global variables
char siteid[60] = "";
char apikey[120] = "";

bool ConnectionPossible=false;

bool reset1 = false;
bool reset2 = false;

//set the following to "D2" if you want to toggle Pin D2 or LED_BUILTIN if you rather want to toggle the led on the print
int led = LED_BUILTIN;  
int meteradapter = D1; 

unsigned long startMillis;    //some global variables available anywhere in the program
unsigned long currentMillis;
unsigned long period = 2000;    //time between pulses, initial 2000 ms
unsigned long startMillis2;    //some global variables available anywhere in the program
unsigned long currentMillis2;
unsigned long InitialGetdatafrequency = 20000; //only first time to get data from Growatt, then time will be set to Getdatafrequency)
unsigned long Getdatafrequency = 60000; //time between data transfers from GroWatt
unsigned long Getdatafrequencyset = 0; //time between data transfers from GroWatt
unsigned long timebetweenpulses = 2000; //time between pulses calculated (initial)
bool shouldSaveConfig = false;
bool ActualPowerZero = false;

int PulsesGenerated = 0;    //pulses generated in the periods from last change    (when blink, PulsesGenerated++ and reset after calculation of correction)
float NewDayTotal = -1;
float PrevDayTotal = -1;     //shoud be -1
float ActualPower = 0;
float PowerCorrection = 0;
int NumberofPeriodSinceUpdate = 0;
int EnergyfromDaytotal = 0;
float CorrectedPowerNextPeriod = 0;
int MaxNumberofCorrections = 0;
bool newday = false;        // must be false first time

void configModeCallback (WiFiManager *myWiFiManager) {
	Serial.println("Entered config mode");
	Serial.println(WiFi.softAPIP());
	//if you used auto generated SSID, print it
	Serial.println(myWiFiManager->getConfigPortalSSID());
}

//callback notifying us of the need to save config
void saveConfigCallback () {
	Serial.println("Should save config");
	shouldSaveConfig = true;
}

void setup() {
	// put your setup code here, to run once:
	Serial.begin(115200);

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);
	
	pinMode(led, OUTPUT);
  	digitalWrite(led, LOW);
  	
	pinMode(meteradapter, OUTPUT);
  	digitalWrite(meteradapter, HIGH);

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

				StaticJsonDocument<200> doc;
				DeserializationError error = deserializeJson(doc, configFile);
				if (error) {
					Serial.println("failed to load json config");
					return;
				}

				// Extract each characters by one by one
				while (configFile.available()) {
					Serial.print((char)configFile.read());
				}
				Serial.println();
				Serial.println("\nparsed json");
			
				strcpy(siteid, doc["siteid"]);
				strcpy(apikey, doc["apikey"]);

			} else {
				Serial.println("failed to load json config");
			}
		}
	} else {
		Serial.println("failed to mount FS");
	}

	// setup wifi
	WiFiManagerParameter custom_text("<p>Please specify your SolarEdge credentials</p>");
	WiFiManagerParameter cust_key("API key", "apikey", apikey, 60);
	WiFiManagerParameter cust_id("Site Id", "siteid", siteid, 60);

	WiFiManager wifiManager;
	wifiManager.setSaveConfigCallback(saveConfigCallback);
	wifiManager.setAPCallback(configModeCallback);    
	wifiManager.addParameter(&cust_key);
	wifiManager.addParameter(&cust_id);
	wifiManager.autoConnect("SolarBridgeAP");

	wifiManager.setConfigPortalTimeout(180);

	Serial.println("connected...yeey :)");

	//read updated parameters
	strcpy(apikey, cust_key.getValue());
	strcpy(siteid, cust_id.getValue());
	Serial.println(apikey);
	Serial.println(siteid);

	delay(2000);

	Serial.println("local ip");
	Serial.println(WiFi.hostname());
	Serial.println(WiFi.localIP());
	Serial.println(WiFi.gatewayIP());
	Serial.println(WiFi.subnetMask());
	Serial.println(WiFi.dnsIP());
	
	//write config to config.json
	if (shouldSaveConfig) {
		Serial.println("saving config to    FS");    
		StaticJsonDocument<200> doc;

		doc["apikey"] = apikey;
		doc["siteid"] = siteid;

		File configFile = SPIFFS.open("/config.json", "w");
		if (!configFile) {
			Serial.println("failed to open config file for writing");
		}
		
		// Serialize JSON to file
		if (serializeJson(doc, configFile) == 0) {
			Serial.println(F("Failed to write to file"));
		}
		
		// Close the file
		configFile.close();
		//end save
	}

	WiFi.begin();
	server.begin();
}

void blinkled() {
	if (led != LED_BUILTIN) {
		digitalWrite(led, HIGH);    
	}
	else {
		digitalWrite(led, LOW);    
	}
	
	digitalWrite(meteradapter, HIGH);
	delay(100);                             
	digitalWrite(meteradapter, LOW);
	
	if (led != LED_BUILTIN) {
		digitalWrite(led, LOW);
	} 
	else {
		digitalWrite(led, HIGH);
	}	
}

void loop() {
	// put your main code here, to run repeatedly:
	webserver();
 
	if ((reset1) && (reset2)) { 
			delay(2000);
			Serial.println("Reset requested");
			Serial.println("deleting configuration file");
			SPIFFS.remove("/config.json");
			WiFiManager wifiManager;
			wifiManager.resetSettings();
			delay(500);
			ESP.reset();
	}

	currentMillis = millis();    //get the current "time" (actually the number of milliseconds since the program started)
	if (currentMillis - startMillis >= period) { //test whether the pulsetime has eleapsed
		Serial.print("PulsesGen.: ");Serial.print(PulsesGenerated);
		Serial.print(" ** EnergyDay: ");Serial.print(EnergyfromDaytotal);
		Serial.print(" ** Act.Power: ");Serial.print(ActualPower);
		Serial.print(" ** N.Periods :");Serial.print(NumberofPeriodSinceUpdate);
		Serial.print(" ** PowerCorre.: ");Serial.print(PowerCorrection);
		Serial.print(" ** Corr.Power: ");Serial.print(CorrectedPowerNextPeriod);
		Serial.print(" ** timebetw.pulses: ");Serial.print(timebetweenpulses);
		Serial.print(" ** newday: ");Serial.println(newday);

		if (!ActualPowerZero) {
			blinkled();
		}
		PulsesGenerated++;
		period = timebetweenpulses;
		startMillis = currentMillis;    //IMPORTANT to save the start time of the current LED state.
	}

	currentMillis2 = millis();    //get the current "time" (actually the number of milliseconds since the program started)
	if (currentMillis2 - startMillis2 >= Getdatafrequencyset) { //test whether the period has elapsed to get new data from the server
		startMillis2 = currentMillis2;    //IMPORTANT to save the start time of the current LED state.
		if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
			getdata();
		}
	}
}