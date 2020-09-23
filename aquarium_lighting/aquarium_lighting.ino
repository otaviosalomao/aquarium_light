#include "stdio.h"
#include "DS1302.h"
#include "math.h"

//Global Variables
int redLedIntensity = 0;
int greenLedIntensity = 0;
int blueLedIntensity = 0;
int WhiteLedIntensity = 0;
int brightness = 0;
int testHour = 0;
int testMinute = 0;
int kelvin = 0;

//Constants
const int startHourCycle = 5;
const int endHourCycle = 21;
const int startEndFadeBrightnessMinutes = 180;
const int startKelvin = 1000;

//Pins Definitions
const int ClkPino = 16;
const int DatPino = 15;
const int RstPino = 14;
const int blueLedPin = 8;
const int greenLedPin = 9;
const int redLedPin = 10;
const int whiteLedPin = 11;

DS1302 rtc(RstPino, DatPino, ClkPino);

void setup() {
  Serial.begin(9600);  
  pinMode(blueLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(whiteLedPin, OUTPUT);
}

void loop() {  
  Time t = rtc.time();
  testHour = (testHour > 21) ? startHourCycle : (testMinute > 60) ? testHour + 1 : testHour;
  testMinute = (testMinute <= 60) ? testMinute + 1 : 0;
  timeCycle(t.hr, t.min);
  //timeCycle(testHour, testMinute);
  delay(1000);
}

String dayAsString(const Time::Day day) {
  switch (day) {
    case Time::kSunday: return "Domingo";
    case Time::kMonday: return "Segunda-Feira";
    case Time::kTuesday: return "Terca-Feira";
    case Time::kWednesday: return "Quarta-Feira";
    case Time::kThursday: return "Quinta-Feira";
    case Time::kFriday: return "Sexta-Feira";
    case Time::kSaturday: return "Sabado";
  }
}
void setTime() {
  rtc.writeProtect(false);
  rtc.halt(false);
  Time t(2020, 9, 23, 07, 8, 00, Time::kWednesday);
  rtc.time(t);
}
void printTime()
{
  Time t = rtc.time();
  const String day = dayAsString(t.day);
  char buf[100];
  float brightnessPercentage = (((double)brightness / 255) * 100);
  Serial.println(brightnessPercentage);
  snprintf(buf, sizeof(buf), "%s %04d-%02d-%02d %02d:%02d:%02d - Temperature: %dk - Brightness: %f %", day.c_str(), t.yr, t.mon, t.date, t.hr, t.min, t.sec, kelvin, brightnessPercentage);
  Serial.println(buf);  
}
void powerOnLight (int hours, int minutes) {
  setBrightness(hours, minutes);
  setKelvin(hours, minutes);
  setRGBW();
  analogWrite(blueLedPin, blueLedIntensity);
  analogWrite(redLedPin, redLedIntensity);
  analogWrite(greenLedPin, greenLedIntensity);
  analogWrite(whiteLedPin, WhiteLedIntensity);
}
void powerOffLight () {
  analogWrite(blueLedPin, 0);
  analogWrite(redLedPin, 0);
  analogWrite(greenLedPin, 0);
  analogWrite(whiteLedPin, 0);
}
int setKelvin(int hours, int minutes) {
  int halfCycleHour = (endHourCycle - startHourCycle) / 2;
  if (hours < (halfCycleHour + startHourCycle)) {
    kelvin = (startKelvin * (hours - startHourCycle)) + (round(startKelvin / 60) * minutes);
  } else if (hours > (halfCycleHour + startHourCycle)) {
    kelvin = (startKelvin * (endHourCycle - hours)) - (round(startKelvin / 60) * minutes);
  } else {
    kelvin = startKelvin * halfCycleHour;
  }
}
void timeCycle(int hours, int minutes) {   
  printTime();
  if (hours >= startHourCycle && hours <= endHourCycle) {
    powerOnLight(hours, minutes);
  } else {
    powerOffLight();    
  }
}
void setBrightness(int hour, int minute) {
  int startEndFadeBrightnessHours = startEndFadeBrightnessMinutes / 60;
  if ((hour - startHourCycle) < startEndFadeBrightnessHours) {
    int minuteBrightness = ((hour - startHourCycle) * 60) + minute;
    brightness = map(minuteBrightness, 0, startEndFadeBrightnessMinutes, 0, 255);
  } else if ((endHourCycle - hour) <= startEndFadeBrightnessHours) {
    int minuteBrightness = ((endHourCycle - hour) * 60) - minute;
    brightness = map(minuteBrightness, startEndFadeBrightnessMinutes, 0, 255, 0);
  } else {
    brightness = 255;
  }
  brightness = max(min(brightness, 255), 0);
}

void setRGBW() {
  redLedIntensity = map(redFromKelvin(kelvin), 0, 255, 0, brightness);
  greenLedIntesity = map(greenFromKelvin(kelvin), 0, 255, 0, brightness);;  blueLedIntesity = map(blueFromKelvin(kelvin), 0, 255, 0, brightness);;
  whiteLedIntesity = map(whiteFromKelvin(kelvin), 0, 255, 0, brightness);;
}

int whiteFromKelvin(int kelvin) {
  int temperature = kelvin / 100;
  int  whiteValue = (temperature < 65) ? map(temperature, 0, 65, 0, 255) : map(temperature, 65, 120, 255, 0);
  return max(min(whiteValue, 255), 0);
}

int greenFromKelvin(int kelvin) {
  int temperature = kelvin / 100;
  int greenValue = 0;
  if (temperature <= 65) {
    greenValue = round((99.4708025861 * log(temperature) - 161.1195681661));
  } else {
    greenValue = round(288.1221695283 * pow(temperature - 60, -0.0755148492));
  }
  return max(min(greenValue, 255), 0);
}

int blueFromKelvin(int kelvin) {
  int temperature = kelvin / 100;
  int blueValue = 0;
  if (temperature > 20) {
    blueValue = round(138.5177312231 * log(temperature - 10) - 305.0447927307);
  }
  return max(min(blueValue, 255), 0);
}

int redFromKelvin(int kelvin) {
  int temperature = kelvin / 100;
  int redValue = 255;
  if (temperature > 65) {
    redValue = round(329.698727446 * pow(temperature - 60, -0.1332047592));
  }
  return max(min(redValue, 255), 0);
}
