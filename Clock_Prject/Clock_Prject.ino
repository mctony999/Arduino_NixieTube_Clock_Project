#include <Wire.h>
#include"RTClib.h"
#include <DS3231.h>
#include <TM1637Display.h>

RTC_DS3231 rtc;
DS3231 Clock;

/** TIME UPDATE*/
//unsigned long previousMills = 0;
const long interval = 500;

/**TM1637Display*/
#define CLK A0
#define DIO A1


int colon_value = 1244; //For colon value 
TM1637Display display(CLK, DIO);
//TM1637Display display(A0,13);
boolean colon = true;

String tt = "";
String hh = "";
String mm = "";
String ss = "";

/**Button*/
const int buttonPin = A2 ; //the number of the pushbutton pin
boolean TempChangeShow = false;

int buttonState; //previous stable state of the input pin
int lastButtonState = HIGH; //the previous reading from the input pin
long lastDebounceTime = 0; //the last time the output pin was toggled
long debounceDelay = 50; //the debounce time; increase if the output flickers

/*function*/
boolean Button_Push_Check (int&, int&, int&, long&, long&, boolean&);

/*Clock convert Array*/
uint8_t D_Array[10] = {0b00111111,0b00000110,0b01011011,0b01001111,0b01100110,0b01101101,0b01111101,0b00000111,0b01111111,0b01101111};




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
  pinMode(12,OUTPUT);
  pinMode(13,OUTPUT);
  
  pinMode(buttonPin,INPUT_PULLUP); //enable pullup resistor
  
  
}

void loop() {
    
  // time update very second
  //unsigned long currentMillis = millis(); 
  boolean StoptheLoop = false;
  int reading = digitalRead(buttonPin);
  StoptheLoop = Button_Push_Check (reading, buttonState, lastButtonState,lastDebounceTime,debounceDelay, TempChangeShow);
  if (StoptheLoop == true){
    return;
  }

  //decide show Temperature or Time
  if (TempChangeShow == false) {
      unsigned long previousMills = millis();
      //previousMills = currentMillis;
      DateTime now = rtc.now();
      hh = now.hour();
      mm = now.minute();
      ss = now.second();
      
      //hour:minute
      int t = hh.toInt()*100 + mm.toInt();
      //minute:seconds
      //int t = mm.toInt()*100 +ss.toInt();
      display.showNumberDec(t, true);
      while ((millis()-previousMills>0) && (millis()-previousMills<500)){
        int reading = digitalRead(buttonPin);
        StoptheLoop = Button_Push_Check (reading, buttonState, lastButtonState,lastDebounceTime,debounceDelay, TempChangeShow);
        if (StoptheLoop == true){
             return;
        }
     };
    
    unsigned long colon_t = millis();
    uint8_t segto;    //Display colon
    segto = 0x80 | display.encodeDigit((t/100)%10);
    display.setSegments(&segto, 1, 1);  /** Display center colon*/ 
    while ((millis()-colon_t>0) && (millis()-colon_t<500)){
      int reading = digitalRead(buttonPin);
      StoptheLoop = Button_Push_Check (reading, buttonState, lastButtonState,lastDebounceTime,debounceDelay, TempChangeShow);
      if (StoptheLoop == true){
             return;
        }
      };
  }
  else {
    int TEMP = Clock.getTemperature(); //get temperature
    /**mapping Temperature number to 7-segment and show*/
    uint8_t data[] = {D_Array[TEMP/10],D_Array[TEMP%10],0b01100011,0b00111001};
    display.setSegments(data);
  }
}


boolean Button_Push_Check (int& reading, int&buttonState, int& lastButtonState,long& lastDebounceTime,long& debounceDelay ,boolean& TempChangeShow){
  boolean result = false;
  if (reading != lastButtonState){ //button state changed
    lastDebounceTime = millis(); //update last debounce time
  }
  if ((millis() - lastDebounceTime) > debounceDelay){ //overtime
    if (reading != buttonState) {//button state has changed
    buttonState = reading; //update previous stable button state
    if (buttonState == LOW){ // button presses
      TempChangeShow =!TempChangeShow; //reverse the boolean
      result = true;
    }
    }
  }
  lastButtonState = reading; //update last button state
  return result;
}


