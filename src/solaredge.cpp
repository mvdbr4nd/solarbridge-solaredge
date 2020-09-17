#include <solaredge.h>

String JSESSIONID;
String SERVERID;

String todayval = "Not Connected Yet";
String monthval= "Not Connected Yet";
bool cookiestep= false;

String fingerprint = "69 01 51 C2 49 16 4A 38 93 FA 7C A8 E4 BC 61 9A 25 4B 98 BF";

void getdata() {
	//get new data from the solaredge server.
	HTTPClient http;
	const char * headerkeys[] = {"User-Agent","Set-Cookie","Cookie","Date","Content-Type","Connection"} ;
	size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
	String apiurl = "";
	apiurl = "https://monitoringapi.solaredge.com/site/" +String(siteid) +"/overview.json?api_key=" +String(apikey);
	http.begin(apiurl, fingerprint); 
	
	int httpCode = http.GET();
	if ((httpCode=200) || (httpCode = 301) || (httpCode = 302)){
		ConnectionPossible = true;
		String payload = http.getString();
		const size_t capacity = 5*JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(7);

		DynamicJsonDocument jsonBuffer(capacity);
		auto error = deserializeJson(jsonBuffer, payload);
		if (error) {
			Serial.print(F("deserializeJson() failed with code "));
			Serial.println(error.c_str());
			return;
		} 
		else {
			ActualPower  = jsonBuffer["overview"]["currentPower"]["power"];
			float MonthValue  = jsonBuffer["overview"]["lastMonthData"]["energy"];
			NewDayTotal = jsonBuffer["overview"]["lastDayData"]["energy"];

			NewDayTotal = NewDayTotal / 1000;
			MonthValue = MonthValue / 1000;

			todayval = String(NewDayTotal)+ " KWh";
			monthval = String(MonthValue)+ " KWh";

			Serial.print("PrevDayTotal :");
			Serial.println(PrevDayTotal);
			Serial.print("NewDayTotal :");
			Serial.println(NewDayTotal);

			if (NewDayTotal == 0) { 
				// it will be a new day or a new counting period.
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
 				//reset the number of periods since last update
				NumberofPeriodSinceUpdate=0;
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
 				
				//calculation of the interval
				timebetweenpulses = 3600000/CorrectedPowerNextPeriod;
				Serial.print("timebetweenpulses :");
				Serial.println(timebetweenpulses);
			}
			else{
				//1 hour interval when ActualPower = 0
				timebetweenpulses = 360000; 
				CorrectedPowerNextPeriod = 0;
				PowerCorrection = 0;
			}
			Getdatafrequencyset = Getdatafrequency;
		}
	}
	http.end(); 
}