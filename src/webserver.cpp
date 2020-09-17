#include <webserver.h>

String header_web = "";
WiFiServer server(80);

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

							client.println("APIkey: " + String(apikey) + "<br>");
							client.println("SiteID: " + String(siteid) + "</p>");
							
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
							reset1 = true;
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
					} 
					else { // if you got a newline, then clear currentLine
						currentLine = "";
					}
				} 
				else if (c != '\r') {  // if you got anything else but a carriage return character,
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