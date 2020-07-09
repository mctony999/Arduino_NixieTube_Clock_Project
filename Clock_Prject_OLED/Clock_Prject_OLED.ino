/**Oled part*/
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
/**RTC Part*/
#include"RTClib.h"
#include <DS3231.h>
/**Oled Custom Font*/
#include <Fonts/FreeMonoBold12pt7b.h>  // Add a custom font
#include <Fonts/FreeMono9pt7b.h>  // Add a custom font
/**Enconder Part*/
#include <ClickEncoder.h>
#include <TimerOne.h>
/**Oled Screen size define*/
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
/**Arduino Tube Control pin define */
#define IC_595_1_input 2
#define IC_595_2_input 3
#define IC_595_3_input 4
#define IC_595_4_input 5
#define IC_595_5_input 6
#define IC_595_SRCLK_input 7
#define IC_595_SHIFT_pin 8
// Declaration for SSD1306 display connected using software SPI (default case):
#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    29
#define OLED_RESET 13
/**Oled initial*/
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
/**RTC Variable declare*/
RTC_DS3231 rtc;
DS3231 Clock;
ClickEncoder *encoder;

/** Function*/
void timerIsr();//Enconder need
int Time_Setup_Adjust(int, int);
void Oled_Time_Display_Function(int, int, int, int, boolean);
void Oled_Date_Display_Function(int, int, int);
boolean Oled_Date_Setting_Function(int,int,int,int);
void Oled_Setup_Menu_Display_Function();
void Tube_Switch_off_Function(int, int, int, int);
/**State Machine function*/
void StateMachine_Main();
void Oled_Time_Display();
void Oled_Temp_Display();
void Oled_Date_Display();
void Oled_Time_Setup();
void Oled_Date_Setup();
void Close_Oled_Start_Tube();
void Tube_Display_Contorl();
void Tube_Temp_Display();
void Tube_Date_Display();
void Tube_Time_Setup();
void Tube_Date_Setup();
void Setup_Menu();

/** Variable*/
unsigned long previousMills;
ClickEncoder::Button B_State = encoder->getButton(); //Button State container

boolean Display_12_hours = false; //F for 24, T for 12
boolean Display_F = false; //F for Celsius, T for Fahrenheit
boolean Oled_to_Tube = true; //For switch tube and oled state
int h_Fir = 0; //For Time Display Variable
int h_Sec = 0;
int m_Fir = 0;
int m_Sec = 0;
/*Tube control Variable*/
//byte Tube_cathode_Control_1 = 0;
//byte Tube_cathode_Control_2 = 0;
//byte Tube_cathode_Control_3 = 0;
//byte Tube_cathode_Control_4 = 0;
//byte Tube_cathode_Control_5 = 0;

int state_Main = 0; //State Machine state control
int state_Main_prev = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  /**Encoder Initial*/
  encoder = new ClickEncoder(A1, A2, A0); //DT,CLK,sw
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); 

  /**Oled Initial*/
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
   Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
    
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.clearDisplay();  // Clear the buffer

  /** RTC Initial PROCESS */ 
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  /** RTC SYNC PROCESS */ 
//  if (rtc.lostPower()) {
//    Serial.println("RTC lost power, lets set the time!");
//    // following line sets the RTC to the date & time this sketch was compiled
//    DateTime timeupdate = DateTime(F(__DATE__), F(__TIME__)) + TimeSpan( /*days*/0,/*hours*/0, /*minutes*/0, /*seconds*/11 );
//    // This line sets the RTC with an explicit date & time, for example to set
//    // January 21, 2014 at 3am you would call:
//     rtc.adjust(timeupdate);
//  }
  /** RTC SYNC PROCESS End */ 

  /**Pin Mode initial*/
  pinMode(IC_595_1_input,OUTPUT); //74595_1 input pin
  pinMode(IC_595_2_input,OUTPUT); //74595_2 input pin
  pinMode(IC_595_3_input,OUTPUT); //74595_3 input pin
  pinMode(IC_595_4_input,OUTPUT); //74595_4 input pin
  pinMode(IC_595_5_input,OUTPUT); //74595_5 input pin
  pinMode(IC_595_SRCLK_input,OUTPUT); //For all 74595 Clock pin
  pinMode(IC_595_SHIFT_pin,OUTPUT); //For all 74595 shift register
  pinMode(12,OUTPUT); //In-3 Tube pin

  Tube_Display_Contorl(10, 10, 10, 10); //Tube initial
}

void loop() {
  StateMachine_Main();

//  display.drawRect(0, 0, 128, 64, WHITE); //boundary
//  display.drawLine(63, 0, 63, 63, WHITE); //Vertical line
//  display.drawLine(0, 31, 127, 31, WHITE); //Horizontal line
 
}

void timerIsr() {
  encoder->service();
}
/**Adjust setup stage time display*/
int Time_Setup_Adjust(int Time, int H_or_M){
  switch (H_or_M){
    case 0:
    if (Time > 23)
    {
      Time = 0;
      }
    else if (Time < 0)
    {
      Time = 23;
    }
    return Time;
    break;
    case 1:
    if (Time >= 60)
    {
      Time = 0;
      }
    else if (Time < 0)
    {
      Time = 59;
    }
    return Time;
    break;
  }
}

void Oled_Time_Display_Function(int H_fn, int H_Sn, int M_fn, int M_Sn, boolean Show_Colon){
  int T_V = H_fn;
  display.setFont(&FreeMonoBold12pt7b);  // Set a custom font
  /*Colon display*/
  if (Show_Colon == true) 
  {
    if (Display_12_hours == true) 
    {
      display.fillRect(61, 9, 5, 5, WHITE);
      display.fillRect(61, 29, 5, 5, WHITE);
    }
    else
    {
      display.fillRect(61, 20, 5, 5, WHITE);
      display.fillRect(61, 40, 5, 5, WHITE);
    }
  }
  
  if (Display_12_hours == true)
  {
    if ((H_fn*10+H_Sn)>12)
    {
      H_fn = ((H_fn*10+H_Sn)-12)/10;
      H_Sn = ((T_V*10+H_Sn)-12)%10;

      display.setCursor(90,57);  // (x,y)
      display.setTextSize(0);
      display.println("pm");
    }
    else
    {
      display.setCursor(90,57);  // (x,y)
      display.setTextSize(0);
      display.println("am");
    }
    display.setTextSize(2);
    /*First number*/
    display.setCursor(5,35);  // (x,y)
    display.println(H_fn);
    /*Second number*/
    display.setCursor(30,35);  // (x,y)
    display.println(H_Sn);
    /*Thrid number*/
    display.setCursor(70,35);  // (x,y)
    display.println(M_fn);
    /*Four number**/
    display.setCursor(95,35);  // (x,y)
    display.println(M_Sn);
  }
  else
  {
    display.setTextSize(2);
   /*First number*/
    display.setCursor(5,45);  // (x,y)
    display.println(H_fn);
   /*Second number*/
   display.setCursor(30,45);  // (x,y)
   display.println(H_Sn);
   /*Thrid number*/
   display.setCursor(70,45);  // (x,y)
   display.println(M_fn);
   /*Four number**/
   display.setCursor(95,45);  // (x,y)
   display.println(M_Sn);
  }

}

