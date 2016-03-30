#include "SparkFunBME280/SparkFunBME280.h"
BME280 mySensorA;
float tempF;
float pressure;
float RH;

#include "SparkFunMAX17043/SparkFunMAX17043.h"
// MAX17043 battery manager IC settings
float batteryVoltage;
float batterySOC;
bool batteryAlert;

#include "ThingSpeak/ThingSpeak.h"
//################### update these vars ###################
unsigned long myChannelNumber = your channel number here;  //e.g. 101992
const char * myWriteAPIKey = "your api key here"; // write key here, e.g. ZQV7CRQ8PLKO5QXF
//################### update these vars ###################
TCPClient client;
unsigned long lastMeasureTime = 0;
unsigned long measureInterval = 60000; // can send data to thingspeak every 15s, but give the matlab analysis a chance to add data too

// ultrasonic distance sensor for water height measurement
float uSperInch = 147; // from datasheet
float distance;
unsigned long duration;
float waterHeight;
//################### update these vars ###################
float totalDistance = 64; // the distance from the sensor to the bottom of the water tank
//################### update these vars ###################

// photocell
float lightIntensity;

// connection settings
STARTUP(WiFi.selectAntenna(ANT_EXTERNAL)); // use the u.FL antenna, get rid of this if not using an antenna
float batterySOCmin = 40.0; // minimum battery state of charge needed for short wakeup time
unsigned long wakeUpTimeoutShort = 300; // wake up every 5 mins when battery SOC > batterySOCmin
unsigned long wakeUpTimeoutLong = 900; // wake up every 15 mins during long sleep, when battery is lower
unsigned long connectedTime; // millis() at the time we actually get connected, used to see how long it takes to connect
unsigned long connectionTime; // difference between connectedTime and startTime

// for updating software
bool waitForUpdate = false; // for updating software
unsigned long updateTimeout = 600000; // 10 min timeout for waiting for software update
unsigned long communicationTimeout = 120000; // wait 2 mins before sleeping
unsigned long bootupStartTime;

// for publish and subscribe events
//################### update these vars ###################
String eventPrefix = "your prefix"; // e.g. myFarm/waterSystem
//################### update these vars ###################

bool pumpOn;

void setup() {
    // Set up the MAX17043 LiPo fuel gauge:
	lipo.begin(); // Initialize the MAX17043 LiPo fuel gauge

	// Quick start restarts the MAX17043 in hopes of getting a more accurate
	// guess for the SOC.
	lipo.quickStart();

	// We can set an interrupt to alert when the battery SoC gets too low.
	// We can alert at anywhere between 1% - 32%:
	lipo.setThreshold(20); // Set alert threshold to 20%.
	// use this to measure how long it takes to connect the Photon to the internet if you're in spotty wifi coverage
    pinMode(A0, INPUT); // ultrasonic distance sensor
    
    // set up BME280 sensor
	mySensorA.settings.commInterface = I2C_MODE;
	mySensorA.settings.I2CAddress = 0x77;
	mySensorA.settings.runMode = 3; //  3, Normal mode
	mySensorA.settings.tStandby = 0; //  0, 0.5ms
	mySensorA.settings.filter = 0; //  0, filter off
	//tempOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
	mySensorA.settings.tempOverSample = 1;
	//pressOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
    mySensorA.settings.pressOverSample = 1;
	//humidOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
	mySensorA.settings.humidOverSample = 1;
	mySensorA.begin();
    
    ThingSpeak.begin(client);
    
	Particle.subscribe(eventPrefix + "/waterTankSensor/update", eventHandler); // MY_DEVICES doesn't allow publishes from the cloud API
	Particle.subscribe(eventPrefix + "/waterTankPump/pumpOn", eventHandler, MY_DEVICES);
    Particle.publish(eventPrefix + "/waterTankSensor/online", "true"); // subscribe to this with the API like: curl https://api.particle.io/v1/devices/events/temp?access_token=1234
    bootupStartTime = millis();
    doTelemetry(); // always take the measurements at least once
}

void loop() {
    if (waitForUpdate || millis() - bootupStartTime > communicationTimeout || batterySOC > 75.0 || pumpOn) {
        // The Photon will stay on unless the battery is less than 75% full, or if the pump is running.
        // If the battery is low, it will stay on if we've told it we want to update the firmware, until that times out (updateTimeout)
        // It will stay on no matter what for a time we set, stored in the variable communicationTimeout
        if (millis() - lastMeasureTime > measureInterval) {
            doTelemetry();
        }
        	if ((millis() - bootupStartTime) > updateTimeout) {
        	    waitForUpdate = false;
        	}
    } else {
            if (batterySOC < batterySOCmin) {
                System.sleep(SLEEP_MODE_DEEP, wakeUpTimeoutLong);
            } else {
                System.sleep(SLEEP_MODE_DEEP, wakeUpTimeoutShort);
            }
    }
}

void eventHandler(String event, String data)
{
  // to publish update: curl https://api.particle.io/v1/devices/events -d "name=update" -d "data=true" -d "private=true" -d "ttl=60" -d access_token=1234
  if (event == eventPrefix + "/waterTankSensor/update") {
      (data == "true") ? waitForUpdate = true : waitForUpdate = false;
      if (waitForUpdate) {
        Serial.println("wating for update");
        Particle.publish(eventPrefix + "/waterTankSensor/updateConfirm", "waiting for update");
      } else {
        Serial.println("not wating for update");
        Particle.publish(eventPrefix + "/waterTankSensor/updateConfirm", "not waiting for update");
      }
  } else if (event == eventPrefix + "/waterTankPump/pumpOn") {
      (data == "true") ? pumpOn = true : pumpOn = false;
  }
  Serial.print(event);
  Serial.print(", data: ");
  Serial.println(data);
}

void doTelemetry() {
    // let the pump controller know we're still here
    Particle.publish(eventPrefix + "/waterTankSensor/online", "true");
    
    // water height
    duration = pulseIn(A0, HIGH);
    distance = duration / uSperInch; // in inches
    waterHeight = totalDistance - distance;
    ThingSpeak.setField(1, waterHeight);
	
	Particle.publish(eventPrefix + "/waterTankSensor/waterHeight", String(waterHeight));
    
    // BME280
    pressure = mySensorA.readFloatPressure()*29.529983/100000.0;
    ThingSpeak.setField(2, pressure);
    tempF = mySensorA.readTempF();
    ThingSpeak.setField(4, tempF);
    RH = mySensorA.readFloatHumidity();
    ThingSpeak.setField(5, RH);
    
    // photocell
    lightIntensity = analogRead(A1);
    ThingSpeak.setField(6, lightIntensity);
    
    // read battery states
    batteryVoltage = lipo.getVoltage();
    ThingSpeak.setField(7, batteryVoltage);
    // lipo.getSOC() returns the estimated state of charge (e.g. 79%)
    batterySOC = lipo.getSOC();
    ThingSpeak.setField(8, batterySOC);
    // lipo.getAlert() returns a 0 or 1 (0=alert not triggered)
    //batteryAlert = lipo.getAlert();
    
    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey); 
    lastMeasureTime = millis();
}