#include <OneWire.h>
#include <devstat.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define USERNAME "Your user on devstat "
#define SID "wifi sid"
#define PASS "wifi password "

OneWire ds(D1);
ESP8266WiFiMulti WiFiMulti;
devStat dsn(USERNAME); // User id 

#define DHTTYPE           DHT22     // DHT 22 (AM2302)
#define DHTPIN           D2         // Pin which is connected to the DHT sensor.

byte T1Pos;
byte M1Pos;

DHT_Unified dht(DHTPIN, DHTTYPE);

void setup() {
    Serial.begin(115200); // Set serial communication speed 
    dht.begin();
    delay(500); // Wait for everything to settle 
    pinMode(LED_BUILTIN, OUTPUT); // Blink light   
    WiFiMulti.addAP(SID, PASS);
    Serial.println("Connecting Wifi...");
    
    dsn.setDebug(2); // Display everything 
    dsn.OneWireFindDevices(&ds);  // Search for devices 
    T1Pos = dsn.addDevice("585CF3333B9D"); // Outside temp
    M1Pos = dsn.addDevice("585CF3B97554");// Moisture 
}

void loop() {

      if((WiFiMulti.run() == WL_CONNECTED)) {
        dsn.OneWireIssueCopyToScratchAll(); // Update scratch from memory
        digitalWrite(LED_BUILTIN, HIGH); 
        delay(450);                       // wait for a second
        digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
        
       // Get temperature event and print its value.
  sensors_event_t event;  
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println("Error reading temperature!");
  }
  else {
    dsn.smoothData(event.temperature,T1Pos);
  }
  
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println("Error reading humidity!");
  }
  else {
    dsn.smoothData(event.relative_humidity,M1Pos);    
  }
        delay(450);   
        dsn.OneWireReadAllDevices();         // Read all devices
        dsn.sendData();  // Send data to server 
        Serial.println();
      }

}