void Oled_Date_Display_Function(int Year, int Month, int Days){
  char string[10];  // Create a character array of 10 characters
  
  display.setFont(&FreeMonoBold12pt7b);
  display.setTextSize(0);
  /**Years number*/
  display.setCursor(3,20);  // (x,y)
  display.println(Year);
  display.setTextSize(2);
  /**Month number*/
  dtostrf(Month, 3, 0, string);  // (<variable>,<amount of digits we are going to use>,<amount of decimal digits>,<string name>)
  /** uncommon if you want to let digit left Align 
  display.setCursor(8,60);  // (x,y)
  display.println(Month);  // Text or value to print
  */
  display.setCursor(-25,60);  // (x,y)
  display.println(string);  // Text or value to print
  /**Day number*/
  dtostrf(Days, 3, 0, string);  // (<variable>,<amount of digits we are going to use>,<amount of decimal digits>,<string name>)
  /** uncommon if you want to let digit left Align 
  display.setCursor(70,60);  // (x,y)
  display.println(Day);  // Text or value to print
  */
  display.setCursor(45,60);  // (x,y)
  display.println(string);  // Text or value to print  
  display.fillCircle(66, 60, 2, WHITE); //dot
}

void Oled_Setup_Menu_Display_Function(){
  display.setFont();
  display.setTextSize(2);// Normal 1:1 pixel scale
  display.setCursor(3,3); 
  /**Can add if else to change displayment*/
  if (Display_12_hours)
  {
    display.println("12 hours");
  }
  else
  {
    display.println("24 hours");
  }
  display.setCursor(25,27); 
  /**Can add if else to change displayment*/
  if (Display_F) 
  {
    display.println("F");
  }
  else
  {
    display.println("C");
  }
  display.fillCircle(15, 26, 4, WHITE);
  display.fillCircle(15, 26, 2, BLACK);
  display.setCursor(3,45); 
  display.println("Time set");
}

void Tube_Display_Contorl(int T_1, int T_2, int T_3, int T_4){
  volatile byte Tube_cathode_Control_1 = 0;
  volatile byte Tube_cathode_Control_2 = 0;
  volatile byte Tube_cathode_Control_3 = 0;
  volatile byte Tube_cathode_Control_4 = 0;
  volatile byte Tube_cathode_Control_5 = 0;

  
  if (T_1 != 10){ //10 for close the tube
    if (T_1 < 8) {
      bitWrite(Tube_cathode_Control_1, T_1, 1);
    }
    else 
    {
      bitWrite(Tube_cathode_Control_2, T_1-8, 1);
    }
  }
  if (T_2 != 10){ //10 for close the tube
    if (T_2 < 6) {
      bitWrite(Tube_cathode_Control_2, T_2+2, 1);
    }
    else 
    {
      bitWrite(Tube_cathode_Control_3, T_2-6, 1);
    }
  }
  if (T_3 != 10){ //10 for close the tube
    if (T_3 < 4) {
      bitWrite(Tube_cathode_Control_3, T_3+4, 1);
    }
    else 
    {
      bitWrite(Tube_cathode_Control_4, T_3-4, 1);
    }
  }
  if (T_4 != 10){ //10 for close the tube
    if (T_4 < 2) {
      bitWrite(Tube_cathode_Control_4, T_4+6, 1);
    }
    else 
    {
      bitWrite(Tube_cathode_Control_5, T_4-2, 1);
    }
  }
  
  for (int i = 7; i>=0; --i) {
    digitalWrite(IC_595_1_input, bitRead(Tube_cathode_Control_1,i));
    digitalWrite(IC_595_2_input, bitRead(Tube_cathode_Control_2,i));
    digitalWrite(IC_595_3_input, bitRead(Tube_cathode_Control_3,i));
    digitalWrite(IC_595_4_input, bitRead(Tube_cathode_Control_4,i));
    digitalWrite(IC_595_5_input, bitRead(Tube_cathode_Control_5,i));
    digitalWrite(IC_595_SRCLK_input, HIGH); //Next Clock
    delay(1);
    digitalWrite(IC_595_SRCLK_input, LOW); //Next Clock
  }
  digitalWrite(IC_595_SHIFT_pin, HIGH); // Shift the output
  delay(1);
  digitalWrite(IC_595_SHIFT_pin, LOW); // Shift the output

}

void StateMachine_Main(){
  switch (state_Main) {
    case 0: //Oled_Time_Display
    Oled_Time_Display();
    break;

    case 1: //Oled_Temp_Display
    Oled_Temp_Display();
    break;

    case 2: //Oled_Date_Display
    Oled_Date_Display();
    break;

    case 3: //Oled_Time_Setup
    Oled_Time_Setup();
    break;

    case 4: //Oled_Date_Setup
    Oled_Date_Setup();
    break;

    case 5: //Close_Oled_Start_Tube
    Close_Oled_Start_Tube();
    break;

    case 6: //Tube_Time_Display
    Tube_Time_Display();
    break;

    case 7: //Tube_Temp_Display
    Tube_Temp_Display();
    break;

    case 8: //Tube_Date_Display
    Tube_Date_Display();
    break;

    case 9: //Tube_Time_Setup
    Tube_Time_Setup();
    break;

    case 10: //Tube_Date_Setup
    /**Don't have enough memory size to finish this*/
//    Tube_Date_Setup();
    break;   
    case 11: //Setup_Menu
    Setup_Menu();
    break; 
  }
}

