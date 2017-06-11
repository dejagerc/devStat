#include <OneWire.h>
#include <devstat.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "max6675.h" // Thermo couple 

#define USERNAME "Get from devstat.net"
#define SID "Your wifi SID"
#define PASS "Your wifi password"

OneWire ds(D1);
ESP8266WiFiMulti WiFiMulti;
devStat dsn(USERNAME); // User id 

// Thermocouple 
int thermoDO = D4;
int thermoCS = D3;
int thermoCLK = D2;
byte TCPos;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

void setup() {
    Serial.begin(115200); // Set serial communication speed 
    delay(500); // Wait for everything to settle 
    pinMode(LED_BUILTIN, OUTPUT); // Blink light   
    WiFiMulti.addAP(SID, PASS);
    Serial.println("Connecting Wifi...");
    
    dsn.setDebug(2); // Display everything 
    dsn.OneWireFindDevices(&ds);  // Search for devices 
    TCPos = dsn.addDevice("CHIM_01"); // Exhaust gas (chimney)
}

void loop() {

      if((WiFiMulti.run() == WL_CONNECTED)) {
        dsn.OneWireIssueCopyToScratchAll(); // Update scratch from memory
        digitalWrite(LED_BUILTIN, HIGH); 
        delay(490);                       // wait for a second
        digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
        dsn.smoothData(thermocouple.readCelsius(),TCPos);
        delay(490);   
        dsn.OneWireReadAllDevices();         // Read all devices
        dsn.sendData();  // Send data to server 
        Serial.println();
      }

}


