#ifndef datastoreH
#define datastoreH

void store(int z, int bytecounter);
int retrieve(int bytecounter);
float retrieve_time(int address);
boolean sendGPSInfo();
void sendFlightInfo(int feet, int gees);
void storeFlightInfo(int flightCounter, int feet, int gees);
void menu();

#endif




