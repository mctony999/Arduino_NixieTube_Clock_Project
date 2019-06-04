#include <Wire.h>
#include"RTClib.h"
#include <DS3231.h>
#include <TM1637Display.h>

RTC_DS3231 rtc;
DS3231 Clock;

/** TIME UPDATE*/
unsigned long previousMills = 0;
const long interval = 1000;

/**TM1637Display*/
#define CLK 2
#define DIO 3

TM1637Display display(CLK, DIO);
boolean colon = true;

String tt = "";
String hh = "";
String mm = "";
String ss = "";

/**Button*/
const int buttonPin = 4; //the number of the pushbutton pin
boolean TempChangeShow = false;

int buttonState; //previous stable state of the input pin
int lastButtonState = LOW; //the previous reading from the input pin
long lastDebounceTime = 0; //the last time the output pin was toggled
long debounceDelay = 50; //the debounce time; increase if the output flickers

void setup() {
  Serial.begin(9600);
  display.setBrightness(0xA);
  /** RTC SYNC PROCESS */ 
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    DateTime timeupdate = DateTime(F(__DATE__), F(__TIME__)) + TimeSpan( /*days*/0,/*hours*/0, /*minutes*/0, /*seconds*/11 );
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
     rtc.adjust(timeupdate);
  }

  /**Button Setup*/
  pinMode(buttonPin,INPUT_PULLUP); //enable pullup resistor
  
  
}

void loop() {
    
  // time update very second
  unsigned long currentMillis = millis();
  int reading = digitalRead(buttonPin);
  
  Serial.print("start");
  Serial.print('\n');
  
  if (reading != lastButtonState){ //button state changed
    lastDebounceTime = millis(); //update last debounce time
  }

  Serial.print("Second");
  Serial.print('\n');
  
  if ((millis() - lastDebounceTime) > debounceDelay){ //overtime
    if (reading != buttonState) {//button state has changed
    buttonState = reading; //update previous stable button state
    if (buttonState == LOW){ // button presses
      TempChangeShow =!TempChangeShow; //reverse the boolean
      Serial.print('-');
    }
    }
  }
  
  if (TempChangeShow == true) {
    if( currentMillis - previousMills >= interval ){
      previousMills = currentMillis;
      DateTime now = rtc.now();
      hh = now.hour();
      mm = now.minute();
      ss = now.second();

      uint8_t segto;    //Display colon
      int value = 1000;
      //hour:minute
      int t = hh.toInt()*100 + mm.toInt();
      //minute:seconds
      //int t = mm.toInt()*100 +ss.toInt();
    
     /** Display center colon */
     segto = 0x80 | display.encodeDigit((t/100)%10);
     display.setSegments(&segto, 1, 1);  
     delay(500);
     display.showNumberDec(t, true);
     delay(500);
  }
  }
  else {
    int TEMP = Clock.getTemperature(); //get temperature
   Serial.print(TEMP);
    Serial.print('\n');
    
    /**mapping Temperature number to 7-segment*/
    uint8_t TEMP_1 = TEMP/10;
    uint8_t TEMP_2 = TEMP%10;
    //First number
    if (TEMP_1==0){
      TEMP_1 = 0b00111111;
    }
    else if (TEMP_1==1){
      TEMP_1 = 0b00000110;
    }
    else if (TEMP_1==2){
      TEMP_1 = 0b01011011;
    }
    else if (TEMP_1==3){
      TEMP_1 = 0b01001111;
    }
    else if (TEMP_1==4){
      TEMP_1 = 0b01100110;
    }
    else if (TEMP_1==5){
      TEMP_1 = 0b01101101;
    }
    else if (TEMP_1==6){
      TEMP_1 = 0b01111101;
    }
    else if (TEMP_1==7){
      TEMP_1 = 0b00000111;
    }
    else if (TEMP_1==8){
      TEMP_1 = 0b01111111;
    }
    else{
     TEMP_1 = 0b01101111;
      }
    //second number
    if (TEMP_2==0){
      TEMP_2 = 0b00111111;
    }
    else if (TEMP_2==1){
      TEMP_2 = 0b00000110;
    }
    else if (TEMP_2==2){
      TEMP_2 = 0b01011011;
    }
    else if (TEMP_1==3){
      TEMP_1 = 0b01001111;
    }
    else if (TEMP_2==4){
      TEMP_2 = 0b01100110;
    }
    else if (TEMP_2==5){
      TEMP_2 = 0b01101101;
    }
    else if (TEMP_2==6){
      TEMP_2 = 0b01111101;
    }
    else if (TEMP_2==7){
      TEMP_2 = 0b00000111;
    }
    else if (TEMP_2==8){
      TEMP_2 = 0b01111111;
    }
    else{
     TEMP_2 = 0b01101111;
      }

    /**Show Temperature*/
    uint8_t data[] = {TEMP_1,TEMP_2,0b01100011,0b00111001};
    display.setSegments(data);
    delay(1000);
  }
}
