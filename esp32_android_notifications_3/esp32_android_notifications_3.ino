/*****************************************************************************
  The MIT License (MIT)
  Copyright (c) 2021 Matthew James Bellafaire
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
******************************************************************************/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <U8x8lib.h>

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);


//variables and defines used by BLEServer.ino
String currentDataField;

String previous="";

#define SERVICE_UUID        "5ac9bc5e-f8ba-48d4-8908-98b80b566e49"
#define COMMAND_UUID        "bcca872f-1a3e-4491-b8ec-bfc93c5dd91a"
BLECharacteristic *commandCharacteristic;

//indicates connection state to the android device
boolean connected = false;

//indiciates whether or not a operation is currently in progress
boolean operationInProgress = false;

//function signitures
String sendBLE(String command);
void addData(String data);  //adds data to a current string, used within BLEServer.ino
void initBLE(); //initializes the BLE connection by starting advertising.




void addData(String data) {
//  Serial.println("Adding:" + data);
  currentDataField += data;
}

class cb : public BLEServerCallbacks    {
    void onConnect(BLEServer* pServer) {
      connected = true;
    }
    void onDisconnect(BLEServer* pServer) {
      connected = false;
    }
};

class ccb : public BLECharacteristicCallbacks  {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      addData(String( pCharacteristic->getValue().c_str()));
    }
    void onRead(BLECharacteristic* pCharacteristic) {
      //      Serial.println("Characteristic Read");
      operationInProgress = false;
    }
};

void initBLE() {
  BLEDevice::init("ESP32 Smartwatch");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  //define the characteristics and how they can be used
  commandCharacteristic = pService->createCharacteristic(
                            COMMAND_UUID,
                            BLECharacteristic::PROPERTY_READ   |
                            BLECharacteristic::PROPERTY_WRITE  |
                            BLECharacteristic::PROPERTY_NOTIFY
                          );


  //set all the callbacks
  commandCharacteristic->setCallbacks(new ccb());
  commandCharacteristic->setValue("");

  //add server callback so we can detect when we're connected.
  pServer->setCallbacks(new cb());

  pService->start();


  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

String sendBLE(String command) {
  operationInProgress = true;
  commandCharacteristic->setValue(command.c_str());
  commandCharacteristic->notify();

  currentDataField = "";

  unsigned long startTime = millis();
  while (operationInProgress && (startTime + 2000 > millis()))
    delay(25);

  return currentDataField;

}

void setup() {
  Serial.begin(115200);
  initBLE();
   u8x8.begin();
    u8x8.setFont(u8x8_font_7x14B_1x2_f);
   u8x8.clear();
   u8x8.setCursor(0,0); 
   u8x8.print("Starting...");
   delay(5000);
}

void loop() {

  u8x8.clear();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
 
  //available commands
    //sendBLE("/notifications"); //gets current android notifications as a string format "appName,Title;ExtraText,ExtraInfoText,ExtraSubText,ExtraTitle;Description;"
  //  sendBLE("/calendar", &string); // returns a string of calender events for the next 24 hours in format "title;description;startDate;startTime;endTime;eventLocation;"
  //  sendBLE("/time", &string); // returns a string representing the time
  //  sendBLE("/isPlaying", &string); //returns "true" or "false" indicating whether spotify is playing on the android device
  //  sendBLE("/currentSong", &string); //returns the current song name and artist playing on spotify as one string
  //  sendBLE("/play"); //hits the media play button on the android device
  //  sendBLE("/pause"); //hits the media pause button on the android device
  //  sendBLE("/nextSong"); //hits the media next song button on the android device
  //  sendBLE("/lastSong"); //hits the media previous song button on the android device

  //Serial.println(sendBLE("/time"));
  String outcome = sendBLE("/notifications");

  if(outcome.length() < 2) {
     u8x8.setCursor(0,0);
     u8x8.print("Checking ...");
  }
  else {
  //Serial.println(outcome);
    Serial.println("------------------");

    String theTime = sendBLE("/time");
    String t = getValue(theTime,' ',0);
    String date = getValue(theTime,' ',1);
    Serial.println(t);
    u8x8.setCursor(0,0);
    u8x8.print(t);
    int linenum = 1;
    for (int i =0; i<30; i++ ){
      String xval = getValue(outcome, '\n', i);
      if(xval.length() > 0) {
         u8x8.setCursor(0,linenum);
         //u8x8.print(i);
         //u8x8.print(":");
         String sub = xval.substring(0,12);
         u8x8.print(sub);
         linenum+=1;
        Serial.print(i);
        Serial.print(":");
        Serial.println(xval);
      }
      //Serial.print("|");
      //Serial.println(xval.length());
      //Serial.println(xval);
    }
    
    delay(5000);
  }
}
String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
