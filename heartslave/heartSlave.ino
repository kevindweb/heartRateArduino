#include <Wire.h>
#include <TimeLib.h>

#define USE_ARDUINO_INTERRUPTS true
// Set-up low-level interrupts for most acurate BPM math
#include <PulseSensorPlayground.h>
// Includes the PulseSensorPlayground Library.

#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>

#define TIME_HEADER "T"
// Header tag for serial time sync message
#define TIME_REQUEST 7
// ASCII bell character requests a time sync message

int PulseWire = 0;
// Pulse Sensor PURPLE WIRE connected to ANALOG PIN 0

int LED13 = 13;
//  The on-board Arduion LED

int heartReading = 0;
// holds the incoming raw data. heartReading value can range from 0-1024

int Threshold = 550;
// Determine which heartReading to "count as a beat", and which to ingore.

int headingDegrees = 0;
// holds data coming from Triple-Axis magnetometer

int counter = 0;
String timStamp = "00:00";

PulseSensorPlayground pulseSensor;

/* Assign a unique ID to this sensor at the same time */
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);

void setup()
{
    Serial.begin(9600);

    Wire.begin(8);            // join i2c bus with address #8
    Wire.onRequest(sendData); // master wants user input

    pulseSensor.analogInput(PulseWire);
    pulseSensor.blinkOnPulse(LED13); //auto-magically blink Arduino's LED with heartbeat.
    pulseSensor.setThreshold(Threshold);
    if (pulseSensor.begin())
        Serial.println("We created a pulseSensor Object !"); //This prints one time at Arduino power-up,  or on Arduino reset.

    setSyncProvider(requestSync); //set function to call when sync required
    Serial.println("Waiting for sync message");

    /* Initialise the sensor */
    if (!mag.begin())
    {
        /* There was a problem detecting the HMC5883 ... check your connections */
        Serial.println("Ooops, no HMC5883 detected ... Check your wiring!");
        while (1)
            ;
    }
}

void loop()
{
    heartReading = pulseSensor.getBeatsPerMinute(); // Calls function on our pulseSensor object that returns BPM as an "int".

    if (counter % 50 == 0)
        // run clock every 1000ms
        checkClock();

    if (counter % 25 == 0)
        // get magnetometer data every 500ms
        getRotation();

    counter++;
    delay(20);
}

void checkClock()
{
    // process RTC logic
    if (Serial.available())
        processSyncMessage();

    if (timeStatus() != timeNotSet)
        digitalClockSet();
}

void getRotation()
{
    /* Get a new sensor event */
    sensors_event_t event;
    mag.getEvent(&event);

    // Hold the module so that Z is pointing 'up' and you can measure the heading with x&y
    // Calculate heading when the magnetometer is level, then correct for signs of axis.
    float heading = atan2(event.magnetic.y, event.magnetic.x);

    // Once you have your heading, you must then add your 'Declination Angle', which is the 'Error' of the magnetic field in your location.
    // Find yours here: http://www.magnetic-declination.com/
    // Mine is: -13* 2' W, which is ~13 Degrees, or (which we need) 0.22 radians
    // If you cannot find your Declination, comment out these two lines, your compass will be slightly off.
    float declinationAngle = 0.22;
    heading += declinationAngle;

    // Correct for when signs are reversed.
    if (heading < 0)
        heading += 2 * PI;

    // Check for wrap due to addition of declination.
    if (heading > 2 * PI)
        heading -= 2 * PI;

    // Convert radians to degrees for readability.
    headingDegrees = heading * 180 / M_PI;
}

void sendData(int num)
{
    // master arduino asked for the current status
    String data = "";
    //creates empty string

    if (heartReading < 100)
    {
        data.concat("0");
        //because the heart rate has to be three digits, we add one zero if the heart rate is only two

        if (heartReading < 10)
            data.concat("0");
        //if the heart rate is only a single digit, we add two zeroes first to make it three
    }
    data.concat(heartReading);
    data.concat(" ");

    data += timStamp; //adds timStamp string to data string.
    data.concat(" ");

    if (headingDegrees < 100)
    {
        // make sure we have 3 digits for degrees
        data.concat("0");

        if (headingDegrees < 10)
            data.concat("0");
    }
    data.concat(headingDegrees);

    char buffer[14];
    data.toCharArray(buffer, 14); 
    // places the string in a char array

    Serial.println(buffer);
    // sends char array to master for parsing
    Wire.write(buffer); 
}

void digitalClockSet()
{
    // digital clock display of the time
    timStamp = "";
    //create empty string
    timStamp.concat(hour());
    timStamp.concat(":");
    int minutes = minute();

    if (minutes < 10)
        timStamp.concat("0");
    //the minutes has to be two digits, so if it is naturally one digit, we add a zero first
    timStamp.concat(minutes);
}

void processSyncMessage()
{
    unsigned long pctime;
    const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

    if (Serial.find(TIME_HEADER))
    {
        pctime = Serial.parseInt();
        if (pctime >= DEFAULT_TIME)
        {
            // check the integer is a valid time (greater than Jan 1 2013)
            setTime(pctime);
            // Sync Arduino clock to the time received on the serial port
        }
    }
}

time_t requestSync()
{
    Serial.write(TIME_REQUEST);
    // the time will be sent later in response to serial mesg
    return 0;
}