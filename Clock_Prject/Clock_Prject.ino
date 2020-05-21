#include <Wire.h>
#include"RTClib.h"
#include <DS3231.h>
#include <TM1637Display.h>

RTC_DS3231 rtc;
DS3231 Clock;

/** TIME UPDATE*/
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
const int buttonPin_Tube = A3; //the number of the pushbutton pin which to change displayment between Tube and 7-segment
boolean TempChangeShow = false;

int buttonState; //previous stable state of the input pin
int lastButtonState = HIGH; //the previous reading from the input pin
long lastDebounceTime = 0; //the last time the output pin was toggled
long debounceDelay = 50; //the debounce time; increase if the output flickers

int Tube_SW_LastState = HIGH;//the previous reading from the Tube switch


/*function*/
boolean TEMP_Button_Push_Check (int&, int&, int&, long&, long&, boolean&);
void TUBE_Displayment (int&, uint8_t&);
boolean TUBE_Switch_Check (int&, int&);

/*Clock convert Array*/
uint8_t D_Array[10] = {0b00111111,0b00000110,0b01011011,0b01001111,0b01100110,0b01101101,0b01111101,0b00000111,0b01111111,0b01101111};


/*Tube Display variable*/
int T_buttonState = LOW;
uint8_t T_Array [10] = {0b00000111,0b00001111,0b00010111,0b00011111,0b00100111,0b00111000,0b00111001,0b00111010,0b00111011,0b00111100};
int tube_1_Anode = 8;
int tube_2_Anode = 9;
int tube_3_Anode = 10;
int tube_4_Anode = 11;
int colon_cathode = 12;

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
  pinMode(buttonPin_Tube, INPUT_PULLUP); //Change Tube Display Button 

  //Tube Dispaly Part (cathode pin)
  pinMode(2,OUTPUT); //D2
  pinMode(3,OUTPUT); //D3
  pinMode(4,OUTPUT); //D4
  pinMode(5,OUTPUT); //D5
  pinMode(6,OUTPUT); //D6
  pinMode(7,OUTPUT); //D7
  pinMode(colon_cathode,OUTPUT); //Colon cathode

  //anodepin
  pinMode(tube_1_Anode,OUTPUT); //D4
  pinMode(tube_2_Anode,OUTPUT); //D5
  pinMode(tube_3_Anode,OUTPUT); //D6
  pinMode(tube_4_Anode,OUTPUT); //D7

  

  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(5, HIGH);
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);
  digitalWrite(10, LOW);
  digitalWrite(11, LOW);
  
}

