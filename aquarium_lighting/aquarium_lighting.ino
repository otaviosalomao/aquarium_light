#include "stdio.h"
#include "RTClib.h"
#include "math.h"
#include "DallasTemperature.h"
#include "OneWire.h"
#include "SPI.h"
#include "UIPEthernet.h"
#include "Wire.h"

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; //ATRIBUIÇÃO DE ENDEREÇO MAC AO ENC28J60
byte ip[] = { 192, 168, 15, 20 }; //COLOQUE UMA FAIXA DE IP DISPONÍVEL DO SEU ROTEADOR. EX: 192.168.1.110  **** ISSO VARIA, NO MEU CASO É: 192.168.0.175
EthernetServer server(80); //PORTA EM QUE A CONEXÃO SERÁ FEITA
RTC_DS1307 RTC;

//Pins Definitions
const int TemperaturePin = 7;
const int RelayPin = 2;
const int blueLedPin = 8;
const int greenLedPin = 10;
const int redLedPin = 9;
const int whiteLedPin = 11;

OneWire ourWire(TemperaturePin);
DallasTemperature sensors(&ourWire);

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac, ip);
  server.begin();
  pinMode(RelayPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(whiteLedPin, OUTPUT);
  sensors.begin();
  Wire.begin();
  RTC.begin();
  //RTC.adjust(DateTime(__DATE__, __TIME__));
  powerOffPump();  
}
void loop() {
  DateTime now = RTC.now();  
  sunLigtCycle(now.hour(), now.minute());
  moonLightCycle(now.hour(), now.minute());
  pumpPowerCycle(now.hour());
  printTemperature();   
  EthernetShield();  
  delay(1000);
}

String readString = String(30);
int status = 0;
void HeaderHTML (EthernetClient client) {
  client.println("<html>");
  client.println("HTTP/1.1 200 OK\r\n");
  client.println("Content-Type: text/html\r\n");
  client.println("<html>");
}

void BodyHTML (EthernetClient client) {
  client.println("<body style=background-color:#ADD8E6>\r\n");
  client.println("<center><font color='blue'><h1>Aquario</font></center></h1>");
  client.println("</body>");
}

void FooterHTML(EthernetClient client) {
  client.println("</html>");
}

void EthernetShield() {
  EthernetClient client = server.available();
  Serial.println(client);
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        BodyHTML(client);
        HeaderHTML(client);
        FooterHTML(client);
        client.stop();
      }
    }
  }
}

void printTemperature() {
  sensors.requestTemperatures();
  Serial.print("Temperatura: ");
  Serial.print(sensors.getTempCByIndex(0));
  Serial.println("ºC");
}
int testHour = 0;
int testMinute = 0;
void sunLigtCycle(int currentHour, int currentMinute) {
  testHour = (testHour > 23) ? 21 : (testMinute > 60) ? testHour + 1 : testHour;
  testMinute = (testMinute <= 60) ? testMinute + 1 : 0;
  int startCycle = 5;
  int endCycle = 21;
  int startEndKelvin = 1000;
  int startEndMinutesFade = 180;
  cycle(currentHour, currentMinute, startCycle, endCycle, startEndKelvin, startEndMinutesFade);
}
void moonLightCycle(int currentHour, int currentMinute) {
  testHour = (testHour > 23) ? 21 : (testMinute > 60) ? testHour + 1 : testHour;
  testMinute = (testMinute <= 60) ? testMinute + 1 : 0;
  int startCycle = 21;
  int endCycle = 23;
  int startEndKelvin = 10000;
  int startEndMinutesFade = 30;
  cycle(currentHour, currentMinute, startCycle, endCycle, startEndKelvin, startEndMinutesFade);
}
void pumpPowerCycle(int currentHour) {
  testHour = (testHour > 23) ? 21 : (testMinute > 60) ? testHour + 1 : testHour;
  testMinute = (testMinute <= 60) ? testMinute + 1 : 0;
  int startCycle = 5;
  int endCycle = 23;
  if (currentHour >= startCycle && currentHour <= endCycle)  {
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

void printTime(int hour, int minutes, int kelvin, int brightness)
{
  DateTime now = RTC.now();  
  char daysOfTheWeek[7][12] = {"Domingo", "Segunda", "Terça", "Quarta", "Quinta", "Sexta", "Sábado"};  
  char buf[100];
  int brightnessPercentage = round(((double)brightness / 255) * 100);
  snprintf(buf, sizeof(buf), "%s %04d-%02d-%02d %02d:%02d:%02d - Temperature: %dk - Brightness: %d%%", daysOfTheWeek[now.dayOfTheWeek()], now.year(), now.month(), now.day(), hour, minutes, now.second(), kelvin, brightnessPercentage);
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
