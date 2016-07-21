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

// The carriage return, newline sequence that separates http headers from content
char crnl[] = {'\r', '\n', '\r', '\n'};

EthernetClient client;
IPAddress ip(10, 0, 0, 24);
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

/**
   ElCheapo reset function, should probably use the WDT instead, right..
*/
void(* resetFunc)(void) = 0;

void setup() {
  resetEthernet(A5);
  Serial.begin(9600);
  delay(500);
  Serial.println(F("Started"));
  setupWDT();
  // put your setup code here, to run once:

}

void loop() {
  // Leave this here
  apolloUpdate();

}


void apolloUpdate() {
  if (updateNow) {
    resetEthernet(A5);
    delay(2000);
    Serial.println(F("Apollo init"));

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
      while (client.connected()) {
        while (client.available()) {
          if (checkBytes(crnl, 4, byteStream)) {
            goto end;
          }
        }
      }
end:
      Serial.println();
      Serial.print(F("File size = "));

      // The first two bytes are the size of the file
      unsigned int prependedSize = twoBytesToInt(byteStream);
      // The next two bytes are the checksum
      unsigned int checksum = twoBytesToInt(byteStream);

      Serial.print(prependedSize);
      Serial.println(F(" bytes"));
      Serial.print(F("File checksum = "));
      Serial.println(checksum);
      Serial.print(F("Receiving Data: "));
      int bytesReceived = 0;
      byte sum1 = 0;
      byte sum2 = 0;

      while (client.connected()) {
        while (client.available()) {
          byte e = client.read();

          if (bytesReceived == 0) {
            Serial.print(F("0% "));
          }
          else if (bytesReceived % (prependedSize / 64) == 0) {
            Serial.print(F("."));
          }
          else if (bytesReceived == prependedSize - 1) {
            Serial.print(F(" 100%"));
          }

          file.write(e);
          sum1 = (sum1 + e) % 255;
          sum2 = (sum2 + sum1) % 255;
          bytesReceived++;
        }
      }

      unsigned int computedChecksum = sum1 << 8;
      computedChecksum |= sum2;

      Serial.println();
      Serial.print(F("File check: "));
      if (prependedSize == bytesReceived && checksum == computedChecksum) {
        Serial.println(F("OK"));
      }
      else if (checksum != computedChecksum) {
        Serial.println(F("CHECKSUM ERROR!"));
      }
      else if (prependedSize != bytesReceived) {
        Serial.println(F("SIZE ERROR!"));
      }

      if (!client.connected()) {
        Serial.println(F("Disconecting"));
        client.stop();
      }


      file.close();
      //Disable the SD card:
      pinMode(CHIP_SELECT, OUTPUT);
      digitalWrite(CHIP_SELECT, HIGH);
      Serial.println(F("Resetting..."));
      delay(200);
      resetFunc();

    }
    else {
      Serial.println(F("Connection failed")); //error message if no client connect
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




/**
 * A little wrapper function which can be pointed to.
 */
char byteStream() {
  return client.read();
}

/**
 * Used to count the amount of mathching bytes found in the stream.
 * It can not be local to checkBytes
 */
int matchingBytes = 0;

/**
 * Searches a byteStream for a sequence of bytes
 */
bool checkBytes(char* sequence, int sequenceLength, char (*stream)()) {
  int byteCount = 0;
  while (matchingBytes < sequenceLength) {
    matchingBytes = sequence[matchingBytes] == stream() ? matchingBytes + 1 : 0;
    byteCount++;
    // Allow some sort of default behaviour
    if (byteCount >= 2048) {
      return false;
    }
  }
  return (matchingBytes == sequenceLength); // a bit pedantic
}

/**
 * Reads two consecutive bytes from the byteStream, and returns a little endian int.
 */
int twoBytesToInt(char (*stream)()) {
  int tempVal = 0;
  tempVal = stream() << 8;
  tempVal |= stream();
  return tempVal;
}


