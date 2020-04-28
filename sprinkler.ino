/*----------------------------------------
          LIBRARIES
------------------------------------------*/

/***** RTC LIBRARIES *****/
// Date and time functions using a DS1307 RTC connected via I2C
#include "RTClib.h"

/***** LCD LIBRARIES *****/
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "TouchScreen.h"

/*----------------------------------------
          DEFINITIONS
------------------------------------------*/

/***** RTC DEFINITIONS *****/
#if defined(ARDUINO_ARCH_SAMD)
// for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
#define Serial SerialUSB
#endif

RTC_DS1307 rtc;




/****** LCD DEFINITIONS ******/
// Define LCD Connections
#define TFT_DC 8
#define TFT_CS 10
#define TFT_RST 9

// Define Touchscreen Definitions
#define YP A2  // analog pin 2
#define XM A3  // analog pin 3
#define YM 7   // digital pin 7
#define XP 8   // digitial pin 8 (same as DC)

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940
#define MINPRESSURE 10
#define MAXPRESSURE 1000



/*----------------------------------------
          GLOBAL VARIABLES 
------------------------------------------*/

/***** MOISTURE SENSOR VARIABLES *****/

//Initial testing for moisture sensor. 
//Reads analog input from the moisture sensor, and determines if the soil is dry or wet, giving a recommendation to water or not
const int sensorPin = A0; //set variable name for the pin being used for the sensor
const int sensorVcc = 4; // variable for the pin used to turn the sensor on or off by setting pin 4 high or low
int senseState = HIGH; // variable for the state of the sensor. High means sensor is on, LOW turns sensor off

//Placeholders for the thresholds of detecting moisture level
int upperThresh = 550; 
int lowerThresh = 200;


/***** LCD VARIABLES *****/

struct myValve {
  int valveNum;
  boolean waterOn;
  boolean onSchedule;
  String nextDay;
  String startTime;
  int duration;
};


//State of valves used for LCD display 
myValve valve1 = {.valveNum = 1, .waterOn = false, .onSchedule = true, .nextDay = "Friday", .startTime = "13:35", .duration = 2};
myValve valve2 = {.valveNum = 2, .waterOn = false, .onSchedule = true, .nextDay = "Friday", .startTime = "13:36", .duration = 1};
myValve valve3 = {.valveNum = 3, .waterOn = false, .onSchedule = true, .nextDay = "Friday", .startTime = "13:37", .duration = 2};
myValve valve4 = {.valveNum = 4, .waterOn = false, .onSchedule = true, .nextDay = "Monday", .startTime = "13:35", .duration = 1};
myValve valve5 = {.valveNum = 5, .waterOn = false, .onSchedule = true, .nextDay = "Tuesday", .startTime = "13:36", .duration = 2};
myValve valve6 = {.valveNum = 6, .waterOn = false, .onSchedule = true, .nextDay = "Friday", .startTime = "13:37", .duration = 2};

myValve* oldValve;
myValve* currentValve = &valve1;


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);  //Adafruit graphics constructor
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);  //Touchscreen constructor




void setup() {

  //Begin serial communication with USB and microcontroller at 9600 BAUD rate
  Serial.begin(9600); //USB
  Serial1.begin(9600); //Microcontroller

  /***** MOISTURE SENSOR SETUP *****/

  pinMode(sensorPin, INPUT); //define moisture sensor as input
  pinMode(sensorVcc, OUTPUT); //set the sensor power pin as output

  /***** RTC SETUP *****/
  
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  // following line sets the RTC to the date & time this sketch was compiled
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));


  /***** LCD DEFAULT SCREEN SETUP ******/

  tft.begin();
  tft.setRotation(2);
  tft.fillScreen(ILI9341_BLACK);

  displayValveSelection(); //Display current valve 
  if (currentValve->onSchedule) { displaySchedule(); }
  else { displayOverride(); }
  if (currentValve->waterOn) { displayOffButton(); }
  else { displayOnButton(); }
  displayResetButton(); //Display reset button
}



