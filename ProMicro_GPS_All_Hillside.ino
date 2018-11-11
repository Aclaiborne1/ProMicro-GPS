/* Working program - compiles as Leonardo
** Both LEDs on as it initializes.  If Initialize fails, they both remain on.
** Goes to terminal menu if menu jumper off or init fail.  At menu, shows error number.
**
** Error number code:
**  0 - Menu select jumper is off or over-write protect true
**  1 - Barometer sensor failure
**  2 - Acclerometer failure
**  3 - Radio transmitter failure
**  4 - GPS receiver failure
**
** If any character is entered at menu prompt, any existing flight data is
** displayed and over-write protect is set to false.
**
** If initialize passes, green turns off and red remains on.
** Queries GPS until it locks onto satellite.  When it locks on, red LED turns
** off and green LED turns on.  This indicates it is ready for flight.
** Stores flight data in FRAM during entire flight, transmits flight data until apogee.  
** Transmits GPS data after apogee - this so location can be determined if line of sight
** lost prior to landing.
** 
** Landing is detected by comparing current height to height five records back.
** If height is not changing more than "landing" over five successive records, landing is 
** deemed to have occurred (assures landing detected even if landing in trees or on hill).  
**
** On landing, over-write protect is set to true and 
** no more data is recorded - GPS continues to be transmitted.
***********************************************************************************************

  Wiring for radio transmitter flight computer, using Pro-Micro
 
  For LEDs:
  PIN7 -- RED LED
  PIN8 -- GREEN LED
  
  For i2C:
  GND --- GND
  VCC --- VCC
  PIN2 -- SDA
  PIN3 -- SCL
  
  For Radio:
  GND --- GND
  5V --- VCC
  PIN15 - SCK
  PIN16 - MOSI
  PIN14 - MISO
  PIN19 - CS
  PIN18 - RST
  PIN0 -- GO (INTERRUPT)
***********************************************************************************************/

#include <Wire.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h> // for altimeter
#include <Adafruit_ADXL345_U.h> // for ADXL 345 accelerometer
#include <Adafruit_FRAM_I2C.h> // for FRAM

#include <SPI.h> // for communication to transmitter
#include <RH_RF95.h>  // for transmitter

#include <SparkFun_I2C_GPS_Arduino_Library.h> //for GPS
I2CGPS myI2CGPS; //Hook object to the library

#include <TinyGPS++.h> // GPS data parser
TinyGPSPlus gps; //Declare parsed gps object

/* local libraries */
#include "init.h"
#include "datastore.h"
#include "rocket.h"

/* create sensor specific objects */
Adafruit_BMP280 bmp = Adafruit_BMP280(); // for altimeter
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345); // for accelerometer
Adafruit_FRAM_I2C fram = Adafruit_FRAM_I2C(); // for FRAM

/* create general sensor and event objects for sensors */
sensor_t sensor;
sensors_event_t event;

/* LED Pins */
#define redLEDPin 7
#define greenLEDPin 8

/* radio pins for pro-micro*/
#define RFM95_CS 19 // radio chip select pin
#define RFM95_RST 18 // radio reset pin 
#define RFM95_INT 0 // radio interrupt pin for pro-micro, note is interrupt 2

/* radio specific definitions */
#define RFM95_IRQN 2  // pin 0 is IRQ2
#define RF95_FREQ 915.0 // set transmitter frequency

/* Singleton instance of the radio driver */
RH_RF95 rf95(RFM95_CS, RFM95_INT);

/* constants for flight status determination */
#define liftoff 15 // if we have reached 15 feet, we've launched
#define landing 2 // if we've launched but the altitude is not changing by two feet, we've landed

/* menu select pin */
#define menuPin 10

/* memory locations for data */
#define maxBytes 32000 // total memory space
#define flightCounterAdd 32002 // location to store number of records
#define overwrite_protect 32004 // location to protect against data overwrite
const int partition = maxBytes / 3; // for 3 variables
const int limit = partition / 2; // bytes for each variable
const int timestart = 0;
const int altstart = partition;
const int geesstart = 2 * partition;

/* global character buffer for radio */
char transmission[30];

unsigned long starttime;

void setup() 
{
  float temperature;
  int fahrenheit;
  
  pinMode(menuPin, INPUT);
  pinMode(redLEDPin, OUTPUT);
  pinMode(greenLEDPin, OUTPUT);
  
  LED(HIGH, HIGH);
  menuOn();
  initbaro();
  initaccel();
  initradio();
  initGPS();
  fram.begin();

  LED(HIGH, LOW);
  while(!sendGPSInfo()){}; // it may take a while for the GPS to lock onto satellite
  
  temperature = bmp.readTemperature();
  fahrenheit = (9.0 * temperature / 5.0) + 32.5;
  sprintf(transmission, "Launch temperature %d deg. F.", fahrenheit);
  radio_trans(transmission);
  LED(LOW, HIGH);
  starttime = millis();
}

void loop()
{
  int gees, feet, feet_old;
  
  static int groundlevel = getGroundlevel();
  static int landedCount = 0;
  static int downCount = 0;
  static boolean launched = false;
  static boolean peaked = false;
  static boolean landed = false;
  static int flightCounter = 0;
  
  if(!landed)
  {
    feet = getelev() - groundlevel;
    delay(8);
    gees = getgees();
    delay(8);
    if (!peaked)
      sendFlightInfo(feet, gees);
    if (peaked)
      sendGPSInfo();
    storeFlightInfo(flightCounter, feet, gees);
    flightCounter++;
    
    if (flightCounter == 20) // test for liftoff at datapoint 20
    {
      if (feet < liftoff) // if not above liftoff feet, not launched
      {
        launched = false; 
        flightCounter = 0; 
        starttime = millis();
      }
      if (feet >= liftoff)
      {
        launched = true;
        sprintf(transmission, "Launched");
        radio_trans(transmission);
      }
    }
    if (launched) // launched so test apogee, landing
    {
      
      if (!peaked) // apogee not reached yet, test for apogee
      {
        feet_old = retrieve(altstart + 2 * (flightCounter - 5)); // five records back
        if(feet <= feet_old) {downCount++;}
        if(feet > feet_old) {downCount = 0;}
        if(downCount == 3) // need three in a row
        {
          peaked = true;
          sprintf(transmission, "Apogee");
          radio_trans(transmission);
        }
      }
    
      if (peaked) // reached apogee, now test for landing, even in a tree
      {
        feet_old = retrieve(altstart + 2 * (flightCounter-5)); // five records back
        if(abs(feet-feet_old) <= landing) (landedCount++); // looking for altitude not changing much
        else (landedCount = 0);
        
        if (landedCount == 5) // need five in a row
        {
          landed = true; 
          store(flightCounter, flightCounterAdd);
          sprintf(transmission, "Landed");
          radio_trans(transmission);
          store(true, overwrite_protect);  // prevents overwrite of completed flight data
        }  
        if (landedCount < 5)
          landed = false;
      }
    }
  }
  
  if(landed)
  {
    sendGPSInfo();
  }
}