void Oled_Time_Display(){
  previousMills = millis();
  DateTime now = rtc.now(); //Take time from RTC
  /**Get each number to display*/
  h_Fir = now.hour()/10;
  h_Sec = now.hour()%10;
  m_Fir = now.minute()/10;
  m_Sec = now.minute()%10;

  Oled_Time_Display_Function(h_Fir, h_Sec, m_Fir, m_Sec, true); //true to display colon
  display.display();
  while ((millis()-previousMills>0) && (millis()-previousMills<500)){
    //Detect Button
    delay(5); //Inorder to avoid button not response.
    B_State = encoder->getButton();
    if (B_State == ClickEncoder::Clicked) {
      state_Main = 1; //Jump to Oled temp display state
      break;
    }
    else if (B_State == ClickEncoder::Held){
      while(B_State == ClickEncoder::Held){ //Wait until button is released and button state is no longer in held
        B_State = encoder->getButton();
      }
      state_Main = 11; //Open Setup menu
      break;
    }
    else if (B_State == ClickEncoder::DoubleClicked){
      state_Main = 5; //Start tube display
      break;
    }
  }
  display.clearDisplay();
  
  previousMills = millis();
  Oled_Time_Display_Function(h_Fir, h_Sec, m_Fir, m_Sec, false);
  display.display();
  while ((millis()-previousMills>0) && (millis()-previousMills<500)){
    //Detect Button
    delay(5); //Inorder to avoid button not response.
    B_State = encoder->getButton();
    if (B_State == ClickEncoder::Clicked) {
      state_Main = 1; //Jump to Oled temp display state
      break;
    }
    else if (B_State == ClickEncoder::Held){
      while(B_State == ClickEncoder::Held){ //Wait until button is released and button state is no longer in held
        B_State = encoder->getButton();
      }
      state_Main = 11;
      break;
    }
    else if (B_State == ClickEncoder::DoubleClicked){
      state_Main = 5; //Start tube display
      break;
    }
  }
   display.clearDisplay();
}

void Oled_Temp_Display(){
  previousMills = millis();
  int TEMP = Clock.getTemperature();
  int TEMP_Fir = 0;
  int TEMP_Second = 0;
  display.setCursor(95,45);  // (x,y)
  if (Display_F != true)
  { 
    TEMP_Fir = TEMP/10;
    TEMP_Second = TEMP%10;
    display.println("C");  // Text or value to print  
  }
  else
  {
    TEMP_Fir = ((TEMP*9/5)+32)/10;
    TEMP_Second = ((TEMP*9/5)+32)%10;
    display.println("F");  // Text or value to print  
  }

  display.setFont(&FreeMonoBold12pt7b);  // Set a custom font
  display.setTextSize(2);
  /*First number*/
  display.setCursor(5,45);  // (x,y)
  display.println(TEMP_Fir);
  /*Second number*/
  display.setCursor(32,45);  // (x,y)
  display.println(TEMP_Second);
  /**Symbol part*/
  display.fillCircle(80, 24, 8, WHITE);
  display.fillCircle(80, 24, 4, BLACK);
  
  display.display();
  
  while ((millis()-previousMills>0) && (millis()-previousMills<500)){
    //Detect Button
    delay(5); //Inorder to avoid button not response.
    B_State = encoder->getButton();
    if (B_State == ClickEncoder::Clicked) {
      state_Main = 2; //Jump to Oled Date display state
      break;
    }
    else if (B_State == ClickEncoder::Held){
      while(B_State == ClickEncoder::Held){ //Wait until button is released and button state is no longer in held
        B_State = encoder->getButton();
      }
      state_Main = 11; //Open setup menu
      break;
    }
    else if (B_State == ClickEncoder::DoubleClicked){
      state_Main = 5; //Start tube display
      break;
    }
  }
  display.clearDisplay();
}

void Oled_Date_Display(){
  /**Get Date time*/
  previousMills = millis();
  DateTime now = rtc.now();
  int Years = now.year();
  int Month = now.month();
  int Day = now.day();

  Oled_Date_Display_Function(Years, Month, Day);
  display.display();
  
  while (((millis()-previousMills>0) && (millis()-previousMills<1000))){
    //Detect Button
    delay(5); //Inorder to avoid button not response.
    B_State = encoder->getButton();
    if (B_State == ClickEncoder::Clicked) {
      state_Main = 0; //Jump to oled time display
      break;
    }
    else if (B_State == ClickEncoder::Held){ 
      while(B_State == ClickEncoder::Held){ //Wait until button is released and button state is no longer in held
        B_State = encoder->getButton();
      }
      state_Main = 11; //Open setup menu
      break;
    }
    else if (B_State == ClickEncoder::DoubleClicked){
      state_Main = 5; //Start tube display
      break;
    }
  }
  display.clearDisplay();
}

