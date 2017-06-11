/*
 * devstat.c
 *
 *  Created on: 9 Dec 2016
 *      Author: Christiaan
 */

#include "devstat.h"
#include "Arduino.h"

#if USEHTTPCLIENT
#include "ESP8266HTTPClient.h"
#endif

#if USEONEWIRE
#include "OneWire.h"
#endif

/*
 *  Create instance with a user id from https://devstat.net
 *  userID is 12 character long GUID, this is used to validate that the device is yours to post to.
 *  NB!! Keep the userID a secret
 */
devStat::devStat(String userID) {
	// User id must be 12 characters long as per https://devstat.net, Go get one :)
	if (userID.length() == 12) {
		_userID = userID;
	} else {
		_userID = "";
		if (_debug_level > 0)
			DEBUG_PRINT(
					"User id must be 12 characters long as per https://devstat.net, Go get one :)");
	}
}
;

/*
 * Add own device to _devArr[]
 * return position in aray
 */

byte devStat::addDevice(String deviceID)
{
	if(_deviceCounter < DEVSTATMAX)
	{
		_devArr[_deviceCounter].deviceID = deviceID;
		_deviceCounter++;
	}
	return (_deviceCounter -1);
}

/*
 * Pass data to array to smooth
 * Return avarage value
 */
float devStat::smoothData(float inVal, byte index) {

	// subtract the last reading:
	_devArr[index].total = _devArr[index].total
			- _devArr[index].readings[_devArr[index].readIndex];
	// Add new value to list
	_devArr[index].readings[_devArr[index].readIndex] = inVal;
	// add the reading to the total:
	_devArr[index].total = _devArr[index].total
			+ _devArr[index].readings[_devArr[index].readIndex];

	// advance to the next position in the array:
	_devArr[index].readIndex = _devArr[index].readIndex + 1;

	// if we're at the end of the array
	if (_devArr[index].readIndex >= DEVSTATSMOOTH) {
		// ...wrap around to the beginning:
		_devArr[index].readIndex = 0;
	}

	if (_devArr[index].readCounter < DEVSTATSMOOTH) {
		_devArr[index].readCounter++;
	}

	// calculate the average:
	_devArr[index].average = _devArr[index].total / _devArr[index].readCounter;

	if (_debug_level > 1) {
		DEBUG_PRINT("Current Reading : ");
		DEBUG_PRINT(inVal);
		DEBUG_PRINT("Average Reading : ");
		DEBUG_PRINT(_devArr[index].average);
	}
	return _devArr[index].average;
}

/*
 * Change default debug level
 * 0 nothing
 * 1 Warning/Errors
 * 2 Everything
 */
void devStat::setDebug(byte level) {
	_debug_level = level;
}

/*
 * Return JSON string to be send to the server
 */
String devStat::buildJSON() {
	String json;

	if (_debug_level > 1)
		DEBUG_PRINT("Building Json");

	// Only send data if userID is set
	if (_userID.length() != 12) {
		return ("Provide valid user id of 12 characters");
	}

	// Build json to send
	json = "{\"u\":\"" + _userID + "\""; // Open UserID

	// Add devices in list
	if (_deviceCounter > 0) {
		json += ",\"r\":["; // Open Readings
		for (byte i = 0; i < _deviceCounter; i++) {
			if (i > 0) {
				json += ","; // Continue with next group
			}
			json += "{\"d\":\"" + _devArr[i].deviceID + "\",\"v1\":\""
					+ _devArr[i].average + "\"}";
		}
		json += "]"; // Close readings
	}

	// Add events
	if (_eventCounter > 0) {
		if (_deviceCounter > 0) {
			json += ","; // Continue with next group
		}
		json += "\"e\":["; // Open events
		for (byte i = 0; i < _deviceCounter; i++) {
			if (i > 0) {
				json += ","; // Continue with next group
			}
			json += "{\"d\":\"" + _devArr[i].deviceID + "\",\"v1\":\""
					+ _devArr[i].average + "\"}";
		}
		json += "]"; // Close events

	}
	json += "}"; // Close userID

	return json;
}
;

/*
 *  Return number of devices currently in the array
 */
byte devStat::devicesInArray() {
	return (_deviceCounter);
}
;

/*
 * Find device in array and return current position
 * -1 = not found
 */
int devStat::idInArray(String inID) {
	int pos = -1;

	for (byte i = 0; i < _deviceCounter; i++) {
		if (_devArr[i].deviceID == inID) {
			return i;
		}
	}

	return (pos);
}

#if USEONEWIRE // Include OneWire protocall or not
/*
 *  Find devices on 1-wire bus
 */
byte devStat::OneWireFindDevices(OneWire* oneWire) {

	byte power = 0;
	byte addr[8]; // Device address
	byte devicesFound = 0; // Number of devices found this round
	byte devicePosition;

	if (_debug_level > 1)
		DEBUG_PRINT("Searching for devices on 1wire bus");

	// Look for new devices on bus
	while (oneWire->search(addr) && _deviceCounter < DEVSTATMAX) {

		// Check if address valid
		if (OneWire::crc8(addr, 7) != addr[7]) {
			if (_debug_level > 0)
				DEBUG_PRINT("CRC is not valid!");
		} else {
			// Check power mode of device
			oneWire->write(READPOWERSUPPLY);
			power = oneWire->read_bit();

			devicesFound++;
			// Check if new device to add to array
			String deviceID = OneWireDeviceAddressToString(addr);
			if (devicePosition = idInArray(deviceID) < 0) {
				//Insert device into array
				_devArr[_deviceCounter].deviceID = deviceID;
				memcpy(_devArr[_deviceCounter].addr, addr, 8);
				_devArr[_deviceCounter]._wire = oneWire;
				_devArr[_deviceCounter].power = power;
				_deviceCounter++;
			} else {
				// Update device parasite mode and _wire, just incase they moved or changed
				_devArr[devicePosition]._wire = oneWire;
				_devArr[devicePosition].power = power;
			}

			// Tell me about the devices found
			if (_debug_level > 1)
				DEBUG_PRINT(
						"Found - " + deviceID + " (power mode " + power + ")");
		}
	}
	oneWire->reset();

	DEBUG_PRINT("Number of devices found on 1wire bus " + devicesFound);

	return (devicesFound);
}
;

