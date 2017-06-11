/*
 * devstat.h
 *
 *  Created on: 9 Dec 2016
 *      Author: Christiaan
 */

#ifndef DEVSTAT_H_
#define DEVSTAT_H_

#define DEVSTATLIBVERSION "1.0.0"

// Put this before #include <devstat.h> to exclude parts of code
#ifndef USEONEWIRE
#define USEONEWIRE true // Set to false if you do not want to use OneWire protocall or provide your own
#endif

#ifndef USEHTTPCLIENT
#define USEHTTPCLIENT true // Set to false if you do not want to use HTTP Client or provide your own
#endif

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL  0
#endif

#ifndef DEBUG_PRINT
#define DEBUG_PRINT(x) Serial.println(x)
#endif

/*
 *  Copy from  DALLASTEMPLIBVERSION "3.7.3"
 */
// Model IDs
#define DS18S20MODEL 0x10  // also DS1820
#define DS18B20MODEL 0x28
#define DS1822MODEL  0x22
#define DS1825MODEL  0x3B

// OneWire commands
#define STARTCONVO      0x44  // Tells device to take a temperature reading and put it on the scratchpad
#define COPYSCRATCH     0x48  // Copy EEPROM
#define READSCRATCH     0xBE  // Read EEPROM
#define WRITESCRATCH    0x4E  // Write to EEPROM
#define RECALLSCRATCH   0xB8  // Reload from last known
#define READPOWERSUPPLY 0xB4  // Determine if device needs parasite power
#define ALARMSEARCH     0xEC  // Query bus for devices with an alarm condition

// End copy from DALLASTEMPLIBVERSION "3.7.3"

// Maximum devices to track
#ifndef DEVSTATMAX
#define DEVSTATMAX 15
#endif

// Number of readings used to smooth data
#ifndef DEVSTATSMOOTH
#define DEVSTATSMOOTH 15
#endif

// Server URL to post to
#define DEVSTATURLHTTPS "https://upload.devstat.net/dataIn.php"
#define DEVSTATURLHTTP "https://upload.devstat.net/dataIn.php"
#define FINGERPRINT "89 7a 08 c0 16 b7 9f 16 ac 8d 35 ec 3d af a3 e3 35 11 b5 5b"

//Include Libraries
#include "Arduino.h"

#if USEHTTPCLIENT
#include "ESP8266HTTPClient.h"
#endif

#if USEONEWIRE
#include "OneWire.h"
#endif

// Structures used
struct _DEVSTAT_ {
	float readings[DEVSTATSMOOTH];    // Device readings
	int readIndex = 0;                // the index of the current reading
	float total = 0;                  // the running total
	float average = 0;                // the average
	byte readCounter = 0;             // Reading in array
	String deviceID;
#if USEONEWIRE
	OneWire* _wire;					  // OneWire reference
	byte addr[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // Device Address
	byte power; // When in parasite mode wait for the data (use not parasite for heigher poll frequencies )
#endif
};

class devStat {
public:
	devStat(String userID); 		 // UserID from devstat.net NB!! Keep secret
	String buildJSON();     		 // Build JSON sting to send to server and return
	byte devicesInArray();           // Return Devices currently in the array
	void setDebug(byte level);       // Set debug level
	float smoothData(float inval, byte index); // Pass value to array to smooth, return average
#if USEHTTPCLIENT
	String sendData(bool secure ); // Send data to devstat.net
	String sendData(); 					 // Send data to devstat.net
	byte addDevice(String deviceID);    // Add own device to array
#endif
#if USEONEWIRE
	byte OneWireFindDevices(OneWire*); 			// Find 1Wire devices on the bus
	String OneWireDeviceAddressToString(byte addr[8]); // Convert 1wire address to a device ID
	float OneWireReadTemperature(OneWire*, byte addr[8]); // Read device Temperature in celcuis at address on bus
	void OneWireReadAllDevices(); // Read all devices on the bus stored in _devArr[]
	void OneWireIssueCopyToScratch(OneWire*, byte addr[8],byte power = 0); // Issue copy reading to scratch from memory for device on bus with address
	void OneWireIssueCopyToScratchAll(); // Issue copy to scratch for alld devices in _devArr[]
#endif
private:
	String _userID;
	_DEVSTAT_ _devArr[DEVSTATMAX];
	byte _deviceCounter = 0;    // Number of devices found on channel / in array
	byte _eventCounter = 0;     // Event counter
	int idInArray(String inID); // Find ID in array and return position
	byte _debug_level = DEBUG_LEVEL;          // 0 noting, 1 errors , 2 all

};

#endif /* DEVSTAT_H_ */