void Oled_Time_Setup(){
  /**Get Now Time*/
  DateTime now = rtc.now();
  /**Variable for control loop*/
  bool Exit_Time_Set_loop = true;
  bool Exit_HM_Set_loop = true;
  bool which_to_Set = true;
  /**Variable for Time setup*/
  int H_to_Set = now.hour();
  int M_to_Set = now.minute();
  int Last_H = H_to_Set;
  int Last_M = M_to_Set;
  /**Setup what number should show up*/
  h_Fir = H_to_Set/10;
  h_Sec = H_to_Set%10;
  m_Fir = M_to_Set/10;
  m_Sec = Last_M%10;

  while (Exit_Time_Set_loop){
    Exit_HM_Set_loop = true;
    switch (which_to_Set) {
      /** Hour setup part */
      case true: 
      while (Exit_HM_Set_loop){ 
        previousMills = millis();
        Oled_Time_Display_Function(h_Fir, h_Sec, m_Fir, m_Sec, true);
        display.display();
        
        while ((millis()-previousMills>0) && (millis()-previousMills<1000)){
          //Detect Button
          H_to_Set += encoder->getValue();
          H_to_Set = Time_Setup_Adjust(H_to_Set,0); //0 for Hour, 1 for Minute
          
          if (H_to_Set != Last_H){ //Change display time number
            Last_H = H_to_Set;
            h_Fir = H_to_Set/10;
            h_Sec = H_to_Set%10;
            break;
          }
          /**Button detect part*/
          B_State = encoder->getButton(); //Get button state
          if (B_State == ClickEncoder::Clicked){
            Exit_HM_Set_loop = false; //Break this loop
            which_to_Set = false; //Select to change Minute
            break;
          }
          else if (B_State == ClickEncoder::DoubleClicked){
            Exit_HM_Set_loop = false; //Break this loop
            which_to_Set = false; //Break Time set loop
            break;
          }
          else if (B_State == ClickEncoder::Held){
            while(B_State == ClickEncoder::Held){ //Wait until button is released and button state is no longer in held
              B_State = encoder->getButton(); 
            }
            Exit_HM_Set_loop = false; //Break this loop
            Exit_Time_Set_loop = false; //Break Time set loop
            break;
          }
        }
        /**Button result*/
        if (Exit_HM_Set_loop == false){
          display.clearDisplay();
          break;
        }
        display.clearDisplay(); //Clear display
        
        previousMills = millis();
        Oled_Time_Display_Function(h_Fir, h_Sec, m_Fir, m_Sec, true);
        display.fillRect(0, 0, 60, 70, BLACK); //Block the selected number, Draw rectangle (x,y,width,height,color)
        display.display();

        while ((millis()-previousMills>0) && (millis()-previousMills<1000)){
          //Detect Button
          H_to_Set += encoder->getValue();
          H_to_Set = Time_Setup_Adjust(H_to_Set,0); //0 for Hour, 1 for Minute
          
          if (H_to_Set != Last_H){
            Last_H = H_to_Set;
            h_Fir = H_to_Set/10;
            h_Sec = H_to_Set%10;
            break;
          }
          /**Button detect part*/
          B_State = encoder->getButton(); //Get button state
          if (B_State == ClickEncoder::Clicked){
            Exit_HM_Set_loop = false; //Break this loop
            which_to_Set = false; //Select to change Minute
            break;
          }
          else if (B_State == ClickEncoder::DoubleClicked){
            Exit_HM_Set_loop = false; //Break this loop
            which_to_Set = false; //Break Time set loop
            break;
          }
          else if (B_State == ClickEncoder::Held){
            while(B_State == ClickEncoder::Held){ //Wait until button is released and button state is no longer in held
              B_State = encoder->getButton(); 
            }
            Exit_HM_Set_loop = false; //Break this loop
            Exit_Time_Set_loop = false; //Break Time set loop
            break;
          }
        }
        /**Button result*/
        if (Exit_HM_Set_loop == false){
          display.clearDisplay();
          break;
        }
        display.clearDisplay();
      }
      break;
      /** Minute setup part */
      case false:
      while (Exit_HM_Set_loop){ 
        previousMills = millis();
        Oled_Time_Display_Function(h_Fir, h_Sec, m_Fir, m_Sec, true);
        display.display();
        
        while ((millis()-previousMills>0) && (millis()-previousMills<1000)){
          //Detect Button
          M_to_Set += encoder->getValue();
          M_to_Set = Time_Setup_Adjust(M_to_Set,1); //0 for Hour, 1 for Minute
          
          if (M_to_Set != Last_M){
            Last_M = M_to_Set;
            m_Fir = M_to_Set/10;
            m_Sec = M_to_Set%10;
            break;
          }

          B_State = encoder->getButton(); //Get button state
          if (B_State == ClickEncoder::DoubleClicked){
            Exit_HM_Set_loop = false; //Break this loop
            which_to_Set = true; //Select to change Hour part
            break;
          }
          else if (B_State == ClickEncoder::Clicked){
            Exit_HM_Set_loop = false; //Break this loop
            which_to_Set = true; //Break Time set loop
            break;
          }
          else if (B_State == ClickEncoder::Held){
            while(B_State == ClickEncoder::Held){ //Wait until button is released and button state is no longer in held
              B_State = encoder->getButton(); 
            }
            Exit_HM_Set_loop = false; //Break this loop
            Exit_Time_Set_loop = false; //Break Time set loop
            break;
          }
        }
        if (Exit_HM_Set_loop == false){
          display.clearDisplay();
          break;
        }
        display.clearDisplay();
        
        previousMills = millis();
        Oled_Time_Display_Function(h_Fir, h_Sec, m_Fir, m_Sec, true);

        display.fillRect(72, 0, 50, 47, BLACK); //Block the selected number
        display.display();

        while ((millis()-previousMills>0) && (millis()-previousMills<1000)){
          //Detect Button
          M_to_Set += encoder->getValue();
          M_to_Set = Time_Setup_Adjust(M_to_Set,1); //0 for Hour, 1 for Minute
          
          if (M_to_Set != Last_M){
            Last_H = M_to_Set;
            m_Fir = M_to_Set/10;
            m_Sec = M_to_Set%10;
            break;
          }
          
          B_State = encoder->getButton(); //Get button state
          if (B_State == ClickEncoder::DoubleClicked){
            Exit_HM_Set_loop = false; //Break this loop
            which_to_Set = true; //Select to change Minute
            break;
          }
          else if (B_State == ClickEncoder::Clicked){
            Exit_HM_Set_loop = false; //Break this loop
            which_to_Set = true; //Break Time set loop
            break;
          }
          else if (B_State == ClickEncoder::Held){
            while(B_State == ClickEncoder::Held){ //Wait until button is released and button state is no longer in held
              B_State = encoder->getButton(); 
            }
            Exit_HM_Set_loop = false; //Break this loop
            Exit_Time_Set_loop = false; //Break Time set loop
            break;
          }
        }
        if (Exit_HM_Set_loop == false){
          display.clearDisplay();
          break;
        }
        display.clearDisplay();
      }
      break;
    }
  }

  DateTime manual = DateTime(now.year(), now.month(), now.day(), H_to_Set, M_to_Set, 0);
  rtc.adjust(manual);
  state_Main = 4; //return to time display state
}