void loop() {

  
  /***** RTC LOOP******/

  //retreive current time from rtc
  DateTime now = rtc.now();


 
  checkSchedule(&now, &valve1);
  checkSchedule(&now, &valve2);  
  checkSchedule(&now, &valve3);
  checkSchedule(&now, &valve4);
  checkSchedule(&now, &valve5);
  checkSchedule(&now, &valve6);


  /***** LCD DISPLAY AND TOUCH LOOP******/

  // Detect touches on screen
  TSPoint p = ts.getPoint();  // Retrieve a point
  if (p.z < MINPRESSURE || p.z > MAXPRESSURE) {  //Register no touch if outside of pressure threshold
    return;
  }
  // Scale from ~0->1000 to tft.width using the calibration #'s
  p.x = map(p.x, TS_MAXX, TS_MINX, 0, tft.width()); // invert x axis to match rotation of display
  p.y = map(p.y, TS_MAXY, TS_MINY, 0, tft.height());  // invert y axis to match rotation of display


  // If touch within ON/OFF button borders, send ON/OFF signal
  if ((p.x >= 10 && p.x <= tft.width()-20) && (p.y >= 75 && p.y <= 175)) {
    if (currentValve->waterOn) { //Change to OFF button
      Serial.println("Turn OFF");
      tft.drawRect(10, 75, tft.width() - 20, 100, ILI9341_RED);
      delay(150);
      displayOnButton();
      currentValve->waterOn = false;
      currentValve->onSchedule = false;
      displayOverride(); // Display override text

      
 
      //ENTER CODE TO SEND TO MICROCONTROLLER
    }
    else { //Change to ON button
      Serial.println("Turn ON");
      tft.drawRect(10, 75, tft.width() - 20, 100, ILI9341_RED);
      delay(150);
      displayOffButton();
      currentValve->waterOn = true;
      currentValve->onSchedule = false;
      displayOverride(); // Display override text

      //ENTER CODE TO SEND TO MICROCONTROLLER
    }
  }
  // If touch within RESET borders, cancel override and reset to original schedule
  if ((p.x >= 10 && p.x <= tft.width()-20) && (p.y >= tft.height()/2+25 && p.y <= tft.height()/2+105)) {
    Serial.println("RESET");
    tft.drawRect(10, tft.height()/2 + 25, tft.width() - 20, 80, ILI9341_WHITE);
    delay(150);
    tft.drawRect(10, tft.height()/2 + 25, tft.width() - 20, 80, ILI9341_RED);
    currentValve->onSchedule = true;
    displaySchedule(); // Display reset text, back to current watering schedule
    
    //ENTER CODE TO SEND TO MICROCONTROLLER

  }

  // Code to select different valves
  if (p.y >= tft.height()-40) {
    oldValve = currentValve;
    if (p.x < tft.width()/6) { currentValve = &valve1; }
    else if (p.x < 2*tft.width()/6) { currentValve = &valve2; }
    else if (p.x < 3*tft.width()/6) { currentValve = &valve3; }
    else if (p.x < 4*tft.width()/6) { currentValve = &valve4; }
    else if (p.x < 5*tft.width()/6) { currentValve = &valve5; }
    else { currentValve = &valve6; }
    
    if (oldValve->valveNum != currentValve->valveNum) {
      displayValveSelection(); //Display current valve 
      if (currentValve->onSchedule) { displaySchedule(); }
      else { displayOverride(); }
      if (currentValve->waterOn) { displayOffButton(); }
      else { displayOnButton(); }
      displayResetButton(); //Display reset button
    }
  }

/*CODE FOR SENDING DATA TO MICROCONTROLLER*/

  int outgoingByte = 0;
  if(!currentValve->waterOn) { 
    outgoingByte = ((currentValve->valveNum) * 2);
    Serial1.write(outgoingByte); 
  }
  else { 
    outgoingByte = 1 + ((currentValve->valveNum) * 2);
    Serial1.write(outgoingByte); 
  }
}



// Check moisture sensor function
// returns 0 if too dry, 1 if too moist, 2 if within appropriate range
int checkMoisture() {
  //If about to water, check if moisture is within thresholds
  digitalWrite(sensorVcc, HIGH); // assigns turns the sensor on
  delay(100);
  int moisture = analogRead(sensorPin); //reads value from sensor

  Serial.print("\nMoisture Level: "); //print moisture reading
  Serial.println(moisture);


  if(moisture < lowerThresh) //detects if lawn is too dry
  {
    Serial.println("Water your lawn!");
    return 0;
  }
  else if (moisture > upperThresh) //detects if lawn is too wet
  {
    //Turn off valve for corresponding zone; serial comm
    Serial.println("It's too wet!");
    return 1;
  }
  else
  {
    Serial.println("Just right!");
    return 2;
  }
  digitalWrite(sensorVcc, LOW); // turns the sensor back off after checking
}

