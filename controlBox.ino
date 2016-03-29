#include "ThingSpeak/ThingSpeak.h"
// channel we're writing to
//################### update these vars ###################
unsigned long myWriteChannelNumber = your channel number; // e.g 101223
const char * myWriteAPIKey = "your write API key for the pump controller channel";
// channel we're reading water height from
unsigned long myReadChannelNumber = 101982; // e.g 101223
const char * myReadAPIKey = "your read API key for the telemetry channel";
//################### update these vars ###################
TCPClient client;
unsigned long lastMeasureTime = 0;
unsigned long measureInterval = 60000; // can send data to thingspeak every 15s, but once a minute is fine

bool pumpOn = false;
float waterHeight = 1000; // we want to make sure the relay isn't falsely triggered on from the get-go
//################### update these vars ###################
float lowerCutoff = 20; // lowest acceptable water height, in inches
float higherCutoff = 42; // highest acceptable water height, in inches
float totalDistance = 64; // the distance from the sensor to the bottom of the water tank
//################### update these vars ###################
int success;
unsigned long relayStartTime;
unsigned long lastSignal = millis();
unsigned long pumpTimeout = 15*60*1000; // turn off the pump if haven't heard from sensor in 15 mins
unsigned long pumpOnTime = 15*60*1000; // pump run time in manual mode

// PIR motion sensor
int relayPin = 0;
int PIRpin = 7;
int PIRval;

// for publish and subscribe events
//################### update these vars ###################
String eventPrefix = "myFarm/waterSystem"; // e.g. myFarm/waterSystem
//################### update these vars ###################

void setup() {
	pinMode(relayPin, OUTPUT);
	pinMode(PIRpin, INPUT);
	digitalWrite(relayPin, LOW);
	
	Particle.subscribe(eventPrefix, eventHandler, MY_DEVICES);

	ThingSpeak.begin(client);
}

void loop() {
	autoPumpControl();
	checkPIR();
	recordThingSpeakData();
}

int relayControl(String relayState)
{
	if (relayState == "on") {
		pumpOn = true;
		Particle.publish(eventPrefix + "/waterTankPump/pumpOn", boolToText(pumpOn));
		digitalWrite(relayPin, HIGH);
		relayStartTime = millis();
		ThingSpeak.setField(1, 1); // our "pump on" field
		return 1;
	}
	else if (relayState == "off") {
		pumpOn = false;
		Particle.publish(eventPrefix + "/waterTankPump/pumpOn", boolToText(pumpOn));
		digitalWrite(relayPin, LOW);
		ThingSpeak.setField(1, 0); // our "pump on" field
		return 1;
	}
	else {
		return 0;
	}
}

void autoPumpControl() {
	if (pumpOn) {
		if (millis() - lastSignal > pumpTimeout) { // if we haven't heard from the water tanks in a while, turn off the pump
			relayControl("off");
		}
	}
	if (waterHeight < lowerCutoff) {
		success = relayControl("on");
	} else if (waterHeight > higherCutoff) {
		success = relayControl("off");
	} else {
		ThingSpeak.setField(1, boolToNum(pumpOn)); // our "pump on" field
		Particle.publish(eventPrefix + "/waterTankPump/pumpOn", boolToText(pumpOn));
	}
}

void checkPIR() {
	PIRval = digitalRead(PIRpin);
	if(PIRval == LOW){
		ThingSpeak.setField(2, 1); // 1 = motion detected, 0 = no motion
	}
}

void recordThingSpeakData() {
	if (millis() - lastMeasureTime > measureInterval) {
		ThingSpeak.writeFields(myWriteChannelNumber, myWriteAPIKey);
		ThingSpeak.setField(2,0); // reset PIR motion sensor field to 'no motion detected'
		lastMeasureTime = millis();
	}
}

String boolToText(bool thing)
{
	String result;
	thing ? result = "true" : result = "false";
	return result;
}

int boolToNum(bool thing)
{
	int result;
	thing ? result = 1 : result = 0;
	return result;
}

void eventHandler(String event, String data)
{
  if (event == eventPrefix + "/waterTankSensor/online") {
	  Particle.publish(eventPrefix + "/waterTankPump/pumpOn", boolToText(pumpOn));
  } else if (event == eventPrefix + "/waterTankSensor/online") {
	  (data == "true") ? lastSignal = millis() : Serial.println(data);
  } else if (event == eventPrefix + "/waterTankSensor/waterHeight") {
      waterHeight = data.toFloat();
  }
}