/*Oled_Date_Setup is Not FINISH YET*/
void Oled_Date_Setup(){ 
  /**Get Date time*/
  DateTime now = rtc.now();
  int Y_to_set = now.year();
  int M_to_set = now.month();
  int D_to_set = now.day();
  /**Variable for detect number change*/
  int Last_Y = Y_to_set;
  int Last_M = M_to_set;
  int Last_D = D_to_set;
  /**Variable for control loop*/
  bool Exit_Date_Set_loop = true;
  bool Exit_YMD_Set_loop = true;
  int which_to_Set = 0; //0 for years, 1 for month, 2 for days

  while (Exit_Date_Set_loop)
  {
    Exit_YMD_Set_loop = true;
    switch (which_to_Set) 
    {
      /** Year setup */
      case 0: 
      while (Exit_YMD_Set_loop)
      {
        previousMills = millis();
        Oled_Date_Display_Function(Y_to_set, M_to_set, D_to_set);
        display.fillRect(3, 6, 55, 15, BLACK); //In order to block number
        display.display();
        
        while ((millis()-previousMills>0) && (millis()-previousMills<1000))
        {
          //Detect Button
          Y_to_set += encoder->getValue();
          if (Y_to_set < 1971) //limit the earliest year can set is 1971
          {
            Y_to_set = 1971;
          }
          
          if (Y_to_set != Last_Y){ //Change display time number
            Last_Y = Y_to_set;
            break;
          }
          /**Button detect part*/
          delay(5); //In order to prevent button nott respond
          B_State = encoder->getButton(); //Get button state
          if (B_State == ClickEncoder::Clicked){
            Exit_YMD_Set_loop = false; //Break this loop
            which_to_Set = 1; //Select to change Minute
            break;
          }
          else if (B_State == ClickEncoder::DoubleClicked){
            Exit_YMD_Set_loop = false; //Break this loop
            which_to_Set = 2; //Select to change Year
            break;
          }
          else if (B_State == ClickEncoder::Held){
            while(B_State == ClickEncoder::Held){ //Wait until button is released and button state is no longer in held
              B_State = encoder->getButton(); 
            }
            Exit_YMD_Set_loop = false; //Break this loop
            Exit_Date_Set_loop = false; //Break Time set loop
            break;
          }
        }
        if (Exit_YMD_Set_loop == false){
          display.clearDisplay();
          break;
        }
        display.clearDisplay();

        previousMills = millis();
        Oled_Date_Display_Function(Y_to_set, M_to_set, D_to_set); //Display Date on screen
        display.display();

        while ((millis()-previousMills>0) && (millis()-previousMills<1000))
        {
          //Detect Button
          Y_to_set += encoder->getValue();
          if (Y_to_set < 1971)
          {
            Y_to_set = 1971;
          }
          
          if (Y_to_set != Last_Y){ //Change display time number
            Last_Y = Y_to_set;
            break;
          }
          /**Button detect part*/
          delay(5); //In order to prevent button nott respond
          B_State = encoder->getButton(); //Get button state
          if (B_State == ClickEncoder::Clicked){
            Exit_YMD_Set_loop = false; //Break this loop
            which_to_Set = 1; //Select to change Minute
            break;
          }
          else if (B_State == ClickEncoder::DoubleClicked){
            Exit_YMD_Set_loop = false; //Break this loop
            which_to_Set = 2; //Select to change Year
            break;
          }
          else if (B_State == ClickEncoder::Held){
            while(B_State == ClickEncoder::Held){ //Wait until button is released and button state is no longer in held
              B_State = encoder->getButton(); 
            }
            Exit_YMD_Set_loop = false; //Break this loop
            Exit_Date_Set_loop = false; //Break Time set loop
            break;
          }
        }
        if (Exit_YMD_Set_loop == false){
          display.clearDisplay();
          break;
        }
        display.clearDisplay();
      }
      break;
      /** Month setup */
      case 1:
      while (Exit_YMD_Set_loop)
      {
        previousMills = millis();
        Oled_Date_Display_Function(Y_to_set, M_to_set, D_to_set);
        display.fillRect(3, 30, 55, 40, BLACK); //In order to block number
        display.display();
        
        while ((millis()-previousMills>0) && (millis()-previousMills<1000))
        {
          //Detect Button
          M_to_set += encoder->getValue();
          if (M_to_set >= 13)
          {
            M_to_set = 1;
          }
          else if (M_to_set <= 0)
          {
            M_to_set = 12;
          }
          
          if (M_to_set != Last_M){ //Change display time number
            Last_M = M_to_set;
            break;
          }
          /**Button detect part*/
          delay(5); //In order to prevent button nott respond
          B_State = encoder->getButton(); //Get button state
          if (B_State == ClickEncoder::Clicked){
            Exit_YMD_Set_loop = false; //Break this loop
            which_to_Set = 2; //Select to change Day
            break;
          }
          else if (B_State == ClickEncoder::DoubleClicked){
            Exit_YMD_Set_loop = false; //Break this loop
            which_to_Set = 0; //Select to change Year
            break;
          }
          else if (B_State == ClickEncoder::Held){
            while(B_State == ClickEncoder::Held){ //Wait until button is released and button state is no longer in held
              B_State = encoder->getButton();
            }
            Exit_YMD_Set_loop = false; //Break this loop
            Exit_Date_Set_loop = false; //Break Time set loop
            break;
          }
        }
        if (Exit_YMD_Set_loop == false){
          display.clearDisplay();
          break;
        }
        display.clearDisplay();

        previousMills = millis();
        
        Oled_Date_Display_Function(Y_to_set, M_to_set, D_to_set);
        display.display();

        while ((millis()-previousMills>0) && (millis()-previousMills<1000))
        {
          //Detect Button
          M_to_set += encoder->getValue();
          /**Month display adjust part*/
          if (M_to_set >= 13)
          {
            M_to_set = 1;
          }
          else if (M_to_set <= 0)
          {
            M_to_set = 12;
          }
          
          if (M_to_set != Last_M){ //Change display month number
            Last_M = M_to_set;
            break;
          }
          /**Button detect part*/
          delay(5); //In order to prevent button nott respond
          B_State = encoder->getButton(); //Get button state
          if (B_State == ClickEncoder::Clicked){
            Exit_YMD_Set_loop = false; //Break this loop
            which_to_Set = 2; //Select to change Minute
            break;
          }
          else if (B_State == ClickEncoder::DoubleClicked){
            Exit_YMD_Set_loop = false; //Break this loop
            which_to_Set = 0; //Select to change Year
            break;
          }
          else if (B_State == ClickEncoder::Held){
            while(B_State == ClickEncoder::Held){  //Wait until button is released and button state is no longer in held
              B_State = encoder->getButton();
            }
            Exit_YMD_Set_loop = false; //Break this loop
            Exit_Date_Set_loop = false; //Break Time set loop
            break;
          }
        }
        if (Exit_YMD_Set_loop == false){
          display.clearDisplay();
          break;
        }
        display.clearDisplay();
      }
      break;
      /** Day setup */
      case 2:
      while (Exit_YMD_Set_loop)
      {
        previousMills = millis();
        Oled_Date_Display_Function(Y_to_set, M_to_set, D_to_set);
        display.fillRect(75, 30, 55, 40, BLACK); //In order to block number
        display.display();
        
        while ((millis()-previousMills>0) && (millis()-previousMills<1000))
        {
          //Detect Button
          D_to_set += encoder->getValue();
          /**Day display adjust part*/
          if (M_to_set%2 == 0) //Even Month
          {
            if (M_to_set >=8) //8,10,12
            {
              if (D_to_set >= 32)
              {
                D_to_set = 1;
              }
              else if (D_to_set <= 0)
              {
                D_to_set = 31;
              }
            }
            else
            {
              if (D_to_set >= 31) //2,4,6
              {
                D_to_set = 1;
              }
              else if (D_to_set <= 0)
              {
                D_to_set = 30;
              }
            }
          }
          else //Odd month
          {
            if (M_to_set <=7) //1,3,5,7
            {
              if (D_to_set >= 32)
              {
                D_to_set = 1;
              }
              else if (D_to_set <= 0)
              {
                D_to_set = 31;
              }
            }
            else
            {
              if (D_to_set >= 31) //9,11
              {
                D_to_set = 1;
              }
              else if (D_to_set <= 0)
              {
                D_to_set = 30;
              }
            }
          }

          if (D_to_set != Last_D){ //Change display Day number
            Last_D = D_to_set;
            break;
          }
          /**Button detect part*/
          delay(5); //In order to prevent button nott respond
          B_State = encoder->getButton(); //Get button state
          if (B_State == ClickEncoder::Clicked){
            Exit_YMD_Set_loop = false; //Break this loop
            which_to_Set = 0; //Select to change Day
            break;
          }
          else if (B_State == ClickEncoder::DoubleClicked){
            Exit_YMD_Set_loop = false; //Break this loop
            which_to_Set = 1; //Select to change Year
            break;
          }
          else if (B_State == ClickEncoder::Held){
            while(B_State == ClickEncoder::Held){ //Wait until button is released and button state is no longer in held
              B_State = encoder->getButton();
            }
            Exit_YMD_Set_loop = false; //Break this loop
            Exit_Date_Set_loop = false; //Break Time set loop
            break;
          }
        }
        if (Exit_YMD_Set_loop == false){
          display.clearDisplay();
          break;
        }
        display.clearDisplay();

        previousMills = millis();
        
        Oled_Date_Display_Function(Y_to_set, M_to_set, D_to_set);
        display.display();

        while ((millis()-previousMills>0) && (millis()-previousMills<1000))
        {
          //Detect Button
          D_to_set += encoder->getValue();
          /**Day display adjust part*/
          if (M_to_set%2 == 0) //Even Month
          {
            if (M_to_set >=8) //8,10,12
            {
              if (D_to_set >= 32)
              {
                D_to_set = 1;
              }
              else if (D_to_set <= 0)
              {
                D_to_set = 31;
              }
            }
            else
            {
              if (D_to_set >= 31) //2,4,6
              {
                D_to_set = 1;
              }
              else if (D_to_set <= 0)
              {
                D_to_set = 30;
              }
            }
          }
          else //Odd month
          {
            if (M_to_set <=7) //1,3,5,7
            {
              if (D_to_set >= 32)
              {
                D_to_set = 1;
              }
              else if (D_to_set <= 0)
              {
                D_to_set = 31;
              }
            }
            else
            {
              if (D_to_set >= 31) //9,11
              {
                D_to_set = 1;
              }
              else if (D_to_set <= 0)
              {
                D_to_set = 30;
              }
            }
          }
          
          if (D_to_set != Last_D){ //Change display time number
            Last_D = D_to_set;
            break;
          }
          /**Button detect part*/
          delay(5); //In order to prevent button nott respond
          B_State = encoder->getButton(); //Get button state
          if (B_State == ClickEncoder::Clicked){
            Exit_YMD_Set_loop = false; //Break this loop
            which_to_Set = 0; //Select to change Minute
            break;
          }
          else if (B_State == ClickEncoder::DoubleClicked){
            Exit_YMD_Set_loop = false; //Break this loop
            which_to_Set = 1; //Select to change Year
            break;
          }
          else if (B_State == ClickEncoder::Held){
            while(B_State == ClickEncoder::Held){ //Wait until button is released and button state is no longer in held
              B_State = encoder->getButton();
            }
            Exit_YMD_Set_loop = false; //Break this loop
            Exit_Date_Set_loop = false; //Break Time set loop
            break;
          }
        }
        if (Exit_YMD_Set_loop == false){
          display.clearDisplay();
          break;
        }
        display.clearDisplay();
      }
      break;
    }
  }
  
  DateTime manual = DateTime(Y_to_set, M_to_set, D_to_set, now.hour(), now.minute(), now.second());
  rtc.adjust(manual); //set the day
  state_Main = 0; //return to time display state
}

