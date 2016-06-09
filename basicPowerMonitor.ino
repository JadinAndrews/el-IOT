  #include "ThingSpeak.h"
  #include <SPI.h>
  #include <EthernetV2_0.h>
  #include "EmonLib.h"
  
  EnergyMonitor circuit_1;
  EnergyMonitor circuit_2;
  EnergyMonitor circuit_3;

  EthernetClient client;  
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
  unsigned long myChannelNumber = xxxxx;
  const char * myWriteAPIKey = "xxxxxxxxxxxx";

void setup() {
  //Serial.begin(9600);
  //Serial.println("Energy Monitor Initializing");
  Ethernet.begin(mac);
  ThingSpeak.begin(client);
  circuit_1.current(1, 30);
  circuit_2.current(2, 30);
  circuit_3.current(3, 30);
  
}

void loop() {
  float circuit_1_Irms = circuit_1.calcIrms(1480) * 230 / 1000;
  float circuit_2_Irms = circuit_2.calcIrms(1480) * 230 / 1000;
  float circuit_3_Irms = circuit_3.calcIrms(1480) * 230 / 1000;
  ThingSpeak.setField(1, circuit_1_Irms);
  ThingSpeak.setField(2, circuit_2_Irms);
  ThingSpeak.setField(3, circuit_3_Irms);
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  //Serial.print("Power on circuit_1 : ");
  //Serial.println(circuit_1_Irms);
  //Serial.print("Power on circuit_2 : ");
  //Serial.println(circuit_2_Irms);
  //Serial.print("Power on circuit_3 : ");
  //Serial.println(circuit_3_Irms);
  //Serial.println();
  delay(20000);
}
