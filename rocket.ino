// returns elevation above sea level in feet as integer
// could improve by entering curent SLP instead of constand 1013.25
int getelev()
{
  int xfeet;
  float Altitude =  bmp.readAltitude(1013.25); // get altitude above sea level in meters
  xfeet = (Altitude * 3.2808) + 0.5; // convert to integer feet
  return xfeet;
}

// returns ground level in feet above sea level as integer
// could improve by taking multiple readings and averaging
int getGroundlevel()
{
  return getelev();
}
    
//returns integer that is 100 X actual gees - note x axis is along rocket axis
int getgees()
{
  float xaxis;
  int xgees;
  accel.getSensor(&sensor); // attaching the accelerometer
  accel.getEvent(&event);

  xaxis = event.acceleration.x; // meters per second squared
  xgees = 100.0 * (xaxis / 9.81) + 0.5; // 9.81 meters per second squared is one gee - returning 100 * gees
  
  return(xgees);
}

void LED(int redvalue, int greenvalue)
{
  digitalWrite(redLEDPin, redvalue);
  digitalWrite(greenLEDPin, greenvalue);
}

void verno()
{
   Serial.println(F("Program: 'ProMicro_GPS_All_2_Hillside' - last updated 10-18-18"));
}