void Close_Oled_Start_Tube() {
  /**Clear Oled displayment*/
  display.clearDisplay();
  display.display();
  /**Clear Tube displayment*/
  Tube_Display_Contorl(10, 10, 10, 10);
  if (Oled_to_Tube == true)
  {
    while (B_State == ClickEncoder::DoubleClicked)
    {
      B_State = encoder->getButton(); //Get button state
    }
    state_Main = 6; //Go to Tube time display
    Oled_to_Tube = !Oled_to_Tube;
  }
  else
  {
    while (B_State == ClickEncoder::DoubleClicked)
    {
      B_State = encoder->getButton(); //Get button state
    }
    state_Main = 0; //Go to Oled time display
    Oled_to_Tube = !Oled_to_Tube;
  } 
}

void Tube_Time_Display() {
  previousMills = millis();
  DateTime now = rtc.now(); //Take time from RTC
  h_Fir = now.hour()/10;
  h_Sec = now.hour()%10;
  m_Fir = now.minute()/10;
  m_Sec = now.minute()%10;
  Tube_Display_Contorl(h_Fir, h_Sec, m_Fir, m_Sec); //First Tube Number/ Second Tube Number/ Third Tube Nunber/ Four Tube Number
  digitalWrite(12, HIGH); //light up the colon
  while ((millis()-previousMills>0) && (millis()-previousMills<1000))
  {
    delay(5);
    B_State = encoder->getButton(); //Get button state
    if (B_State == ClickEncoder::Clicked)
    {
      state_Main = 7; //Go to Tube Temp display state
      break;
    }
    else if (B_State == ClickEncoder::DoubleClicked){
      state_Main = 5; //Go to Oled time display
      break;
    }
    else if (B_State == ClickEncoder::Held){
       while(B_State == ClickEncoder::Held){ //Wait until button is released and button state is no longer in held
        B_State = encoder->getButton();
       }
       state_Main = 9; //Go to tube setup time state
       break;
    }
  }
  digitalWrite(12, LOW); //close colon
  previousMills = millis();
  now = rtc.now(); //Take time from RTC
  h_Fir = now.hour()/10;
  h_Sec = now.hour()%10;
  m_Fir = now.minute()/10;
  m_Sec = now.minute()%10;
  Tube_Display_Contorl(h_Fir, h_Sec, m_Fir, m_Sec);

  while ((millis()-previousMills>0) && (millis()-previousMills<1000))
  {
    delay(5);
    B_State = encoder->getButton(); //Get button state
    if (B_State == ClickEncoder::Clicked)
    {
      state_Main = 7; //Go to Tube Temp display state
      break;
    }
    else if (B_State == ClickEncoder::DoubleClicked){
      state_Main = 5; //Go to Oled time display state
      break;
    }
    else if (B_State == ClickEncoder::Held){
       while(B_State == ClickEncoder::Held){ //Wait until button is released and button state is no longer in held
        B_State = encoder->getButton();
       }
       state_Main = 9; //Go to tube setup time state
       break;
    }
  }
  
  digitalWrite(12, LOW);
  
}

void Tube_Temp_Display(){
  previousMills = millis();
  int TEMP = Clock.getTemperature();
  int TEMP_Fir = Clock.getTemperature()/10;
  int TEMP_Second = TEMP%10;
  Tube_Display_Contorl(10, 10, TEMP_Fir, TEMP_Second); //First Tube Number/ Second Tube Number/ Third Tube Nunber/ Four Tube Number
  digitalWrite(12, LOW);
  while ((millis()-previousMills>0) && (millis()-previousMills<1000))
  {
    delay(5);
    B_State = encoder->getButton(); //Get button state
    if (B_State == ClickEncoder::Clicked)
    {
      state_Main = 8; //Go to tube date display state
      break;
    }
    else if (B_State == ClickEncoder::DoubleClicked){
      state_Main = 5; //Go to Oled time display state
      break;
    }
    else if (B_State == ClickEncoder::Held){
       while(B_State == ClickEncoder::Held){ //Wait until button is released and button state is no longer in held
        B_State = encoder->getButton();
       }
       state_Main = 9; //Go to tube setup time state
       break;
    }
  }

}

