#include "stdio.h"
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include "math.h"
#include "DallasTemperature.h"
#include "OneWire.h"
#include "SPI.h"
#include "UIPEthernet.h"
#include "Wire.h"


//Pins Definitions
const int TemperaturePin = 2;
const int HeaterPin = 5;
const int PumpPin = 6;
const int BlueLedPin = 10;
const int GreenLedPin = 11;
const int RedLedPin = 12;
const int WhiteLedPin = 13;
const int RTCDatPin = 8;
const int RTCCLKPin = 9;
const int RTCRSTPin = 7;

//Declarations
EthernetServer server(80); //PORTA EM QUE A CONEXÃO SERÁ FEITA
ThreeWire myWire(RTCDatPin, RTCCLKPin, RTCRSTPin); // DAT, CLK, RST
RtcDS1302<ThreeWire> Rtc(myWire);
OneWire ourWire(TemperaturePin);
DallasTemperature sensors(&ourWire);

//Constants
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; //ATRIBUIÇÃO DE ENDEREÇO MAC AO ENC28J60
byte ip[] = { 192, 168, 15, 20 }; //IP Ethernet Shield
const int MinWaterTemperature = 25;
const int SunLightStartTime = 5;
const int SunLightEndTime = 21;
const int SunLightStartEndKelvin = 1000;
const int SunLigtStartEndMinutesFade = 420;
const int MoonLightStartTime = 21;
const int MoonLightEndTime = 23;
const int MoonLightStartEndKelvin = 10000;
const int MoonLigtStartEndMinutesFade = 30;
const float MaxBrightnessPercentage = 80;

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac, ip);
  pinMode(BlueLedPin, OUTPUT);
  pinMode(RedLedPin, OUTPUT);
  pinMode(GreenLedPin, OUTPUT);
  pinMode(WhiteLedPin, OUTPUT);
  SetTime();
  sensors.begin();
  Wire.begin();
  Rtc.Begin();
  server.begin();
}

void loop() {
  RtcDateTime now = Rtc.GetDateTime();
  int hour = now.Hour();
  int minute = now.Minute();
  float temperature = WaterTemperature();
  //SunLight
  CycleControl(hour, minute, SunLightStartTime, SunLightEndTime, SunLightStartEndKelvin, SunLigtStartEndMinutesFade);
  //MoonLight
  CycleControl(hour, minute, MoonLightStartTime, MoonLightEndTime, MoonLightStartEndKelvin, MoonLigtStartEndMinutesFade);
  //Pump
  PumpControl(hour);
  //Heater
  HeaterControl(temperature, MinWaterTemperature);
  printTemperature();
  printTime(now.Year(), now.Month(), now.Day(), now.DayOfWeek(), now.Hour(), now.Minute(), now.Second());
  EthernetShield();
  delay(1000);
}

void CycleControl(int currentHour, int currentMinute, int startHour, int endHour, int startKelvin, int startEndFade) {
  int currentMinutesTime = (currentHour * 60) + currentMinute;
  int startMinutesCycle = startHour * 60;
  int endMinutesCycle = endHour * 60;
  if (currentMinutesTime >= startMinutesCycle && currentMinutesTime <= endMinutesCycle) {
    int brightness = fadeBrightness(currentHour, currentMinute, startHour, endHour, startEndFade, startEndFade);
    int kelvin = cycleKelvin(currentHour, currentMinute, startHour, endHour, startKelvin);
    printLightInformation(kelvin, brightness);
    powerOnLight(kelvin, brightness);
  }
}

void PumpControl(int currentHour) {
  if (currentHour % 2 == 0)  {
    powerOnPump();
  } else {
    powerOffPump();
  }
}

void HeaterControl(float temperature, float minTemperature) {
  if (temperature < 50 & temperature > 10) {
    if (temperature < (minTemperature + (minTemperature * 0.03))) {
      powerOnHeater();
    }
    else if (temperature > minTemperature) {
      powerOffHeater();
    }
  }
}

float WaterTemperature() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
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
  int maxBrightness = round(255 * (MaxBrightnessPercentage / 100));
  int brightness = maxBrightness;
  int currentStartCycleMinutes = ((hour - startCycleHour) * 60) + minute;
  int currentEndCycleMinutes = ((endCycleHour - hour) * 60 - minute);
  if (currentStartCycleMinutes < startMinutesFade) {
    brightness = map(currentStartCycleMinutes, 0, endMinutesFade, 0, maxBrightness);
  } else if (currentEndCycleMinutes <= endMinutesFade) {
    brightness = map(currentEndCycleMinutes, endMinutesFade, 0, maxBrightness, 0);
  }
  return max(min(brightness, maxBrightness), 0);
}

void powerOnLight (int kelvin, int brightness) {
  int redLedIntensity = map(redFromKelvin(kelvin), 0, 255, 0, brightness);
  int greenLedIntensity = map(greenFromKelvin(kelvin), 0, 255, 0, brightness);
  int blueLedIntensity = map(blueFromKelvin(kelvin), 0, 255, 0, brightness);
  int whiteLedIntensity = map(whiteFromKelvin(kelvin), 0, 255, 0, brightness);
  analogWrite(BlueLedPin, blueLedIntensity);
  analogWrite(RedLedPin, redLedIntensity);
  analogWrite(GreenLedPin, greenLedIntensity);
  analogWrite(WhiteLedPin, whiteLedIntensity);
}

