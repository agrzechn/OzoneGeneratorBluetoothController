/***********************************************************
 *                                                         *
 *            OzoneGeneratorController - Bluetooth         *
 *                                                         *
 *                created by Adrian Grzechnik              *
 *                        Version  1.0                     *
 *                         18.03.2022                      *
 *                                                         *
 ***********************************************************/

#include "BluetoothSerial.h"    // https://github.com/espressif/arduino-esp32/tree/3a4ec66d41615cbb1c3e830cb6e761cdc4cca9d3/libraries/BluetoothSerial
#include "SimpleTimer.h"        // https://github.com/schinken/SimpleTimer

#define REPORT                1 
#define STATUS                0
#define REMAINING_TIME        1
#define sONLINE               1
#define sNOT_WORKING          2
#define sWORKING              3
#define aOFF                  1
#define aON                   2
#define aON_WITH_TIME         3
#define aSEND_WORKING_STATE   4
#define aOZONE_1_ON           5
#define aOZONE_2_ON           6
#define MAX_REPORT_BUFF       5
#define MAX_ACTION_BUFF       5
#define MAX_BUFF              5

// Pin Definitions
// Change these parameters to define pinout to relays  
#define OZONE_GENERATOR_1 16
#define OZONE_GENERATOR_2 18

// You can set device name visible in the application
#define DEVICE_NAME "OzoneGenerator01"

BluetoothSerial ESP_BT; 
SimpleTimer timer;
byte packetBuffer[MAX_BUFF];
int timerId;
unsigned int remainingTime = 0;   // [seconds]
boolean ozone1ON = false;
boolean ozone2ON = false;

void setup() {
  Serial.begin(19200);
  Serial.setTimeout(15);
  ESP_BT.begin(DEVICE_NAME);   // device name
  ESP_BT.setTimeout(15);
 
  pinMode(OZONE_GENERATOR_1, OUTPUT);
  pinMode(OZONE_GENERATOR_2, OUTPUT);

  timerId = timer.setInterval(1000, checkWorkingTime);
  timer.disable(timerId);
}

void loop() {
  timer.run();
  
  if (ESP_BT.available()) 
  {
    memset(packetBuffer, 0, MAX_ACTION_BUFF);
    ESP_BT.readBytes(packetBuffer, MAX_ACTION_BUFF);
    
    int action = (packetBuffer[0] & 0b00111111);
    if (action == aON)
    {
      Serial.println("Ozone Generator turn on");
      setOzone(packetBuffer[1]);
      sendState(sWORKING);
    } 
    else if (action == aOFF)
    {
      Serial.println("Ozone Generator turn off");
      turnOffOzone();
      remainingTime = 0;
      if(timer.isEnabled(timerId))
      {
        timer.restartTimer(timerId);
        timer.disable(timerId);
      }
      sendState(sNOT_WORKING);    
    }
    else if (action == aON_WITH_TIME)
    {
      if(!timer.isEnabled(timerId))
      {
        setOzone(packetBuffer[1]);
        int minutes = packetBuffer[2];
        int seconds = packetBuffer[3];
        remainingTime = minutes*60 + seconds;
        sendRemainingTime(minutes, seconds); 
        timer.enable(timerId);
        Serial.print("Ozone Generator turn on. Working time:  ");
        Serial.print(minutes);Serial.print(":");Serial.println(seconds);
      }
    }
    else if (action == aSEND_WORKING_STATE)
    {
      sendWorkingState();
    }
  }
}

void setOzone(int data1){
  if(((data1 >> 4) & 0b00001111) == aOZONE_1_ON)
    ozone1ON = true;
  if((data1 & 0b00001111) == aOZONE_2_ON)
    ozone2ON = true;
  if(ozone1ON)
    digitalWrite(OZONE_GENERATOR_1, HIGH);
  if(ozone2ON)
    digitalWrite(OZONE_GENERATOR_2, HIGH);
}

void turnOffOzone(){
  ozone1ON = false;
  ozone2ON = false;
  digitalWrite(OZONE_GENERATOR_1, LOW);
  digitalWrite(OZONE_GENERATOR_2, LOW);
}

void checkWorkingTime(){
  if(remainingTime > 0)
  {
    remainingTime--;
    int minutes = remainingTime/60;
    int seconds = remainingTime%60;
    sendRemainingTime(minutes, seconds);
  } 
  else
  {
    turnOffOzone();
    timer.restartTimer(timerId);
    timer.disable(timerId);
    sendState(sNOT_WORKING);
    Serial.println("Ozone Generator turn off");
  }
}

void sendRemainingTime(int minutes, int seconds){
  memset(packetBuffer, 0, MAX_REPORT_BUFF);
  packetBuffer[0] = ((REPORT<<6) & 0b11000000) + REMAINING_TIME;
  packetBuffer[1] = minutes;
  packetBuffer[2] = seconds;
  ESP_BT.write(packetBuffer, 3);
}

void sendState(int state){
  memset(packetBuffer, 0, MAX_REPORT_BUFF);
  packetBuffer[0] = ((REPORT<<6) & 0b11000000) + STATUS;
  packetBuffer[1] = state;
  ESP_BT.write(packetBuffer, 2);
}

void sendWorkingState(){
  if(digitalRead(OZONE_GENERATOR_1) == HIGH || digitalRead(OZONE_GENERATOR_2) == HIGH)
  {
    sendState(sWORKING);
    Serial.println("Device working!");
  } 
  else 
  {
    sendState(sNOT_WORKING);
    Serial.println("Device not working!");
  }
}