void Tube_Date_Display(){
  previousMills = millis();
  DateTime now = rtc.now();
  int Years = now.year();
  int Month = now.month();
  int Day = now.day();
  
  Tube_Display_Contorl(Years/1000, (Years/100)%10, (Years%100)/10, (Years%100)%10);
  digitalWrite(12, LOW);
  while (((millis()-previousMills>0) && (millis()-previousMills<3000)))
  {
    delay(5);
    B_State = encoder->getButton(); //Get button state
    if (B_State == ClickEncoder::Clicked)
    {
      state_Main = 6; //Go to Tube time display state
      break;
    }
    else if (B_State == ClickEncoder::DoubleClicked){
      state_Main = 5; //Go to Oled time display state
      break;
    }
    else if (B_State == ClickEncoder::Held){
       while(B_State == ClickEncoder::Held){ //Wait until button is released and button state is no longer in held
        B_State = encoder->getButton();
       }
       state_Main = 9; //Go to Tube setup time state
       break;
    }
  }

  if (state_Main == 8)
  {
    previousMills = millis();
    Tube_Display_Contorl(Month/10, Month%10, Day/10, Day%10);
    while (((millis()-previousMills>0) && (millis()-previousMills<3000)))
    {
      delay(5);
      B_State = encoder->getButton(); //Get button state
      if (B_State == ClickEncoder::Clicked)
      {
        state_Main = 6; //Go to Tube time display state
        break;
      }
      else if (B_State == ClickEncoder::DoubleClicked){
        state_Main = 5; //Go to Oled time display state
        break;
      }
      else if (B_State == ClickEncoder::Held){
         while(B_State == ClickEncoder::Held){ //Wait until button is released and button state is no longer in held
          B_State = encoder->getButton();
         }
         state_Main = 9; //Go to Tube setup time state
         break;
      }
    }
  }
}


void Tube_Time_Setup() {
  /**Get Now Time*/
  DateTime now = rtc.now();
  /**Variable for control loop*/
  bool Exit_Time_Set_loop = true;
  bool Exit_HM_Set_loop = true;
  bool which_to_Set = true; //True for hours, False for minute
  /**Variable for Time setup*/
  int H_to_Set = now.hour();
  int M_to_Set = now.minute();
  int Last_H = H_to_Set;
  int Last_M = M_to_Set;
  h_Fir = H_to_Set/10;
  h_Sec = H_to_Set%10;
  m_Fir = M_to_Set/10;
  m_Sec = Last_M%10;

  while (Exit_Time_Set_loop) 
  {
    Exit_HM_Set_loop = true;
    switch (which_to_Set) 
    {
      /**Hour Setup part*/
      case true:
      while (Exit_HM_Set_loop) 
      {
        previousMills = millis();
        Tube_Display_Contorl(h_Fir, h_Sec, m_Fir, m_Sec); //First Tube Number/ Second Tube Number/ Third Tube Nunber/ Four Tube Number
        digitalWrite(12,HIGH);
        while ((millis()-previousMills>0) && (millis()-previousMills<1000))
        {
          H_to_Set += encoder->getValue();
          H_to_Set = Time_Setup_Adjust(H_to_Set,0); //0 for Hour, 1 for Minute

          if (H_to_Set != Last_H)
          {
            Last_H = H_to_Set;
            h_Fir = H_to_Set/10;
            h_Sec = H_to_Set%10;
            break;
          }
          /**Detect button*/  
          delay(5);
          B_State = encoder->getButton(); //Get button state
          if (B_State == ClickEncoder::Clicked)
          {
            Exit_HM_Set_loop = false; //Break this loop
            which_to_Set = false; //Select to change Minute
            break;
          }
          else if (B_State == ClickEncoder::DoubleClicked)
          {
            Exit_HM_Set_loop = false; //Break this loop
            which_to_Set = false; //Break Time set loop
            break;
          }
          else if (B_State == ClickEncoder::Held)
          {
            while(B_State == ClickEncoder::Held){
              B_State = encoder->getButton(); //Wait until the button is released
            }
            Exit_HM_Set_loop = false; //Break this loop
            Exit_Time_Set_loop = false; //Break Time set loop
            break;
          }
        }
        /**Button result*/
        if (Exit_HM_Set_loop == false) //Button Result
        {
          break;
        }
        
        previousMills = millis();
        Tube_Display_Contorl(10, 10, m_Fir, m_Sec); //First Tube Number/ Second Tube Number/ Third Tube Nunber/ Four Tube Number
        digitalWrite(12,HIGH);
        while ((millis()-previousMills>0) && (millis()-previousMills<1000))
        {
          H_to_Set += encoder->getValue();
          H_to_Set = Time_Setup_Adjust(H_to_Set,0); //0 for Hour, 1 for Minute

          if (H_to_Set != Last_H)
          {
            Last_H = H_to_Set;
            h_Fir = H_to_Set/10;
            h_Sec = H_to_Set%10;
            break;
          } 
          /**Detect button*/  
          delay(5);
          B_State = encoder->getButton(); //Get button state
          if (B_State == ClickEncoder::Clicked)
          {
            Exit_HM_Set_loop = false; //Break this loop
            which_to_Set = false; //Select to change Minute
            break;
          }
          else if (B_State == ClickEncoder::DoubleClicked)
          {
            Exit_HM_Set_loop = false; //Break this loop
            which_to_Set = false; //Break Time set loop
            break;
          }
          else if (B_State == ClickEncoder::Held)
          {
            while(B_State == ClickEncoder::Held){
              B_State = encoder->getButton(); //Wait until the button is released
            }
            Exit_HM_Set_loop = false; //Break this loop
            Exit_Time_Set_loop = false; //Break Time set loop
            break;
          }
        }
        /**Button result*/
        if (Exit_HM_Set_loop == false) //Button Result
        {
          break;
        }
      }
      break;
      
      /**Minute Setup part*/
      case false: 
      while (Exit_HM_Set_loop) 
      {
        previousMills = millis();
        Tube_Display_Contorl(h_Fir, h_Sec, m_Fir, m_Sec); //First Tube Number/ Second Tube Number/ Third Tube Nunber/ Four Tube Number
        digitalWrite(12,HIGH);
        while ((millis()-previousMills>0) && (millis()-previousMills<1000))
        {
          M_to_Set += encoder->getValue();
          M_to_Set = Time_Setup_Adjust(M_to_Set,1); //0 for Hour, 1 for Minute

          if (M_to_Set != Last_M)
          {
            Last_M = M_to_Set;
            m_Fir = M_to_Set/10;
            m_Sec = M_to_Set%10;
            break;
          }
          /**Detect button*/ 
          delay(5);
          B_State = encoder->getButton(); //Get button state
          if (B_State == ClickEncoder::Clicked)
          {
            Exit_HM_Set_loop = false; //Break this loop
            which_to_Set = false; //Select to change Minute
            break;
          }
          else if (B_State == ClickEncoder::DoubleClicked)
          {
            Exit_HM_Set_loop = false; //Break this loop
            which_to_Set = false; //Break Time set loop
            break;
          }
          else if (B_State == ClickEncoder::Held)
          {
            while(B_State == ClickEncoder::Held){
              B_State = encoder->getButton(); //Wait until the button is released
            }
            Exit_HM_Set_loop = false; //Break this loop
            Exit_Time_Set_loop = false; //Break Time set loop
            break;
          } 
        }
        /**Button result*/
        if (Exit_HM_Set_loop == false) //Button Result
        {
          break;
        }
        previousMills = millis();
        Tube_Display_Contorl(h_Fir, h_Sec, 10, 10); //First Tube Number/ Second Tube Number/ Third Tube Nunber/ Four Tube Number
        digitalWrite(12,HIGH);
        while ((millis()-previousMills>0) && (millis()-previousMills<1000))
        {
          M_to_Set += encoder->getValue();
          M_to_Set = Time_Setup_Adjust(M_to_Set,1); //0 for Hour, 1 for Minute

          if (M_to_Set != Last_M)
          {
            Last_M = M_to_Set;
            m_Fir = M_to_Set/10;
            m_Sec = M_to_Set%10;
            break;
          }
          /**Detect button*/
          delay(5);
          B_State = encoder->getButton(); //Get button state
          if (B_State == ClickEncoder::Clicked)
          {
            Exit_HM_Set_loop = false; //Break this loop
            which_to_Set = false; //Select to change Minute
            break;
          }
          else if (B_State == ClickEncoder::DoubleClicked)
          {
            Exit_HM_Set_loop = false; //Break this loop
            which_to_Set = false; //Break Time set loop
            break;
          }
          else if (B_State == ClickEncoder::Held)
          {
            while(B_State == ClickEncoder::Held){
              B_State = encoder->getButton(); //Wait until the button is released
            }
            Exit_HM_Set_loop = false; //Break this loop
            Exit_Time_Set_loop = false; //Break Time set loop
            break;
          }   
        }
        /**Button result*/
        if (Exit_HM_Set_loop == false) //Button Result
        {
          break;
        }
      }
      break;
    }
  }
  
  digitalWrite(12,LOW);
  DateTime manual = DateTime(now.year(), now.month(), now.day(), H_to_Set, M_to_Set, 0);
  rtc.adjust(manual);
  state_Main = 6; //return to Tube time display state
}


