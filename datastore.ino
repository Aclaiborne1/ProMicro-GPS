// store and retrieve routines
void store(int z, int bytecounter)
// stores integer in two bytes of FRAM starting at bytecounter
{
  fram.begin();
  fram.write8(bytecounter, highByte(z));
  fram.write8(bytecounter + 1, lowByte(z));
}

int retrieve(int bytecounter)
// retrieves integer from two bytes of FRAM starting at bytecounter
{
  fram.begin();
  byte high = fram.read8(bytecounter);
  byte low = fram.read8(bytecounter + 1);
  int z = (high << 8) + low;
  return(z);
}

// returns time as float seconds from address
float retrieve_time(int address)
{
  float value;
  value = retrieve(address) / 100.0;
  return(value);
}

void radio_trans(char *trans_packet)
{
  rf95.send((uint8_t *)trans_packet, 30);
  rf95.waitPacketSent();
}

//Display new GPS info
boolean sendGPSInfo()
{
  char lat_str[12], lng_str[12];
  delay(500);
  while (myI2CGPS.available()) //available() returns the number of new bytes available from the GPS module
  {
    gps.encode(myI2CGPS.read()); //Feed the GPS parser
  }
  if (gps.location.isValid())
  { 
    dtostrf(gps.location.lat(), 12, 6, lat_str);
    sprintf(transmission, lat_str);
    dtostrf(gps.location.lng(), 12, 6, lng_str);
    sprintf(transmission + strlen(transmission), lng_str);
    rf95.send((uint8_t *)transmission, 30);
    rf95.waitPacketSent();
    return true;
  }
  else
  {
    sprintf(transmission, "(>0");
    rf95.send((uint8_t *)transmission, 30);
    rf95.waitPacketSent();
    return false;
  }
}

void sendFlightInfo(int feet, int gees)
{
  char time_str[6];
  float currenttime = float(millis() - starttime)/1000.0;
  dtostrf(currenttime, 4, 2, time_str);
  sprintf(transmission, "%s\t%d\t%d", time_str, feet, gees);
  rf95.send((uint8_t *)transmission, 30);
  rf95.waitPacketSent();  
}

void storeFlightInfo(int flightCounter, int feet, int gees)
{
  int dataindex = 2 * flightCounter;
  unsigned long currenttime = millis() - starttime;
  int sec100 = currenttime / 10;
  store(sec100, timestart + dataindex);
  store(feet, altstart + dataindex);
  store(gees, geesstart + dataindex);
}

void menu(int errno)
{
  char time_str[6];
  while (!Serial);
  Serial.begin(9600);
  verno();
  while (true)
  {
    Serial.print(F("Err.")); Serial.println(errno);
    Serial.print(F("Ret:"));
    int incomingByte = 0;
    while(Serial.available() <=0) {}
      incomingByte = Serial.read();
      
    int dataindex, i, alt, gees;
    float seconds;
    const float inittime = retrieve_time(timestart);
    const int datapoints = retrieve(flightCounterAdd);
    Serial.print(F("Datapoints = ")); Serial.println(datapoints);
    Serial.println();
    Serial.println(F("Time\t\Alt\t\gees"));
    for(i = 0; i < datapoints; i++)
    {
      dataindex = 2 * i;
      seconds = retrieve_time(timestart + dataindex) - inittime;
      dtostrf(seconds, 4, 2, time_str);
      alt = retrieve(altstart + dataindex);
      gees = retrieve(geesstart + dataindex);
      sprintf(transmission, "%s\t%d\t%d", time_str, alt, gees);
      Serial.println(transmission);
    }
    store(false, overwrite_protect); // allows overwrite to enable next flight
  }
}
