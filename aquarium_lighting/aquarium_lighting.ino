#include "stdio.h"
#include "DS1302.h"
#include "math.h"

//Pins Definitions
const int ClkPin = 16;
const int DatPin = 15;
const int RstPin = 14;
const int RelayPin = 2;
const int blueLedPin = 8;
const int greenLedPin = 10;
const int redLedPin = 9;
const int whiteLedPin = 11;

DS1302 rtc(RstPin, DatPin, ClkPin);

void setup() {
  Serial.begin(9600);
  pinMode(RelayPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(whiteLedPin, OUTPUT);
  powerOffPump();
}
void loop() {
  Time t = rtc.time();
  sunLigtCycle(t);
  moonLightCycle(t);
  pumpPowerCycle(t);
  delay(1000);
}
int testHour = 0;
int testMinute = 0;
void sunLigtCycle(Time t) {
  testHour = (testHour > 23) ? 21 : (testMinute > 60) ? testHour + 1 : testHour;
  testMinute = (testMinute <= 60) ? testMinute + 1 : 0;
  int startCycle = 5;
  int endCycle = 21;
  int startEndKelvin = 1000;
  int startEndMinutesFade = 180;
  cycle(t.hr, t.min, startCycle, endCycle, startEndKelvin, startEndMinutesFade);
}
void moonLightCycle(Time t) {
  testHour = (testHour > 23) ? 21 : (testMinute > 60) ? testHour + 1 : testHour;
  testMinute = (testMinute <= 60) ? testMinute + 1 : 0;
  int startCycle = 21;
  int endCycle = 23;
  int startEndKelvin = 10000;
  int startEndMinutesFade = 30;
  cycle(t.hr, t.min, startCycle, endCycle, startEndKelvin, startEndMinutesFade);
}
void pumpPowerCycle(Time t) {
  testHour = (testHour > 23) ? 21 : (testMinute > 60) ? testHour + 1 : testHour;
  testMinute = (testMinute <= 60) ? testMinute + 1 : 0;
  int startCycle = 5;
  int endCycle = 23;
  if (t.hr >= startCycle && t.hr <= endCycle)  {
    powerOnPump();
  } else {
    powerOffPump();
  }
}
void cycle(int currentHour, int currentMinute, int startHour, int endHour, int startKelvin, int startEndFade) {
  int currentMinutesTime = (currentHour * 60) + currentMinute;
  int startMinutesCycle = startHour * 60;
  int endMinutesCycle = endHour * 60;
  if (currentMinutesTime >= startMinutesCycle && currentMinutesTime <= endMinutesCycle) {
    int brightness = fadeBrightness(currentHour, currentMinute, startHour, endHour, startEndFade, startEndFade);
    int kelvin = cycleKelvin(currentHour, currentMinute, startHour, endHour, startKelvin);
    printTime(currentHour, currentMinute, kelvin, brightness);
    powerOnLight(kelvin, brightness);
  }
}
int cycleKelvin(int hours, int minutes, int startCycleHour, int endCycleHour, int startKelvin) {
  int halfCycleHour = (endCycleHour - startCycleHour) / 2;
  if (hours < (halfCycleHour + startCycleHour)) {
    return (startKelvin * ((hours - startCycleHour) + 1)) + (round(startKelvin / 60) * minutes);
  } else if (hours >= (halfCycleHour + startCycleHour)) {
    return (startKelvin * ((endCycleHour - hours) + 1)) - (round(startKelvin / 60) * minutes);
  } else {
    return startKelvin * halfCycleHour;
  }
}
int fadeBrightness(int hour, int minute, int startCycleHour, int endCycleHour, int startMinutesFade, int endMinutesFade) {
  int brightness = 255;
  int currentStartCycleMinutes = ((hour - startCycleHour) * 60) + minute;
  int currentEndCycleMinutes = ((endCycleHour - hour) * 60 - minute);
  if (currentStartCycleMinutes < startMinutesFade) {
    brightness = map(currentStartCycleMinutes, 0, endMinutesFade, 0, 255);
  } else if (currentEndCycleMinutes <= endMinutesFade) {
    brightness = map(currentEndCycleMinutes, endMinutesFade, 0, 255, 0);
  }
  return max(min(brightness, 255), 0);
}
void powerOnLight (int kelvin, int brightness) {
  int redLedIntensity = map(redFromKelvin(kelvin), 0, 255, 0, brightness);
  int greenLedIntensity = map(greenFromKelvin(kelvin), 0, 255, 0, brightness);
  int blueLedIntensity = map(blueFromKelvin(kelvin), 0, 255, 0, brightness);
  int whiteLedIntensity = map(whiteFromKelvin(kelvin), 0, 255, 0, brightness);
  analogWrite(blueLedPin, blueLedIntensity);
  analogWrite(redLedPin, redLedIntensity);
  analogWrite(greenLedPin, greenLedIntensity);
  analogWrite(whiteLedPin, whiteLedIntensity);
}
void powerOnPump() {
  digitalWrite(RelayPin, LOW);
}

void powerOffPump() {
  digitalWrite(RelayPin, HIGH);
}
void powerOffLight () {
  analogWrite(blueLedPin, 0);
  analogWrite(redLedPin, 0);
  analogWrite(greenLedPin, 0);
  analogWrite(whiteLedPin, 0);
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
  Time t(2020, 9, 28, 18, 37, 00, Time::kMonday);
  rtc.time(t);
}
void printTime(int hour, int minutes, int kelvin, int brightness)
{
  Time t = rtc.time();
  const String day = dayAsString(t.day);
  char buf[100];
  int brightnessPercentage = round(((double)brightness / 255) * 100);
  snprintf(buf, sizeof(buf), "%s %04d-%02d-%02d %02d:%02d:%02d - Temperature: %dk - Brightness: %d%%", day.c_str(), t.yr, t.mon, t.date, hour, minutes, t.sec, kelvin, brightnessPercentage);
  Serial.println(buf);
}
void printFloat(float number) {
  static char outstr[15];
  char buf[20];
  dtostrf(number, 7, 2, outstr);
  snprintf(buf, sizeof(buf), "%s%%", outstr);
  Serial.println(buf);
}
void TestPinLeds() {
  analogWrite(blueLedPin, 0);
  analogWrite(redLedPin, 0);
  analogWrite(greenLedPin, 0);
  analogWrite(whiteLedPin, 0);
}