void Setup_Menu() {
  boolean Exit_Setup_Menu = true;
  boolean Break_Loop = true;
  int where_is_Select = 0;
  int last_select = where_is_Select;
  while (Exit_Setup_Menu) {
    /**Correct Where_is_Select value*/
    if (where_is_Select >= 3) {
      where_is_Select = 0;
    }
    else if (where_is_Select <= -1)
    {
      where_is_Select = 2;
    }
    Break_Loop = true; //reinitial Break_Loop value

    switch (where_is_Select)
    {
      /**Change time display format*/
      case 0: 
      while (Break_Loop)
      {
        Oled_Setup_Menu_Display_Function();
        display.fillTriangle(110,12,120,7,120,17,WHITE); //Three point to draw triangel, x1,y1 ,x2, y2, x3, y3
        display.display();
        where_is_Select += encoder->getValue();
        if (where_is_Select != last_select)
        {
          last_select = where_is_Select;
          Break_Loop = false;
          break;
        }
        delay(5);
        B_State = encoder->getButton(); //Get button state
        if (B_State == ClickEncoder::Clicked) 
        {
          Display_12_hours = !Display_12_hours; //Change display in 12 hours or 24 hours
          Break_Loop = false;
          break;
        }
        else if (B_State == ClickEncoder::Held) 
        {
          while(B_State == ClickEncoder::Held) //Wait until the button is released and button state is no longer in held
          {
            B_State = encoder->getButton();
          }
          state_Main = 0; //Back to clock display state
          Break_Loop = false;
          Exit_Setup_Menu = false;
        }
      }
      display.clearDisplay();
      break;
      /**Change Tempeature display format*/
      case 1:
      while (Break_Loop)
      {
        Oled_Setup_Menu_Display_Function(); //Display setup menu
        display.fillTriangle(110,32,120,27,120,37,WHITE); //Three point to draw triangel
        display.display();
        where_is_Select += encoder->getValue();
        if (where_is_Select != last_select)
        {
          last_select = where_is_Select;
          Break_Loop = false;
          break;
        }
        delay(5);
        B_State = encoder->getButton(); //Get button state
        if (B_State == ClickEncoder::Clicked) 
        {
          Display_F = !Display_F; //Change display celsius or fahrenheit
          Break_Loop = false;
          break;
        }
        else if (B_State == ClickEncoder::Held) 
        {
          while(B_State == ClickEncoder::Held) //Wait until the button is released and button state is no longer in held
          {
            B_State = encoder->getButton();
          }
          state_Main = 0; //Back to clock display state
          Break_Loop = false;
          Exit_Setup_Menu = false;
        }
      }
      display.clearDisplay();
      break;
      /**Go to Time setup*/
      case 2:
      while (Break_Loop)
      {
        Oled_Setup_Menu_Display_Function();
        display.fillTriangle(110,53,120,48,120,58,WHITE); //Three point to draw triangel
        display.display();
        where_is_Select += encoder->getValue();
        if (where_is_Select != last_select)
        {
          last_select = where_is_Select;
          Break_Loop = false;
          break;
        }
        delay(5);
        B_State = encoder->getButton(); //Get button state
        if (B_State == ClickEncoder::Clicked)  //Go to time/date setup state
        {
          state_Main = 3;
          Break_Loop = false;
          Exit_Setup_Menu = false;
          break;
        }
        else if (B_State == ClickEncoder::Held) 
        {
          while(B_State == ClickEncoder::Held) //Wait until the button is released and button state is no longer in held
          {
            B_State = encoder->getButton();
          }
          state_Main = 0; //Back to clock display state
          Break_Loop = false;
          Exit_Setup_Menu = false;
        }
      }
      display.clearDisplay();
      break;
    }
  }
}
