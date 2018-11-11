// menu select, menu(0) if jumper not set or unretrieved data
boolean menuOn()
{
  if (!digitalRead(menuPin) || retrieve(overwrite_protect))
    menu(0);
  else
    return 0;
 }

// initialize barometer, menu(1) if test fails
boolean initbaro()
{
  if(!bmp.begin())
    menu(1);
  else
    return 0;
} 

// initialize accelerometer, menu(2) if test fails, sets to +- 16G if good
boolean initaccel()
{
  uint8_t i;
  accel.getSensor(&sensor); // attaching the accelerometer
  
  if (!accel.begin())
    menu(2);
  else
  {
    accel.setRange(ADXL345_RANGE_16_G); // accelerometer okay, so set range of 16G for accelerometer
    return 0;
  }
}

// initialize radio transmitter, menu(3) if test fails
boolean initradio()
{
  digitalWrite(RFM95_RST, HIGH);
  delay(100);

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(50);
  digitalWrite(RFM95_RST, HIGH);
  delay(50);
 
  if (!rf95.init())
    menu(3);
  else
  {
    rf95.setFrequency(RF95_FREQ);
    rf95.setTxPower(23, false); // maximum transmitter power
    return 0;
  }
}

// initialize GPS receiver, menu(4) if test fails
boolean initGPS()
{
  if (!myI2CGPS.begin())
    menu(4);
  else
  {
//    myI2CGPS.sendMTKpacket(F("PMTK352,1*2B")); // to correct for Japanese satellite QZSS
    return(0);
  }
}
  


