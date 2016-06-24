  /**
   * Power Monitor sketch, this monitors the energy usage on analog ports using
   * the emonlib. It updates a phant server roughly every 4s with the data. It also monitors errors
   * and in conjunction with a watchdog timer, will attempt a reset if the amount of errors that occur
   * exceed a certain value. This is VERY useful. The energy monitor will try to keep itself running without
   * locking up, and if it does, the watchdog will take care of that.
   * 
   * If you are using the standard ethernet controller, include the correct library, I am using the w5200.
   * The code includes a 'hack' way of resetting the ethernet controller, with the reset pin attached to A5.
   *
   * Author: Jadin Andrews
   * jadinandrews@gmail.com
   */
  
  #include <Phant.h>
  #include <SPI.h>
  #include <EthernetV2_0.h>
  #include "EmonLib.h"
  #include <avr/wdt.h>
  
  EnergyMonitor circuit_1;
  EnergyMonitor circuit_2;
  EnergyMonitor circuit_3;

  EthernetClient client;  

  // The following variables need to be set.
  IPAddress ip(192, 168, 0, 5);
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
  const char * publicKey = "xxxxxxxxxxxxxxxxxxxxxxxxxxx";
  const char * privateKey = "xxxxxxxxxxxxxxxxxxxxxxxxxxx";
  const char server[] = "phant.io";

  //Construct the Phant object.
  Phant data(server, publicKey, privateKey);

  // keeps track of current errors. 
  int errors;
  int locked_up = 0;
  int max_errors = 8;

void setup() {
  locked_up = 0;
  erorrs = 0;

  /**
   * setup the watchdog timer, it runs every 8s and checks that there aren't too many errors.
   * if too many errors occur, initialize a soft reset.
   */
  setupWDT();
  
  //Disable the SD card:
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);

  //reset ethernet at power on to avoid lock up.
  resetEthernet(A5);
  delay(500);
  
  Serial.begin(9600);
  Serial.println("Energy Monitor Initializing");
  
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    //Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);

  //Construct energy monitor objects
  circuit_1.current(1, 30);
  circuit_2.current(2, 30);
  circuit_3.current(3, 30);
  
}

void loop() {
  // Set to zero to indicate to the WDT that the program has not locked up.
  locked_up = 0;
  
  Serial.print("Errors: ");
  Serial.println(errors);
  
  // Get readings and convert to kW.
  float circuit_1_Irms = circuit_1.calcIrms(2000) * 230 / 1000;
  float circuit_2_Irms = circuit_2.calcIrms(2000) * 230 / 1000;
  float circuit_3_Irms = circuit_3.calcIrms(2000) * 230 / 1000;

  // Add readings to POST field constructor and filter out unwanted noise range.
  data.add("kw1", rejectLowValues(circuit_1_Irms, 0.03));
  data.add("kw2", rejectLowValues(circuit_2_Irms, 0.03));
  data.add("kw3", rejectLowValues(circuit_3_Irms, 0.03));

    if (client.connect(server, 8080)) {
      // Make a HTTP request:
      client.println(data.post());
  
      // Wait for response
      int count = 0;
      while (!client.available()){
        delay(10);
        count++;
        if (count >= 500){
          //Response took too long
          errors++;
          break;
        }
      }
      //Serial.print("Response took: "); 
      //Serial.print(count*10);
      //Serial.println(" milliseconds");
  
      // Read in response, need to add in a response parser to check server response.
      while (client.available()) {
      break; // Actually don't need the response just yet, break out for now.
      //char c = client.read();
      //Serial.print(c);
    }
  }
  else 
  {
    // if you didn't get a connection to the server:
    errors++;
  }
  
  client.stop();
  delay(4000);
}

/**
 * resetEthernet reset the ethernet controller.
 * @param pin is to be directly connected to the reset pin on the ethernet shield.
 */
void resetEthernet(int pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delay(500);
  digitalWrite(pin, HIGH);
}

/**
 * rejectLowValues is one serious hack to remove unwanted noise.
 * @param value is the value to 'clean'
 * @param cutOff is the value of the filter
 * @returns a float that is superficially cleaner.
 */
float rejectLowValues(float value, float cutOff) {
  if (value > cutOff)
  {
    return value;
  }
  else
  {
    return 0.0;
  }
}

/**
 * resetFunc is a software reset function. It does not properly reset, it just returns the cpu to the beginning of the 
 * program. ie: goto 0.
 */
void(* resetFunc)(void) = 0;

/**
 * setupWDT sets up the watchdog timer bits and scaler to execute every 8s.
 */
void setupWDT()
{
  // Clear the reset flag.
  MCUSR &= ~(1<<WDRF);
  
  // In order to change WDE or the prescaler, we need to
  // set WDCE (This will allow updates for 4 clock cycles).
  WDTCSR |= (1<<WDCE) | (1<<WDE);

  // Set new watchdog timeout prescaler value
  // 8.0 seconds
  WDTCSR = 1<<WDP0 | 1<<WDP3;
  
  // Enable the WD interrupt.
  WDTCSR |= _BV(WDIE);
}

/**
  * The WDT_vect routine is called by the WDT interrupt.
  * The Arduino will be reset once a day in case of lock ups.
  *
  */
ISR(WDT_vect)
{
  locked_up++;
   if (errors >= max_errors || locked_up >= max_errors)
   {
     resetFunc();
   }
}
