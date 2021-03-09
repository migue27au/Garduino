/*
 * GARDUINO PROGRAM
 * Author: Migue27au
 * 
 * Description: this code is the controller of a automated watering systems. That 
 * reads sensors variables (temperature, time, humidity, soil humidity and luminosity)
 * and decide if a plant needs water or not.
 * The maximum number of water pumps is 2 and the watering variables can be modified
 * for each pump.
 * 
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <virtuabotixRTC.h>
#include <EEPROM.h>//Se incluye la librería EEPROM
#include <DHT.h>

#define DHTTYPE DHT11


//LCD CONST
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define BMP_HEIGHT   32
#define BMP_WIDTH    16

static const unsigned char PROGMEM bmp[] =
{
0x01, 0xEC, 0x1B, 0xC6, 0x36, 0x33, 0x24, 0x9B, 0x0D, 0xC2, 0x2D, 0x96, 0x34, 0x36, 0x33, 0xEE,
0x39, 0x1C, 0x1C, 0xD8, 0x05, 0xE0, 0x00, 0x78, 0x01, 0x70, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
0x03, 0x00, 0x07, 0x00, 0x1E, 0x00, 0x3E, 0x00, 0x7F, 0x00, 0x7B, 0x00, 0x61, 0xE0, 0x41, 0xF0,
0x01, 0xF8, 0x01, 0x7C, 0x01, 0x3C, 0x02, 0x08, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00
};

//MENU 
#define NO_MENU 0
#define MENU 1
#define MENU_CHANGE_TIME 2
#define MENU_CHANGE_DATE 3
#define MENU_CHANGE_SOIL_1 4
#define MENU_CHANGE_SOIL_2 5
#define MENU_CHANGE_TEMPERATURE_1 6
#define MENU_CHANGE_TEMPERATURE_2 7
#define MENU_CHANGE_HUMIDITY_1 8
#define MENU_CHANGE_HUMIDITY_2 9
#define MENU_CHANGE_LUMINOSITY_1 10
#define MENU_CHANGE_LUMINOSITY_2 11
#define MENU_CHANGE_TANK_SIZE 12

//PROGRAM CONST
#define MIN_YEAR 2021
#define MIN_WATER_LEVEL 15
#define DEFAULT_VALUE 50
#define WATERING_TIME 10 
#define IS_WRITTEN 7777
const float soundSpeed = 34000.0;

//EEPROM ADDRESSES
#define SOIL1_ADDRESS_BOOL 0
#define SOIL2_ADDRESS_BOOL 48
#define TEMPERATURE1_ADDRESS_BOOL 96
#define TEMPERATURE2_ADDRESS_BOOL 144
#define HUMIDITY1_ADDRESS_BOOL 192
#define HUMIDITY2_ADDRESS_BOOL 240
#define LUMINOSITY1_ADDRESS_BOOL 288
#define LUMINOSITY2_ADDRESS_BOOL 336
#define TANK_SIZE_ADDRESS_BOOL 384
#define SOIL1_ADDRESS 432
#define SOIL2_ADDRESS 480
#define TEMPERATURE1_ADDRESS 528
#define TEMPERATURE2_ADDRESS 576
#define HUMIDITY1_ADDRESS 624
#define HUMIDITY2_ADDRESS 672
#define LUMINOSITY1_ADDRESS 720
#define LUMINOSITY2_ADDRESS 768
#define TANK_SIZE_ADDRESS 816

//PINS
const uint8_t BOMB1 = 2;
const uint8_t BOMB2 = 3;
const uint8_t JOYSTICK_X = A0;
const uint8_t JOYSTICK_Y = A1;
const uint8_t LED_BLUE = 9;
const uint8_t LED_YELLOW = 10;
const uint8_t LED_BUTTON = 13;
const uint8_t BUTTON = 5;
const uint8_t SOIL1 = A2;
const uint8_t SOIL2 = A6;

const uint8_t LDR = A7;
const uint8_t DHT_PIN = 4;
const uint8_t TRIGGER = 12;
const uint8_t ECHO = 11;

//OBJECTS
int dayOfMonths[] = {31,28,31,30,31,30,31,31,30,31,30,31};
struct time {
  int day;
  int month;
  int year;
  int hours;
  int minutes;
  int seconds;
} time;

DHT dht(DHT_PIN, DHTTYPE);
virtuabotixRTC myRTC(6, 7, 8);

//VARIABLES
int pos = 0;
int value = 0;
int valueNegative = 0;
bool buttonPulsed = false;
int menu = NO_MENU;

//TRIGGERS AND CHANGE TRIGGERS VARIABLES
int changeHour;
int changeMinute;
int changeDay = 1;
int changeMonth = 1;
int changeYear = MIN_YEAR;
int changeSoil1Trigger;
int changeSoil2Trigger;
int changeTemperature1Trigger;
int changeTemperature2Trigger;
int changeHumidity1Trigger;
int changeHumidity2Trigger;
int changeLuminosity1Trigger;
int changeLuminosity2Trigger;
int changeTankSize;

int soil1Trigger = DEFAULT_VALUE;
int soil2Trigger = DEFAULT_VALUE;
int temperature1Trigger = DEFAULT_VALUE;
int temperature2Trigger = DEFAULT_VALUE;
int humidity1Trigger = DEFAULT_VALUE;
int humidity2Trigger = DEFAULT_VALUE;
int luminosity1Trigger = DEFAULT_VALUE;
int luminosity2Trigger = DEFAULT_VALUE;
int tankSize = DEFAULT_VALUE;


//SENSOR VARIABLES
int soil1;
int soil2;
int luminosity;
int temperature;
int humidity;
int waterLevel;


//WATERING PUMPS VARIABLES
bool enableBomb1 = false;
bool enableBomb2 = false;
bool watering1 = false;
bool watering2 = false;

void setup() {
  Serial.begin(9600);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_BUTTON, OUTPUT);
  pinMode(BOMB1, OUTPUT);
  pinMode(BOMB2, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  dht.begin();
  getParamaters();
  
  displayHello();
  
  display.clearDisplay();
}

void loop() {
  readSensors();
  needWater();
  getTime();
  
  if(waterLevel > MIN_WATER_LEVEL){
    analogWrite(LED_BLUE, LOW);
  } else{
    analogWrite(LED_BLUE, 50);
    enableBomb1 = false;
    enableBomb2 = false;
  }
  
  waterPlants();

  //UI
  updatePos();
  updateValue();
  readButton();
  
  switch(menu){
    case NO_MENU:
      displayInfo();
      if(buttonPulsed){
        menu = MENU;
      }
    break;
    case MENU:
      displayMenu();
      updateValueNegative();
      if(buttonPulsed){
        switch(valueNegative){
          case 0:
            menu = NO_MENU;
            value = 0;
          break;
          case -1:
            menu = MENU_CHANGE_TIME;
          break;
          case -2:
            menu = MENU_CHANGE_DATE;
          break;
          case -3:
            menu = MENU_CHANGE_SOIL_1;
            value = soil1Trigger;
          break;
          case -4:
            menu = MENU_CHANGE_SOIL_2;
            value = soil2Trigger;
          break;
          case -5:
            menu = MENU_CHANGE_TEMPERATURE_1;
            value = temperature1Trigger;
          break;
          case -6:
            menu = MENU_CHANGE_TEMPERATURE_2;
            value = temperature2Trigger;
          break;
          case -7:
            menu = MENU_CHANGE_HUMIDITY_1;
            value = humidity1Trigger;
          break;
          case -8:
            menu = MENU_CHANGE_HUMIDITY_2;
            value = humidity2Trigger;
          break;
          case -9:
            menu = MENU_CHANGE_LUMINOSITY_1;
            value = luminosity1Trigger;
          break;
          case -10:
            menu = MENU_CHANGE_LUMINOSITY_2;
            value = luminosity2Trigger;
          break;
          case -11:
            menu = MENU_CHANGE_TANK_SIZE;
            value = tankSize;
          break;
        }
        valueNegative = 0;
        pos = 0;
      }
    break;
    case MENU_CHANGE_TIME:
      displayChangeTime();
    break;
    case MENU_CHANGE_DATE:
      displayChangeDate();
    break;
    case MENU_CHANGE_SOIL_1:
      displayChangeSoi1Trigger();
    break;
    case MENU_CHANGE_SOIL_2:
      displaychangeSoil2Trigger();
    break;
    case MENU_CHANGE_TEMPERATURE_1:
      displayChangeTemperature1Trigger();
    break;
    case MENU_CHANGE_TEMPERATURE_2:
      displayChangeTemperature2Trigger();
    break;
    case MENU_CHANGE_HUMIDITY_1:
      displayChangeHumidity1Trigger();
    break;
    case MENU_CHANGE_HUMIDITY_2:
      displayChangeHumidity2Trigger();
    break;
    case MENU_CHANGE_LUMINOSITY_1:
      displayChangeLuminosity1Trigger();
    break;
    case MENU_CHANGE_LUMINOSITY_2:
      displayChangeLuminosity2Trigger();
    break;
    case MENU_CHANGE_TANK_SIZE:
      displayChangeTankSize();
    break;
  }  
}

void displayInfo(){
  display.clearDisplay();
  display.setTextSize(1);
  
  display.setCursor(0,0);
  display.println(getDateString());

  display.setCursor(96,0);
  display.println(getTimeString());

  display.setCursor(0,10);
  display.println(F("T"));
  display.setCursor(4,10);
  display.println(F(":"));
  display.setCursor(10,10);
  display.println(temperature);

  display.setCursor(48+0,10);
  display.println(F("H"));
  display.setCursor(48+4,10);
  display.println(F(":"));
  display.setCursor(48+10,10);
  display.println(humidity);

  display.setCursor(92+0,10);
  display.println(F("D"));
  display.setCursor(92+4,10);
  display.println(F(":"));
  display.setCursor(92+10,10);
  display.println(waterLevel);
  
  display.setCursor(0,19);
  display.println(F("H1"));
  display.setCursor(10,19);
  display.println(F(":"));
  display.setCursor(14,19);
  display.println(soil1);
  
  display.setCursor(48+0,19);
  display.println(F("H2"));
  display.setCursor(48+10,19);
  display.println(F(":"));
  display.setCursor(48+14,19);
  display.println(soil2);
  
   display.setCursor(92+0,19);
  display.println(F("L"));
  display.setCursor(92+4,19);
  display.println(F(":"));
  display.setCursor(92+10,19);
  display.println(luminosity);

  display.display();
}

void displayHello(){
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  display.setTextSize(2);
  
  display.setCursor(0,8);
  display.println(F("G"));
  delay(100);
  display.display();
  display.setCursor(2*6,8);
  display.println(F("A"));
  delay(100);
  display.display();
  display.setCursor(2*12,8);
  display.println(F("R"));
  delay(100);
  display.display();
  display.setCursor(2*18,8);
  display.println(F("D"));
  delay(100);
  display.display();
  display.setCursor(2*24,8);
  display.println(F("U"));
  delay(100);
  display.display();
  display.setCursor(2*30,8);
  display.println(F("I"));
  delay(100);
  display.display();
  display.setCursor(2*36,8);
  display.println(F("N"));
  delay(100);
  display.display();
  display.setCursor(2*42,8);
  display.println(F("O"));
  delay(100);
  display.display();
  delay(100);
  
  display.drawBitmap(112,0, bmp, BMP_WIDTH, BMP_HEIGHT, 1);
  display.display();
  
  delay(5000);
}


void displayMenu(){
  display.clearDisplay();
  
  if(valueNegative > -4){
    display.setCursor(0,(0-valueNegative)*8);
    display.println(F(">"));
  } else if(valueNegative > -8){
    display.setCursor(42,(0-(valueNegative+4))*8);
    display.println(F(">"));
  }else if(valueNegative > -12){
    display.setCursor(84,(0-(valueNegative+8))*8);
    display.println(F(">"));
  } else if(valueNegative < -11){
    valueNegative = -11;
  }
  if(valueNegative > 0){
    valueNegative = 0;
  }
  
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setCursor(8,0); 
  display.println(F("Q")); //quit
  display.setCursor(8,8); 
  display.println(F("T")); //change time
  display.setCursor(8,16); 
  display.println(F("D"));  //change date
  display.setCursor(8,24); 
  display.println(F("S1"));  //change h1
  display.setCursor(8+42,0); 
  display.println(F("S2"));  //change h2
  display.setCursor(8+42,8); 
  display.println(F("T1"));  //
  display.setCursor(8+42,16); 
  display.println(F("T2"));
  display.setCursor(8+42,24); 
  display.println(F("H1"));
  display.setCursor(8+84,0); 
  display.println(F("H2"));
  display.setCursor(8+84,8);
  display.println(F("L1"));
  display.setCursor(8+84,16);
  display.println(F("L2"));
  display.setCursor(8+84,24);
  display.println(F("TS"));

  display.display();
}

void displayChangeTime(){
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(F("Change time: "));
  display.setTextSize(2);
  
  if(pos < 0){
    pos = 0;
  } else if(pos > 3){
    pos = 3;
  }

  switch(pos){
    case 0:
      display.setCursor(0, 16);
      display.println(F("__"));
      if(value > 23){
        value = 23;
      }
      changeHour = value;
    break;
    case 1:
      display.setCursor(16+8+4, 16);
      display.println(F("__"));
      if(value > 59){
        value = 59;
      }
      changeMinute = value;
    break;
    case 2:
      display.setCursor(16+8+4+16+8, 16);
      display.println(F("_"));
      if(buttonPulsed){
        myRTC.setDS1302Time(0, changeMinute, changeHour, 0, time.day, time.month, time.year); // SS, MM, HH, DW, DD, MM, YYYY
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
    case 3:
      display.setCursor(16+8+4+16+16+8, 16);
      display.println(F("_"));
      if(buttonPulsed){
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
  }

  display.setCursor(0, 10);
  display.println(changeHour);
  
  display.setCursor(16+4, 10);
  display.println(F(":"));
  
  display.setCursor(16+8+4, 10);
  display.println(changeMinute);

  display.setCursor(16+8+4, 10);
  display.println(changeMinute);

  display.setCursor(16+8+4, 10);
  display.println(changeMinute);

  display.setTextSize(1);
  display.setCursor(16+8+4+16+8, 14);
  display.println(F("OK"));
  
  display.setCursor(16+8+4+16+16+8, 14);
  display.println(F("X"));
  
  
  display.display();
}

void displayChangeTankSize(){
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(F("Change tank size:"));
  display.setTextSize(2);
  
  if(pos < 0){
    pos = 0;
  } else if(pos > 2){
    pos = 2;
  }

  switch(pos){
    case 0:
      display.setCursor(0, 16);
      display.println(F("___"));
      if(value > 200){
        value = 200;
      }
      changeTankSize = value;
    break;
    case 1:
      display.setCursor(10*3+8, 16);
      display.println(F("_"));
      if(buttonPulsed){
        updateTankSize();
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
    case 2:
      display.setCursor(10*3+8+12, 16);
      display.println(F("_"));
      if(buttonPulsed){
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
  }

  display.setCursor(0, 10);
  display.println(changeTankSize);
  
  display.setTextSize(1);
  display.setCursor(10*3+8, 14);
  display.println(F("OK"));
  
  display.setCursor(10*3+8+16, 14);
  display.println(F("X"));
  
  
  display.display();
}

void displayChangeSoi1Trigger(){
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(F("Change soil 1:"));
  display.setTextSize(2);
  
  if(pos < 0){
    pos = 0;
  } else if(pos > 2){
    pos = 2;
  }

  switch(pos){
    case 0:
      display.setCursor(0, 16);
      display.println(F("___"));
      if(value > 100){
        value = 100;
      }
      changeSoil1Trigger = value;
    break;
    case 1:
      display.setCursor(10*3+8, 16);
      display.println(F("_"));
      if(buttonPulsed){
        updateSoil1Trigger();
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
    case 2:
      display.setCursor(10*3+8+12, 16);
      display.println(F("_"));
      if(buttonPulsed){
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
  }

  display.setCursor(0, 10);
  display.println(changeSoil1Trigger);
  
  display.setTextSize(1);
  display.setCursor(10*3+8, 14);
  display.println(F("OK"));
  
  display.setCursor(10*3+8+16, 14);
  display.println(F("X"));
  
  
  display.display();
}

void displaychangeSoil2Trigger(){
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(F("Change soil 2:"));
  display.setTextSize(2);
  
  if(pos < 0){
    pos = 0;
  } else if(pos > 2){
    pos = 2;
  }

  switch(pos){
    case 0:
      display.setCursor(0, 16);
      display.println(F("___"));
      if(value > 100){
        value = 100;
      }
      changeSoil2Trigger = value;
    break;
    case 1:
      display.setCursor(10*3+8, 16);
      display.println(F("_"));
      if(buttonPulsed){
        updateSoil2Trigger();
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
    case 2:
      display.setCursor(10*3+8+12, 16);
      display.println(F("_"));
      if(buttonPulsed){
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
  }

  display.setCursor(0, 10);
  display.println(changeSoil2Trigger);
  
  display.setTextSize(1);
  display.setCursor(10*3+8, 14);
  display.println(F("OK"));
  
  display.setCursor(10*3+8+16, 14);
  display.println(F("X"));
  
  
  display.display();
}

void displayChangeTemperature1Trigger(){
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(F("Change temperature 1:"));
  display.setTextSize(2);
  
  if(pos < 0){
    pos = 0;
  } else if(pos > 2){
    pos = 2;
  }

  switch(pos){
    case 0:
      display.setCursor(0, 16);
      display.println(F("___"));
      if(value > 100){
        value = 100;
      }
      changeTemperature1Trigger = value;
    break;
    case 1:
      display.setCursor(10*3+8, 16);
      display.println(F("_"));
      if(buttonPulsed){
        updateTemperature1Trigger();
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
    case 2:
      display.setCursor(10*3+8+12, 16);
      display.println(F("_"));
      if(buttonPulsed){
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
  }

  display.setCursor(0, 10);
  display.println(changeTemperature1Trigger);
  
  display.setTextSize(1);
  display.setCursor(10*3+8, 14);
  display.println(F("OK"));
  
  display.setCursor(10*3+8+16, 14);
  display.println(F("X"));
  
  
  display.display();
}

void displayChangeTemperature2Trigger(){
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(F("Change temperature 2:"));
  display.setTextSize(2);
  
  if(pos < 0){
    pos = 0;
  } else if(pos > 2){
    pos = 2;
  }

  switch(pos){
    case 0:
      display.setCursor(0, 16);
      display.println(F("___"));
      if(value > 100){
        value = 100;
      }
      changeTemperature2Trigger = value;
    break;
    case 1:
      display.setCursor(10*3+8, 16);
      display.println(F("_"));
      if(buttonPulsed){
        updateTemperature2Trigger();
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
    case 2:
      display.setCursor(10*3+8+12, 16);
      display.println(F("_"));
      if(buttonPulsed){
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
  }

  display.setCursor(0, 10);
  display.println(changeTemperature2Trigger);
  
  display.setTextSize(1);
  display.setCursor(10*3+8, 14);
  display.println(F("OK"));
  
  display.setCursor(10*3+8+16, 14);
  display.println(F("X"));
  
  
  display.display();
}

void displayChangeHumidity1Trigger(){
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(F("Change humidity 1:"));
  display.setTextSize(2);
  
  if(pos < 0){
    pos = 0;
  } else if(pos > 2){
    pos = 2;
  }

  switch(pos){
    case 0:
      display.setCursor(0, 16);
      display.println(F("___"));
      if(value > 100){
        value = 100;
      }
      changeHumidity1Trigger = value;
    break;
    case 1:
      display.setCursor(10*3+8, 16);
      display.println(F("_"));
      if(buttonPulsed){
        updateHumidity1Trigger();
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
    case 2:
      display.setCursor(10*3+8+12, 16);
      display.println(F("_"));
      if(buttonPulsed){
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
  }

  display.setCursor(0, 10);
  display.println(changeHumidity1Trigger);
  
  display.setTextSize(1);
  display.setCursor(10*3+8, 14);
  display.println(F("OK"));
  
  display.setCursor(10*3+8+16, 14);
  display.println(F("X"));
  
  
  display.display();
}

void displayChangeHumidity2Trigger(){
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(F("Change humidity 2:"));
  display.setTextSize(2);
  
  if(pos < 0){
    pos = 0;
  } else if(pos > 2){
    pos = 2;
  }

  switch(pos){
    case 0:
      display.setCursor(0, 16);
      display.println(F("___"));
      if(value > 100){
        value = 100;
      }
      changeHumidity2Trigger = value;
    break;
    case 1:
      display.setCursor(10*3+8, 16);
      display.println(F("_"));
      if(buttonPulsed){
        updateHumidity2Trigger();
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
    case 2:
      display.setCursor(10*3+8+12, 16);
      display.println(F("_"));
      if(buttonPulsed){
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
  }

  display.setCursor(0, 10);
  display.println(changeHumidity2Trigger);
  
  display.setTextSize(1);
  display.setCursor(10*3+8, 14);
  display.println(F("OK"));
  
  display.setCursor(10*3+8+16, 14);
  display.println(F("X"));
  
  
  display.display();
}

void displayChangeLuminosity1Trigger(){
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(F("Change luminosity 1:"));
  display.setTextSize(2);
  
  if(pos < 0){
    pos = 0;
  } else if(pos > 2){
    pos = 2;
  }

  switch(pos){
    case 0:
      display.setCursor(0, 16);
      display.println(F("___"));
      if(value > 100){
        value = 100;
      }
      changeLuminosity1Trigger = value;
    break;
    case 1:
      display.setCursor(10*3+8, 16);
      display.println(F("_"));
      if(buttonPulsed){
        updateLuminosity1Trigger();
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
    case 2:
      display.setCursor(10*3+8+12, 16);
      display.println(F("_"));
      if(buttonPulsed){
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
  }

  display.setCursor(0, 10);
  display.println(changeLuminosity1Trigger);
  
  display.setTextSize(1);
  display.setCursor(10*3+8, 14);
  display.println(F("OK"));
  
  display.setCursor(10*3+8+16, 14);
  display.println(F("X"));
  
  
  display.display();
}

void displayChangeLuminosity2Trigger(){
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(F("Change luminosity 2:"));
  display.setTextSize(2);
  
  if(pos < 0){
    pos = 0;
  } else if(pos > 2){
    pos = 2;
  }

  switch(pos){
    case 0:
      display.setCursor(0, 16);
      display.println(F("___"));
      if(value > 100){
        value = 100;
      }
      changeLuminosity2Trigger = value;
    break;
    case 1:
      display.setCursor(10*3+8, 16);
      display.println(F("_"));
      if(buttonPulsed){
        updateLuminosity2Trigger();
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
    case 2:
      display.setCursor(10*3+8+12, 16);
      display.println(F("_"));
      if(buttonPulsed){
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
  }

  display.setCursor(0, 10);
  display.println(changeLuminosity2Trigger);
  
  display.setTextSize(1);
  display.setCursor(10*3+8, 14);
  display.println(F("OK"));
  
  display.setCursor(10*3+8+16, 14);
  display.println(F("X"));
  
  
  display.display();
}



void displayChangeDate(){
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(F("Change date: "));
  display.setTextSize(2);
  
  if(pos < 0){
    pos = 0;
  } else if(pos > 4){
    pos = 4;
  }

  switch(pos){
    case 0:
      display.setCursor(0, 16);
      display.println(F("____"));
      if(value > 50){
        value = 50;
      }
      changeYear = value + MIN_YEAR;
    break;
    case 1:
      display.setCursor(10*4+12, 16);
      display.println(F("__"));
      if(value > 11){
        value = 11;
      }
      changeMonth = value + 1;
    break;
    case 2:
      display.setCursor(10*4+8+10*2+12, 16);
      display.println(F("__"));
      if(value > dayOfMonths[changeMonth-1]-1){
        value = dayOfMonths[changeMonth-1]-1;
      }
      changeDay = value + 1;
    break;
    case 3:
      display.setCursor(10*4+8+10*2+8+10*2+8, 16);
      display.println(F("_"));
      if(buttonPulsed){
        myRTC.setDS1302Time(time.seconds, time.minutes, time.hours,0 , changeDay-1, changeMonth-1, changeYear); // SS, MM, HH, DW, DD, MM, YYYY
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
    case 4:
      display.setCursor(10*4+8+10*2+8+10*2+8+12, 16);
      display.println(F("_"));
      if(buttonPulsed){
        menu = MENU;
        value = 0;
        pos = 0;
      }
    break;
  }

  display.setCursor(0, 10);
  display.println(changeYear);
  
  display.setCursor(10*4+4, 10);
  display.println(F(":"));
  
  display.setCursor(10*4+12, 10);
  display.println(changeMonth);
  
  display.setCursor(10*4+8+10*2+4, 10);
  display.println(F(":"));

  display.setCursor(10*4+8+10*2+12, 10);
  display.println(changeDay);

  display.setTextSize(1);
  display.setCursor(10*4+8+10*2+8+10*2+8, 14);
  display.println(F("OK"));
  
  display.setCursor(10*4+8+10*2+8+10*2+8+16, 14);
  display.println(F("X"));
  
  
  display.display();
}


void getTime(){
  myRTC.updateTime();

  time.day = myRTC.dayofmonth+1;
  time.month = myRTC.month+1;
  time.year = myRTC.year;
  time.hours = myRTC.hours;
  time.minutes = myRTC.minutes;
  time.seconds = myRTC.seconds;
}

String getTimeString(){
  String s = "";
  if(time.hours < 10){
    s += "0";
  }
  s += (String) time.hours + ":";
  if(time.minutes < 10){
    s += "0";
  }
  s += (String)time.minutes;

  return s;
}


String getDateString(){
  String s = "";
  if(time.day < 10){
    s += "0";
  }
  s += (String) time.day + "/";
  if(time.month < 10){
    s += "0";
  }
  s += (String)time.month + "/";
  s += (String)time.year;

  return s;
}

int readX(){
  int x = analogRead(JOYSTICK_X);
    
  if(x < 100)
    return -1;
  if(x > 725)
    return 1;
  return 0;
}

int readY(){
  int y = analogRead(JOYSTICK_Y);
    
  if(y < 100)
    return -1;
  if(y > 725)
    return 1;
  return 0;
}

void updatePos(){
  int x = readX();
  if(x < 0 && pos > 0){
    pos--;
    delay(100);
  } else if(x > 0){
    pos++;
    delay(100);
  }

  //Serial.print("pos = ");
  //Serial.println(pos);
}

void updateValue(){
  int y = readY();
  if(y < 0 && value > 0){
    value--;
    delay(100);
  } else if(y > 0){
    value++;
    delay(100);
  }

  //Serial.print("value = ");
  //Serial.println(value);
}

void updateValueNegative(){
  int y = readY();
  if(y < 0){
    valueNegative--;
    delay(100);
  } else if(y > 0){
    valueNegative++;
    delay(100);
  }

  //Serial.print("value = ");
  //Serial.println(value);
}

void readButton(){
  if(digitalRead(BUTTON) == false){
    digitalWrite(LED_BUTTON, HIGH);
    buttonPulsed = true;
    delay(200);
  } else {
    digitalWrite(LED_BUTTON, LOW);
    buttonPulsed = false;
  }
}

void setTrigger()
{
  digitalWrite(TRIGGER, LOW);
  delayMicroseconds(2);
  
  // Ponemos el pin Trigger a estado alto y esperamos 10 ms
  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10);
  
  // Comenzamos poniendo el pin Trigger en estado bajo
  digitalWrite(TRIGGER, LOW);
}

void readSensors(){
  soil1 = map(analogRead(SOIL1),300,700,100,0);
  soil2 = map(analogRead(SOIL2),300,700,100,0);
  
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(h) || isnan(t)) {
    t = 0;
    h = 0;
  }
  temperature = (int)t;
  humidity = (int)h;
  //log_x (n) = log(n) / log(x)
  luminosity = map(analogRead(LDR),0,1023,100,0);

  unsigned long echoTime = pulseIn(ECHO, HIGH);
  
  float dist = echoTime * 0.000001 * soundSpeed / 2.0;
  //Serial.print("dist: ");
  //Serial.println(dist);
  //Serial.print("waterLevel: ");
  //Serial.println(waterLevel);
  waterLevel = ((tankSize-dist)/tankSize)*100;
  //Serial.print("waterLevel: ");
  //Serial.println(waterLevel);
  //Serial.print("humedad1: ");
  //Serial.println(soil1);
  //Serial.print("humedad2: ");
  //Serial.println(soil2);
  //Serial.print("humedad: ");
  //Serial.println(humidity);
  //Serial.print("temperatura: ");
  //Serial.println(temperature);
  //Serial.print("luz: ");
  //Serial.println(luminosity);
  //Serial.print("nivel: ");
  //Serial.println(waterLevel);
}

void updateSoil1Trigger(){
  EEPROM.put(SOIL1_ADDRESS, changeSoil1Trigger);
  soil1Trigger = changeSoil1Trigger;
  int checkWritten = 0;
  EEPROM.get(SOIL1_ADDRESS_BOOL, checkWritten);
  if(checkWritten != IS_WRITTEN)
    EEPROM.put(SOIL1_ADDRESS_BOOL, IS_WRITTEN);
}

void updateSoil2Trigger(){
  EEPROM.put(SOIL2_ADDRESS, changeSoil2Trigger);
  soil2Trigger = changeSoil2Trigger;
  int checkWritten = 0;
  EEPROM.get(SOIL2_ADDRESS_BOOL, checkWritten);
  if(checkWritten != IS_WRITTEN)
    EEPROM.put(SOIL2_ADDRESS_BOOL, IS_WRITTEN);
}

void updateTemperature1Trigger(){
  EEPROM.put(TEMPERATURE1_ADDRESS, changeTemperature1Trigger);
  temperature1Trigger = changeTemperature1Trigger;
  int checkWritten = 0;
  EEPROM.get(TEMPERATURE1_ADDRESS_BOOL, checkWritten);
  if(checkWritten != IS_WRITTEN)
    EEPROM.put(TEMPERATURE1_ADDRESS_BOOL, IS_WRITTEN);
}

void updateTemperature2Trigger(){
  EEPROM.put(TEMPERATURE2_ADDRESS, changeTemperature2Trigger);
  temperature2Trigger = changeTemperature2Trigger;
  int checkWritten = 0;
  EEPROM.get(TEMPERATURE2_ADDRESS_BOOL, checkWritten);
  if(checkWritten != IS_WRITTEN)
    EEPROM.put(TEMPERATURE2_ADDRESS_BOOL, IS_WRITTEN);
}

void updateHumidity1Trigger(){
  EEPROM.put(HUMIDITY1_ADDRESS, changeHumidity1Trigger);
  humidity1Trigger = changeHumidity1Trigger;
  int checkWritten = 0;
  EEPROM.get(HUMIDITY1_ADDRESS_BOOL, checkWritten);
  if(checkWritten != IS_WRITTEN)
    EEPROM.put(HUMIDITY1_ADDRESS_BOOL, IS_WRITTEN);
}

void updateHumidity2Trigger(){
  EEPROM.put(HUMIDITY2_ADDRESS, changeHumidity2Trigger);
  humidity2Trigger = changeHumidity2Trigger;
  int checkWritten = 0;
  EEPROM.get(HUMIDITY2_ADDRESS_BOOL, checkWritten);
  if(checkWritten != IS_WRITTEN)
    EEPROM.put(HUMIDITY2_ADDRESS_BOOL, IS_WRITTEN);
}

void updateLuminosity1Trigger(){
  EEPROM.put(LUMINOSITY1_ADDRESS, changeLuminosity1Trigger);
  luminosity1Trigger = changeLuminosity1Trigger;
  int checkWritten = 0;
  EEPROM.get(LUMINOSITY1_ADDRESS_BOOL, checkWritten);
  if(checkWritten != IS_WRITTEN)
    EEPROM.put(LUMINOSITY1_ADDRESS_BOOL, IS_WRITTEN);
}
void updateLuminosity2Trigger(){
  EEPROM.put(LUMINOSITY2_ADDRESS, changeLuminosity2Trigger);
  luminosity2Trigger = changeLuminosity2Trigger;
  int checkWritten = 0;
  EEPROM.get(LUMINOSITY2_ADDRESS_BOOL, checkWritten);
  if(checkWritten != IS_WRITTEN)
    EEPROM.put(LUMINOSITY2_ADDRESS_BOOL, IS_WRITTEN);
}
void updateTankSize(){
  EEPROM.put(TANK_SIZE_ADDRESS, changeTankSize);
  tankSize = changeTankSize;
  int checkWritten = 0;
  EEPROM.get(TANK_SIZE_ADDRESS_BOOL, checkWritten);
  if(checkWritten != IS_WRITTEN)
    EEPROM.put(TANK_SIZE_ADDRESS_BOOL, IS_WRITTEN);
}

void getParamaters(){
  int checkWritten = 0;
  EEPROM.get(SOIL1_ADDRESS_BOOL, checkWritten);
  if(checkWritten == IS_WRITTEN)
    EEPROM.get(SOIL1_ADDRESS, soil1Trigger);
  EEPROM.get(SOIL2_ADDRESS_BOOL, checkWritten);
  if(checkWritten == IS_WRITTEN)
    EEPROM.get(SOIL2_ADDRESS, soil2Trigger);
  EEPROM.get(TEMPERATURE1_ADDRESS_BOOL, checkWritten);
  if(checkWritten == IS_WRITTEN)
   EEPROM.get(TEMPERATURE1_ADDRESS, temperature1Trigger);
  EEPROM.get(TEMPERATURE2_ADDRESS_BOOL, checkWritten);
  if(checkWritten == IS_WRITTEN)
    EEPROM.get(TEMPERATURE2_ADDRESS, temperature2Trigger);
  EEPROM.get(HUMIDITY1_ADDRESS_BOOL, checkWritten);
  if(checkWritten == IS_WRITTEN)
    EEPROM.get(HUMIDITY1_ADDRESS, humidity1Trigger);
  EEPROM.get(HUMIDITY2_ADDRESS_BOOL, checkWritten);
  if(checkWritten == IS_WRITTEN)
    EEPROM.get(HUMIDITY2_ADDRESS, humidity2Trigger);
  EEPROM.get(LUMINOSITY1_ADDRESS_BOOL, checkWritten);
  if(checkWritten == IS_WRITTEN)
    EEPROM.get(LUMINOSITY1_ADDRESS, luminosity1Trigger);
  EEPROM.get(LUMINOSITY2_ADDRESS_BOOL, checkWritten);
  if(checkWritten == IS_WRITTEN)
    EEPROM.get(LUMINOSITY2_ADDRESS, luminosity2Trigger);
  EEPROM.get(TANK_SIZE_ADDRESS_BOOL, checkWritten);
  if(checkWritten == IS_WRITTEN)
    EEPROM.get(TANK_SIZE_ADDRESS, tankSize);
}

void needWater(){
  //Serial.println("Hay que regar?");
  //Plant1
  if(soil1 < soil1Trigger && temperature < temperature1Trigger && 
  humidity < humidity1Trigger && luminosity < luminosity1Trigger){
    //Serial.println("PLANTA 1 NECESITA AGUA");
    enableBomb1 = true;
  } else {
    enableBomb1 = false;
  }

  //Plant2
  if(soil2 < soil2Trigger && temperature < temperature2Trigger && 
  humidity < humidity2Trigger && luminosity < luminosity2Trigger){
    //Serial.println("PLANTA 2 NECESITA AGUA");
    enableBomb2 = true;
  } else {
    enableBomb2 = false;
  }
}

void waterPlants(){
  if(enableBomb1){
    //Serial.println("ACTIVAR BOMBA 1");
    if(watering1 == false && time.seconds == 0){
      //Serial.println("BOMBA 1 activada");
      watering1 = true;
    }
  }
  if(enableBomb2 == true && enableBomb1 == false){
    //Serial.println("ACTIVAR BOMBA 2");
    if(watering2 == false && time.seconds == 0){
      //Serial.println("BOMBA 2 activada");
      watering2 = true;
    }
  }

  if(watering1 == true){ 
    if(time.seconds < WATERING_TIME){
      digitalWrite(BOMB1, HIGH);
      analogWrite(LED_YELLOW, 50);
    } else {
      digitalWrite(BOMB1, LOW);
      analogWrite(LED_YELLOW, 0);
      watering1 = false;
      enableBomb1 = false;
    }
  }
  if(watering2 == true){ 
    if(time.seconds < WATERING_TIME){
      digitalWrite(BOMB2, HIGH);
      analogWrite(LED_YELLOW, 50);
    } else {
      digitalWrite(BOMB2, LOW);
      analogWrite(LED_YELLOW, 0);
      watering2 = false;
      enableBomb2 = false;
    }
  }
}
