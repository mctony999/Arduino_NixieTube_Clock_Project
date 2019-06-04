#include <Wire.h>
#include"RTClib.h"
#include <TM1637Display.h>

RTC_DS3231 rtc;

/** TIME UPDATE*/
unsigned long previousMills = 0;
const long interval = 1000;

/**TM1637Display*/
#define CLK 2
#define DIO 3

TM1637Display display(CLK, DIO);
boolean colon = true;

String dw = "";
String hh = "";
String mm = "";
String ss = "";

/**Button*/
const int buttonPin = 4; //the number of the pushbutton pin
int ledState = HIGH; //the current state of the output pin

int buttonState; //previous stable state of the input pin
int lastButtonState = LOW; //the previous reading from the input pin
long lastDebounceTime = 0 //the last time the output pin was toggled
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