void powerOnPump() {
  pinMode(PumpPin, OUTPUT);
  digitalWrite(PumpPin, LOW);
  Serial.println("PUMP STATUS: UP");
}

void powerOnHeater() {
  pinMode(HeaterPin, OUTPUT);
  digitalWrite(HeaterPin, LOW);
  Serial.println("HEATER STATUS: UP");
}

void powerOffHeater() {
  digitalWrite(HeaterPin, HIGH);
  Serial.println("HEATER STATUS: DOWN");
}

void powerOffPump() {
  digitalWrite(PumpPin, HIGH);
  Serial.println("PUMP STATUS: DOWN");
}

void powerOffLight () {
  analogWrite(BlueLedPin, 0);
  analogWrite(RedLedPin, 0);
  analogWrite(GreenLedPin, 0);
  analogWrite(WhiteLedPin, 0);
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

void printTime(int year, int month, int day, int dayOfWeek, int hour, int minute, int second)
{
  char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  char buf[100];
  snprintf(buf, sizeof(buf), "%s %04d-%02d-%02d %02d:%02d:%02d", daysOfTheWeek[dayOfWeek], year, month, day, hour, minute, second);
  Serial.println(buf);
}

void printTemperature() {
  sensors.requestTemperatures();
  printFloat("Water Temperature: ", sensors.getTempCByIndex(0), "ºC");
}

void printLightInformation(int kelvin, int brightness)
{
  char buf[100];
  int brightnessPercentage = round(((double)brightness / 255) * 100);
  snprintf(buf, sizeof(buf), "Color Temperature(K): %dk - Brightness: %d%%", kelvin, brightnessPercentage);
  Serial.println(buf);
}

void printFloat(char previousStr[100], float number, char afterStr[100]) {
  static char outstr[15];
  char buf[100];
  dtostrf(number, 7, 2, outstr);
  snprintf(buf, sizeof(buf), "%s%s%s", previousStr, outstr, afterStr);
  Serial.println(buf);
}

void SetTime() {
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  if (Rtc.GetIsWriteProtected()) { //SE O RTC ESTIVER PROTEGIDO CONTRA GRAVAÇÃO, FAZ
    Serial.println("RTC está protegido contra gravação. Habilitando a gravação agora..."); //IMPRIME O TEXTO NO MONITOR SERIAL
    Rtc.SetIsWriteProtected(false); //HABILITA GRAVAÇÃO NO RTC
    Serial.println(); //QUEBRA DE LINHA NA SERIAL
  }
  if (!Rtc.GetIsRunning()) { //SE RTC NÃO ESTIVER SENDO EXECUTADO, FAZ
    Serial.println("RTC não está funcionando de forma contínua. Iniciando agora..."); //IMPRIME O TEXTO NO MONITOR SERIAL
    Rtc.SetIsRunning(true); //INICIALIZA O RTC
    Serial.println(); //QUEBRA DE LINHA NA SERIAL
  }
  RtcDateTime now = Rtc.GetDateTime(); //VARIÁVEL RECEBE INFORMAÇÕES
  if (now < compiled) { //SE A INFORMAÇÃO REGISTRADA FOR MENOR QUE A INFORMAÇÃO COMPILADA, FAZ
    Serial.println("As informações atuais do RTC estão desatualizadas. Atualizando informações..."); //IMPRIME O TEXTO NO MONITOR SERIAL
    Rtc.SetDateTime(compiled); //INFORMAÇÕES COMPILADAS SUBSTITUEM AS INFORMAÇÕES ANTERIORES
    Serial.println(); //QUEBRA DE LINHA NA SERIAL
  }
  else if (now > compiled) { //SENÃO, SE A INFORMAÇÃO REGISTRADA FOR MAIOR QUE A INFORMAÇÃO COMPILADA, FAZ
    Serial.println("As informações atuais do RTC são mais recentes que as de compilação. Isso é o esperado."); //IMPRIME O TEXTO NO MONITOR SERIAL
    Serial.println(); //QUEBRA DE LINHA NA SERIAL
  }
  else if (now == compiled) { //SENÃO, SE A INFORMAÇÃO REGISTRADA FOR IGUAL A INFORMAÇÃO COMPILADA, FAZ
    Serial.println("As informações atuais do RTC são iguais as de compilação! Não é o esperado, mas está tudo OK."); //IMPRIME O TEXTO NO MONITOR SERIAL
    Serial.println(); //QUEBRA DE LINHA NA SERIAL
  }
}
void TestPinLeds() {
  analogWrite(BlueLedPin, 0);
  analogWrite(RedLedPin, 0);
  analogWrite(GreenLedPin, 0);
  analogWrite(WhiteLedPin, 0);
}
