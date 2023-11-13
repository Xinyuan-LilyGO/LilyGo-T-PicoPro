/*
  WiFi Web Server

 A simple web server that shows the value of the analog input pins.

 created 13 July 2010
 by dlf (Metodo2 srl)
 modified 31 May 2012
 by Tom Igoe
 modified in Jul 2019 for WiFiEspAT library
 by Juraj Andrassy https://github.com/jandrassy
 */

#include <WiFiEspAT.h>
#include "utilities.h"

#ifndef WIFI_SSID
#define WIFI_SSID             "Your WiFi SSID"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD         "Your WiFi PASSWORD"
#endif

WiFiServer server(80);

void setup()
{

    Serial.begin(115200);
    while (!Serial);

    /* Configure pins for communication with ESP-AT */
    SerialAT.setRX(BOARD_WIFI_RX);
    SerialAT.setTX(BOARD_WIFI_TX);
    SerialAT.begin(115200);

    WiFi.init(SerialAT);

    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("Communication with WiFi module failed!");
        // don't continue
        while (true);
    }

    WiFi.disconnect();     // to clear the way. not persistent
    WiFi.setPersistent();  // set the following WiFi connection as persistent
    WiFi.endAP();          // to disable default automatic start of persistent AP at startup

    Serial.print("Connect to ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // waiting for connection to Wifi network set with the SetupWiFiConnection sketch
    Serial.println("Waiting for connection to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print('.');
    }
    Serial.println();

    server.begin();

    IPAddress ip = WiFi.localIP();
    Serial.println();
    Serial.println("Connected to WiFi network.");
    Serial.print("To access the server, enter \"http://");
    Serial.print(ip);
    Serial.println("/\" in web browser.");
}

void loop()
{

    WiFiClient client = server.available();
    if (client) {
        IPAddress ip = client.remoteIP();
        Serial.print("new client ");
        Serial.println(ip);

        while (client.connected()) {
            if (client.available()) {
                String line = client.readStringUntil('\n');
                line.trim();
                Serial.println(line);

                // if you've gotten to the end of the HTTP header (the line is blank),
                // the http request has ended, so you can send a reply
                if (line.length() == 0) {
                    // send a standard http response header
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: text/html");
                    client.println("Connection: close");  // the connection will be closed after completion of the response
                    client.println("Refresh: 5");  // refresh the page automatically every 5 sec
                    client.println();
                    client.println("<!DOCTYPE HTML>");
                    client.println("<html>");
                    // output the value of analog input pins
                    for (int analogChannel = 0; analogChannel < 4; analogChannel++) {
                        int sensorReading = analogRead(analogChannel);
                        client.print("analog input ");
                        client.print(analogChannel);
                        client.print(" is ");
                        client.print(sensorReading);
                        client.println("<br />");
                    }
                    client.println("</html>");
                    client.flush();
                    break;
                }
            }
        }

        // close the connection:
        client.stop();
        Serial.println("client disconnected");
    }
}

