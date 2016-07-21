/**
   apolloClient is a sketch that downloads another sketch from a webserver and loads
   it onto the arduino.

   The sketch should have been created with the apollo/mkfirmware tool. The sketch is
   downloaded onto an sdcard, and integrity is checked against a few 'prepended'
   bytes. The first two bytes of the binary represent the size of the file excluding
   the prepended bytes. The following two bytes represent the checksum of the remainder
   of the file, which is computed using a fletcher 16 algorithm.

   Once a binary file has been succesfully downloaded the arduino will issue a reset
   call, which will allow the bootloader to load the new firmware.

   This sketch works fine on it's own, but there's no point in it unless the arduino
   it is running on has a modified bootloader.

   Author: Jadin Roste Andrews
   Email: jadinandrews@gmail.com

   TODO: This is like the first working version, with most features in place, lots can still be improved.
   TODO: Need a way to store the downloaded file in a temporary file, and then rename it to FIRMWARE.BIN
         only if the download passes the checks. I don't think this is an easy task with the fat16 lib,
         but maybe it won't be too bad to simply copy a temp file into FIRMWARE.BIN?

   NOTE: This sketch relies on the Seedstudio ethernet shieal v2.0, with an integrated SD card slot.
         For other cobinations, just make sure your cs pins don't conflict, otherwise everything should
         work just fine.

 * */


#include <SPI.h>
#include <EthernetV2_0.h>
#include <Fat16.h>
#include <avr/wdt.h>

SdCard card;
const int CHIP_SELECT = 4;
const char updateServer[] = "www.debcal.co.za";
const char frimwareRequest[] = "GET /apollo/FIRMWARE.BIN HTTP/1.1";
// How often to check for updates in minutes.
int updateInterval = 1;
bool updateNow = false;
int updateCounter = 0;

int temp = 0;

EthernetClient client;
IPAddress ip(10, 0, 0, 24);
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

/**
   ElCheapo reset function.
*/
void(* resetFunc)(void) = 0;


char byteStream() {
  //Serial.println("byteStream called");
  char a = client.read();
  Serial.print(a);
  return a;

}

char crnl[] = {'\r', '\n', '\r', '\n'};
// Counts the amount of mathching bytes found in the stream.
// It can not be local to checkBytes
int matchingBytes = 0;

bool checkBytes(char* bytes, char (*stream)()) {
  int byteCount = 0;
  while (matchingBytes < 4) {
    matchingBytes = bytes[matchingBytes] == stream() ? matchingBytes + 1 : 0;
    byteCount++;
    if (byteCount >= 2048) {
      return false;
    }
    delay(1);
  }
  return (matchingBytes == 4); // a bit pedantic
}


int twoBytes() {
  int tempVal = 0;
  byte one = client.read();
  byte two = client.read();
  tempVal = one << 8;
  tempVal |= two;
  return tempVal;
}



void setup() {
  resetEthernet(A5);
  Serial.begin(9600);
  delay(500);
  Serial.println(F("Started"));
  setupWDT();
  // put your setup code here, to run once:

}

void loop() {
  //Serial.println(F("THIS PROGRAM HAS BEEN UPDATED!!!!"));
  // Leave this here
  apolloUpdate();

  if (temp < updateCounter) {
    Serial.println(updateCounter * 8);
    temp = updateCounter;
  }

}