void displayValveSelection() {
  
  // Draw borders for each valve button
  for (int i = 0; i <= 6; i++) {
      tft.drawRect(i * tft.width()/6, tft.height()-40, 40, 40, ILI9341_GREEN);
  }
  
  // Label each valve button with number of valve
  tft.setTextSize(3);
  tft.setTextColor(ILI9341_YELLOW);
  for (int i = 0; i < 6; i++) {
    tft.setCursor(12 + 40*i, tft.height()-30);
    tft.println(i+1);
  }
  
  //Highlight new valve selection
  tft.drawRect((currentValve->valveNum-1) * tft.width()/6, tft.height()-40, 40, 40, ILI9341_WHITE);
  
  //Display new selected valve number 
  tft.fillRect(0, 0, tft.width(), 40, ILI9341_BLACK);
  tft.setCursor(tft.width() / 4, 5);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(3);
  tft.println("Valve");
  tft.setCursor(tft.width()/4 + 100, 5);
  tft.println(currentValve->valveNum);
}

void displaySchedule() {
  tft.fillRect(0, 40, tft.width(), 20, ILI9341_BLACK);
  tft.setCursor(25, 40);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("Currently scheduled to water on:");
  tft.setCursor(65, 50);
  tft.println(currentValve->nextDay);
  tft.setCursor(130, 50);
  tft.println(currentValve->startTime);
}

void displayOverride() {
  tft.fillRect(0, 40, tft.width(), 20, ILI9341_BLACK);
  tft.setCursor(30, 40);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("Watering scheduling overridden!");
}

void displayOnButton() {
  tft.fillRect(10, 75, tft.width() - 20, 100, ILI9341_WHITE);
  tft.setCursor(80, 105);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(7);
  tft.println("ON");
}

void displayOffButton() {
  tft.fillRect(10, 75, tft.width() - 20, 100, ILI9341_WHITE);
  tft.setCursor(65, 105);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(7);
  tft.println("OFF");
}

void displayResetButton() {
  tft.fillRect(10, tft.height()/2 + 25, tft.width() - 20, 80, ILI9341_RED);
  tft.setCursor(50, 205);
  tft.setTextSize(5);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("RESET");
}

String getValue(String data, char separator, int index)
{
 int found = 0;
  int strIndex[] = {
0, -1  };
  int maxIndex = data.length()-1;
  for(int i=0; i<=maxIndex && found<=index; i++){
  if(data.charAt(i)==separator || i==maxIndex){
  found++;
  strIndex[0] = strIndex[1]+1;
  strIndex[1] = (i == maxIndex) ? i+1 : i;
  }
 }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

int checkSchedule (DateTime* now, myValve* valve) {
  String valveHourString = getValue(valve->startTime, ':', 0);
  String valveMinuteString = getValue(valve->startTime, ':', 1);
  int valveHour = valveHourString.toInt();
  int valveMinute = valveMinuteString.toInt();
  int valveDuration = valve->duration; 
  String valveDay = valve->nextDay;
  String daysOfTheWeek[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  int outgoingByte = 0;


  if((daysOfTheWeek[now->dayOfTheWeek()] == valveDay & now->hour() == valveHour & now->minute() >= valveMinute & now->minute() <= (valveMinute + valveDuration)) & (valve->onSchedule)) {  //If time matches and schedule is not overriden
    Serial.println("Time match");
    int moistureLevel = checkMoisture();
    
    if (moistureLevel == 1) {  //lawn is wet, don't water
      valve->waterOn = false;
      outgoingByte = ((valve->valveNum) * 2);
      Serial.println("On schedule but moisture levels incorrect. Water OFF");
    }
    else {  //begin watering
        valve->waterOn = true;
        outgoingByte = 1 + ((valve->valveNum) * 2);
        Serial.println("On schedule and moisture levels correct. Water ON");
    }
    if (now->minute() == valveMinute + valveDuration) {
      valve->waterOn = false;
      outgoingByte = ((valve->valveNum) * 2);
    } 
  }
    
    Serial1.write(outgoingByte);
  
}
 