/*
 *  Return string value of device address to use as deviceID
 */
String devStat::OneWireDeviceAddressToString(byte addr[8]) {
	char dataString[2];
	String deviceID;

	for (byte i = 0; i < 8; i++) {
		sprintf(dataString, "%02X", addr[i]);
		deviceID = deviceID + dataString;
	}
	return (deviceID);
}

/*
 * Read from 1wire bus for all devices in array
 */
void devStat::OneWireReadAllDevices() {
	if (_debug_level > 1)
		DEBUG_PRINT("Reading data from all devices in _devArr[]");

	for (byte i = 0; i < _deviceCounter; i++) {
		if (_devArr[i].addr[0] != 0 && _devArr[i].addr[0] != 0) { // Check if we have a address since devArr can have other readings as well
			switch (_devArr[i].addr[0]) {
			case DS18S20MODEL:
			case DS18B20MODEL:
			case DS1822MODEL:
			case DS1825MODEL: {
				// All above is temperature devices
				if (_debug_level > 1)
					DEBUG_PRINT(_devArr[i].deviceID);
				float celcuis = OneWireReadTemperature(_devArr[i]._wire,
						_devArr[i].addr);
				smoothData(celcuis, i); // Pass data to smoother
				break;
			}
			default:
				if (_debug_level > 0)
					DEBUG_PRINT("Device family not catered for");
				break;
			}

		}
	}
}

/*
 * Read device on bus with address return
 */
float devStat::OneWireReadTemperature(OneWire* oneWire, byte addr[8]) {
	// - Copy from OneWire lib examples
	oneWire->reset();                    // Reset bus
	oneWire->select(addr);               // Select device at address
	oneWire->write(READSCRATCH);         // Read Scratchpad

	byte data[12]; // Device data
	for (byte i = 0; i < 9; i++) {           // we need 9 bytes
		data[i] = oneWire->read();
	}

	// Convert to Celsius
	// Convert the data to actual temperature
	// because the result is a 16 bit signed integer, it should
	// be stored to an "int16_t" type, which is always 16 bits
	// even when compiled on a 32 bit processor.
	int16_t raw = (data[1] << 8) | data[0];
	byte type_s;
	(DS18S20MODEL == addr[0]) ? type_s = 1 : type_s = 0;
	if (type_s) {
		raw = raw << 3; // 9 bit resolution default
		if (data[7] == 0x10) {
			// "count remain" gives full 12 bit resolution
			raw = (raw & 0xFFF0) + 12 - data[6];
		}
	} else {
		byte cfg = (data[4] & 0x60);
		// at lower res, the low bits are undefined, so let's zero them
		if (cfg == 0x00)
			raw = raw & ~7;  // 9 bit resolution, 93.75 ms
		else if (cfg == 0x20)
			raw = raw & ~3; // 10 bit res, 187.5 ms
		else if (cfg == 0x40)
			raw = raw & ~1; // 11 bit res, 375 ms
		//// default is 12 bit resolution, 750 ms conversion time
	}

	float celsius = (float) raw / 16.0;
	oneWire->reset();
	return (celsius);
}

void devStat::OneWireIssueCopyToScratchAll() {
	if (_debug_level > 1)
		DEBUG_PRINT("Issue copy to scratch for all devices in _devArr[]");

	for (byte i = 0; i < _deviceCounter; i++) {
		if (_devArr[i].addr[0] != 0 && _devArr[i].addr[0] != 0) {
			OneWireIssueCopyToScratch(_devArr[i]._wire, _devArr[i].addr,
					_devArr[i].power);
		}
	}
}

/*
 * Issue copy from memory to device scratch command
 * if not powerd wait for convertion
 */
void devStat::OneWireIssueCopyToScratch(OneWire* oneWire, byte addr[8],
		byte power) {

	oneWire->reset();                    // Reset bus
	oneWire->select(addr);               // Select device at address
	oneWire->write(STARTCONVO);         // Read Scratchpad
	if (power == 0) {
		delay(750);
	}
	oneWire->reset();                    // Reset bus
}

#endif

#if USEHTTPCLIENT // Include HTTP client or not
/*
 * Send data to devstat.net
 */
String devStat::sendData(){
	return (sendData(true));
}

String devStat::sendData(bool secure ) {

	HTTPClient http;

	// Connect to server
	if (secure) {
		http.begin(DEVSTATURLHTTPS, FINGERPRINT);
	} else {
		http.begin(DEVSTATURLHTTP);
	}

	// Post data
	if (_debug_level > 1)
		DEBUG_PRINT("Sending data to devstat.net");
	int httpCode = http.POST(buildJSON());

	// Display return
	if (httpCode == HTTP_CODE_OK) {
		String payload = http.getString();
		DEBUG_PRINT(payload);
	} else {
		DEBUG_PRINT("FAILED. error:");
		DEBUG_PRINT(http.errorToString(httpCode).c_str());
		DEBUG_PRINT("body:");
		DEBUG_PRINT(http.getString());
	}

	return ("Data posted");

// Close connection
	http.end();
}
;

#endif