void apolloUpdate() {
  if (updateNow) {
    resetEthernet(A5);
    delay(2000);
    Serial.println(F("apollo update Initialized"));

    // Init SD Card
    // initialize the SD card
    card.begin(CHIP_SELECT, SPI_HALF_SPEED);

    // initialize a FAT16 volume
    Fat16::init(&card);


    // create a new file
    Fat16 file;
    const char name[] = "FIRMWARE.BIN";
    file.open(name, O_CREAT | O_WRITE);

    Serial.println(F("Getting new Firmware file"));
    if (client.connect(updateServer, 80)) {  //starts client connection, checks for connection
      Serial.println(F("Connected to Update Server"));
      client.println(F("GET /apollo/FIRMWARE.BIN HTTP/1.1")); //download text
      client.println(F("Host: www.debcal.co.za"));
      client.println(F("Connection: close"));  //close 1.1 persistent connection
      client.println(); //end of get request

      // Wait for http reponse and remove header
      Serial.println(F("waiting for data"));
      while (client.connected()) {
        while (client.available()) {
          if (checkBytes(crnl, byteStream)) {
            goto end;
          }
        }
      }
end:

      Serial.println(F("done waiting for data"));
      Serial.println();
      Serial.print(F("Prepended Value = "));

      int prependedSize = twoBytes();
      unsigned int checksum = twoBytes();

      Serial.print(prependedSize);
      Serial.println(F(" bytes"));
      Serial.print(F("Prepended checksum = "));
      Serial.println(checksum);
      Serial.print(F("Data: "));
      int i = 0;
      byte sum1 = 0;
      byte sum2 = 0;

      while (client.connected()) {
        while (client.available()) {
          byte e = client.read();

          if (i == 0) {
            Serial.print(F("0% "));
          }
          if (i % (prependedSize / 64) == 0) {
            Serial.print(F("."));
          }
          if (i == prependedSize - 1) {
            Serial.print(F(" 100%"));
          }

          file.write(e);
          sum1 = (sum1 + e) % 255;
          sum2 = (sum2 + sum1) % 255;
          i++;
        }
      }
      unsigned int computedChecksum = sum1 << 8;
      computedChecksum |= sum2;
      
      Serial.println();
      Serial.print("Computed Checksum = ");
      Serial.print(computedChecksum);

      file.close();
      Serial.println();
      Serial.print(i);
      Serial.println(F(" Bytes Received."));
      if (prependedSize <= 0 || prependedSize != i || checksum != computedChecksum) {
        Serial.println(F("ERROR RECEIVING FIRMWARE!"));
      }
      else {
        Serial.println(F("SUCCESS!"));
      }
      if (!client.connected()) {
        Serial.println(F("Disconecting from update server"));
        client.stop();
      }

      //Disable the SD card:
      pinMode(CHIP_SELECT, OUTPUT);
      digitalWrite(CHIP_SELECT, HIGH);

      if (i > 4096) {
        Serial.println(F("Resetting..."));
        delay(200);
        resetFunc();
      }
    }
    else {
      Serial.println(F("connection failed")); //error message if no client connect
      Serial.println();
      Serial.println(F("Resetting..."));
      delay(200);
      resetFunc();
    }
  }
}


/**
   setupWDT sets up the watchdog timer bits and scaler to execute every 8s.
*/
void setupWDT()
{
  // Clear the reset flag.
  MCUSR &= ~(1 << WDRF);

  // In order to change WDE or the prescaler, we need to
  // set WDCE (This will allow updates for 4 clock cycles).
  WDTCSR |= (1 << WDCE) | (1 << WDE);

  // Set new watchdog timeout prescaler value
  // 8.0 seconds
  WDTCSR = 1 << WDP0 | 1 << WDP3;

  // Enable the WD interrupt.
  WDTCSR |= _BV(WDIE);
}


/**
    The WDT_vect routine is called by the WDT interrupt.
    The Arduino will be reset once the update interval has been exceeded.

*/
ISR(WDT_vect)
{
  updateCounter++;
  if (updateCounter * 8 / 15 >= updateInterval)
  {
    updateNow = true;
  }
  // Quick Lock up watch Dawg.
  else if (updateCounter * 8 / 60 >= updateInterval)
  {
    resetFunc();
  }
}

/**
   resetEthernet reset the ethernet controller.
   @param pin is to be directly connected to the reset pin on the ethernet shield.
*/
void resetEthernet(int pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delay(500);
  digitalWrite(pin, HIGH);
  delay(500);  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    //Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
}