void loop() {
  int T_buttonState = digitalRead(buttonPin_Tube);
  // time update very second
  //unsigned long currentMillis = millis(); 
  boolean StoptheLoop = false;
  int reading = digitalRead(buttonPin);
  StoptheLoop = TEMP_Button_Push_Check (reading, buttonState, lastButtonState,lastDebounceTime,debounceDelay, TempChangeShow);
  if (StoptheLoop == true){
    return;
  }
  
  // decide using Tube or TM1637
  if (T_buttonState == true){ 

  //decide show Temperature or Time
  if (TempChangeShow == false) {
      unsigned long previousMills = millis();
      //previousMills = currentMillis;
      DateTime now = rtc.now();
      hh = now.hour();
      mm = now.minute();
      ss = now.second();
      
      /*hour:minute*/
      int t = hh.toInt()*100 + mm.toInt();
      /*minute:seconds*/
      //int t = mm.toInt()*100 +ss.toInt();
      display.showNumberDec(t, true);
      while ((millis()-previousMills>0) && (millis()-previousMills<500)){
        
        /*check the temperature button*/
        int reading = digitalRead(buttonPin);
        StoptheLoop = TEMP_Button_Push_Check (reading, buttonState, lastButtonState,lastDebounceTime,debounceDelay, TempChangeShow);
        if (StoptheLoop == true){
             return;
        }
        
        /*Check the Tube Switch*/
        reading = digitalRead(buttonPin_Tube);
        StoptheLoop = TUBE_Switch_Check (reading, Tube_SW_LastState);
        if (StoptheLoop == true){
             return;
        }
     };
    
    unsigned long colon_t = millis();
    uint8_t segto;    //Display colon
    segto = 0x80 | display.encodeDigit((t/100)%10);
    display.setSegments(&segto, 1, 1);  /** Display center colon*/ 
    while ((millis()-colon_t>0) && (millis()-colon_t<500)){
      
      /*check the temperature button*/
      int reading = digitalRead(buttonPin);
      StoptheLoop = TEMP_Button_Push_Check (reading, buttonState, lastButtonState,lastDebounceTime,debounceDelay, TempChangeShow);
      if (StoptheLoop == true){
             return;
        }
      /*Check the Tube Switch*/
      reading = digitalRead(buttonPin_Tube);
      StoptheLoop = TUBE_Switch_Check (reading, Tube_SW_LastState);
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
   
   
   else {   //Tube dispaly main part
    //turn off all segments
    uint8_t data[] = {0x0, 0x0, 0x0, 0x0}; 
    display.setSegments(data);

    if (TempChangeShow == false) {
      unsigned long previousMills = millis(); 
      digitalWrite(colon_cathode, HIGH); //Colon light up
      while ((millis()-previousMills>0) && (millis()-previousMills<500)){
        //previousMills = currentMillis;
        /*Get time from RTC*/
        DateTime now = rtc.now();
        hh = now.hour();
        mm = now.minute();
        ss = now.second();

        //convert Number into single variable
        uint8_t Fir_Num = hh.toInt()/10;
        uint8_t Sec_Num = hh.toInt()%10;
        uint8_t Trd_Num = mm.toInt()/10;
        uint8_t Fou_Num = mm.toInt()%10;
        //Time Displayment control
        TUBE_Displayment(tube_1_Anode,T_Array[Fir_Num]);
        TUBE_Displayment(tube_2_Anode,T_Array[Sec_Num]);
        TUBE_Displayment(tube_3_Anode,T_Array[Trd_Num]);
        TUBE_Displayment(tube_4_Anode,T_Array[Fou_Num]);

        /*check the temperature button*/
        int reading = digitalRead(buttonPin);
        StoptheLoop = TEMP_Button_Push_Check (reading, buttonState, lastButtonState,lastDebounceTime,debounceDelay, TempChangeShow);
        if (StoptheLoop == true){
             return;
        }
        /*Check the Tube Switch*/
       reading = digitalRead(buttonPin_Tube);
       StoptheLoop = TUBE_Switch_Check (reading, Tube_SW_LastState);
       if (StoptheLoop == true){
             return;
        }
      };
      previousMills = millis(); 
      digitalWrite(colon_cathode, LOW); //Colon dimmed
      while ((millis()-previousMills>0) && (millis()-previousMills<500)){
        //previousMills = currentMillis;
        /*Get time from RTC*/
        DateTime now = rtc.now();
        hh = now.hour();
        mm = now.minute();
        ss = now.second();

        //convert Number into single variable
        uint8_t Fir_Num = hh.toInt()/10;
        uint8_t Sec_Num = hh.toInt()%10;
        uint8_t Trd_Num = mm.toInt()/10;
        uint8_t Fou_Num = mm.toInt()%10;
        //Time Displayment control
        TUBE_Displayment(tube_1_Anode,T_Array[Fir_Num]);
        TUBE_Displayment(tube_2_Anode,T_Array[Sec_Num]);
        TUBE_Displayment(tube_3_Anode,T_Array[Trd_Num]);
        TUBE_Displayment(tube_4_Anode,T_Array[Fou_Num]);
        
        /*check the button*/
        int reading = digitalRead(buttonPin);
        StoptheLoop = TEMP_Button_Push_Check (reading, buttonState, lastButtonState,lastDebounceTime,debounceDelay, TempChangeShow);
        if (StoptheLoop == true){
             return;
        }
       /*Check the Tube Switch*/
       reading = digitalRead(buttonPin_Tube);
       StoptheLoop = TUBE_Switch_Check (reading, Tube_SW_LastState);
       if (StoptheLoop == true){
             return;
        }
      };
    }
    else{ //Temperature Displayment
      digitalWrite(colon_cathode, LOW); //Colon shut down
      int TEMP = Clock.getTemperature(); //get temperature
      //convert Number into single variable
      uint8_t Fir_Num = TEMP/10;
      uint8_t Sec_Num = TEMP%10;
      //Time Displayment control
      TUBE_Displayment(tube_3_Anode,T_Array[Fir_Num]);
      TUBE_Displayment(tube_4_Anode,T_Array[Sec_Num]);
      
      /*Check the Temperature button*/
      int reading = digitalRead(buttonPin);
        StoptheLoop = TEMP_Button_Push_Check (reading, buttonState, lastButtonState,lastDebounceTime,debounceDelay, TempChangeShow);
        if (StoptheLoop == true){
             return;
        }
       /*Check the Tube Switch*/
       reading = digitalRead(buttonPin_Tube);
       StoptheLoop = TUBE_Switch_Check (reading, Tube_SW_LastState);
       if (StoptheLoop == true){
             return;
        }
    }
      
      
      
    
   }

}



boolean TEMP_Button_Push_Check (int& reading, int&buttonState, int& lastButtonState,long& lastDebounceTime,long& debounceDelay ,boolean& TempChangeShow){
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

void TUBE_Displayment (int& Anode_display ,uint8_t& TUBE_to_Display){
  //Anode control
  digitalWrite(Anode_display,HIGH);
  //Cathode control
  digitalWrite(2, bitRead(TUBE_to_Display,5));
  digitalWrite(3, bitRead(TUBE_to_Display,4));
  digitalWrite(4, bitRead(TUBE_to_Display,3));
  digitalWrite(5, bitRead(TUBE_to_Display,2));
  digitalWrite(6, bitRead(TUBE_to_Display,1));
  digitalWrite(7, bitRead(TUBE_to_Display,0));
  //close Cathode and Anode
  digitalWrite(Anode_display,LOW);
  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(5, HIGH);
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);
}

boolean TUBE_Switch_Check (int& reading, int& Tube_SW_LastState){
  boolean result = false;
  if (reading != Tube_SW_LastState) {
    Tube_SW_LastState = reading;
    result = true;
  }
}


