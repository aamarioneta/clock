#include <NeoPixelBus.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include "credentials.h"
#include <Wire.h>
#include <RtcDS3231.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <Timezone.h>    // https://github.com/JChristensen/Timezone
// Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone CE(CEST, CET);

#define NUM_LEDS 58

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
 
RtcDS3231<TwoWire> rtcObject(Wire);

int tSecond = 0;
int tHour = 20;
int tMinute = 22;
int divisor = 1024;
int r = 45;
int g = 0;
int b = 45;
boolean dot=true;

const uint8_t PixelPin = 3;  // make sure to set this to the correct pin, ignored for Esp8266(set to 3 by default for DMA)
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> ledstrip(NUM_LEDS, PixelPin);
const int led = LED_BUILTIN;
int digits[10][7] = {
  {1, 1, 1, 0, 1, 1, 1}, //0
  {1, 0, 0, 0, 1, 0, 0}, //1
  {0, 1, 1, 1, 1, 1, 0}, //2
  {1, 1, 0, 1, 1, 1, 0}, //3
  {1, 0, 0, 1, 1, 0, 1}, //4
  {1, 1, 0, 1, 0, 1, 1}, //5
  {1, 1, 1, 1, 0, 1, 1}, //6
  {1, 0, 0, 0, 1, 1, 0}, //7
  {1, 1, 1, 1, 1, 1, 1}, //8
  {1, 1, 0, 1, 1, 1, 1}, //9
};
int off[7] = {0, 0, 0, 0, 0, 0, 0}; //off

RgbColor pixel = RgbColor((uint8_t)4, (uint8_t)0, (uint8_t)4);
RgbColor pixelOff(0, 0, 0);

ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);
  pinMode(led, OUTPUT); // Initialize the LED_BUILTIN pin as an output
  digitalWrite(led, LOW);
  Serial.begin(115200);
  connectWifi();
  rtcObject.Begin();
  getInternetTime();
  delay(500);
  ledstrip.Begin();//Begin output
  ledstrip.Show();//Clear the strip for use
  digitalWrite(led, HIGH);
  server.on("/", handleRoot);
  server.on("/t", handleGetTime);
  server.begin();
  timeClient.begin();
}

void getRTCTime() {
  RtcDateTime currentTime = rtcObject.GetDateTime();
  char str[20];
  sprintf(str, "%d/%d/%d %d:%d:%d",
          currentTime.Year(),   //get year method
          currentTime.Month(),  //get month method
          currentTime.Day(),    //get day method
          currentTime.Hour(),   //get tHour method
          currentTime.Minute(), //get tMinute method
          currentTime.Second()  //get tSecond method
         );
  Serial.println(str); //print the string to the serial port
  tSecond = currentTime.Second();
  tMinute = currentTime.Minute();
  tHour = currentTime.Hour();
}

void loop() {
  server.handleClient();
  int light = analogRead(A0);
  float ledValue = 1 - ((float)light) / ((float)1024);
  /*
  Serial.print("Light: ");
  Serial.print(light);
  Serial.print(" ledValue: ");
  Serial.println(ledValue);
  */
  int rl=r*ledValue;
  int gl=g*ledValue;
  int bl=b*ledValue;
  if (rl<1)rl=1;
  if (gl<1)gl=1;
  if (bl<1)bl=1;
  /*
  Serial.print(" r: ");
  Serial.print(rl);
  Serial.print(" g: ");
  Serial.print(gl);
  Serial.print(" b: ");
  Serial.println(bl);
  */
  pixel = RgbColor(rl, gl, bl);

  tSecond++;
  if (tSecond == 60) {
    tSecond = 0;
    tMinute++;
    getRTCTime();
  }
  if (tMinute == 60) {
    tMinute = 0;
    tHour++;
  }
  if (tHour == 24) {
    tHour = 0;
    getInternetTime();
  }
  
  int d0 = tHour / 10;
  int d1 = tHour % 10;
  int d2 = tMinute / 10;
  int d3 = tMinute % 10;
  
  digitAt(0, digits[d3]);
  digitAt(1, digits[d2]);
  digitAt(2, digits[d1]);
  digitAt(3, d0==0?off:digits[d0]);

  if (dot) {
    ledstrip.SetPixelColor(28, pixel);
    ledstrip.SetPixelColor(29, pixel);
  } else {
    ledstrip.SetPixelColor(28, pixelOff);
    ledstrip.SetPixelColor(29, pixelOff);
  }
  dot = !dot;
  ledstrip.Show();
  delay(1000);
}

void handleGetTime() {
  getInternetTime();
  server.send(200, "text/html", "ok");
}

void handleRoot() { 
  String sr = server.arg("r");
  String sg = server.arg("g");
  String sb = server.arg("b");
  String sl = server.arg("l");
  if (sr != "" && sg != "" && sb != "") {
    r = server.arg("r").toInt();
    g = server.arg("g").toInt();
    b = server.arg("b").toInt();
    Serial.print("r=");Serial.print(r);
    Serial.print(", g=");Serial.print(g);
    Serial.print(", b=");Serial.println(b);
    pixel = RgbColor(r,g,b);
  }
  if (sl != "") {
    divisor = sl.toInt();
    Serial.print("l=");Serial.println(divisor);
  }
  String html = "";
  html+="<!DOCTYPE html><html><head>";
  html+="  <link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css'>";
  html+="  <script src='https://cdnjs.cloudflare.com/ajax/libs/jscolor/2.0.4/jscolor.min.js'></script>";
  html+="  <style>input{font-size:48px;}</style></head> ";
  html+="<body>";
  html+="<input onchange='update(this.jscolor)' value=\"ffcc00\" class=\"jscolor {width:800, height:600}\">";
  html+="<br>red: <input id='r'>";
  html+="<br>green:  <input id='g'>";
  html+="<br>blue: <input id='b'>";
  html+="<br>brightness divide by: <input id='l' value='4'>";
  html+="<p id='rect' style='border:1px solid gray; width:161px; height:100px;'> ";
  html+="<p> ";
  html+="<input type='button' onclick='setColor()' value='Apply'> ";
  html+="</p>";
  html+=" ";
  html+="<script>";
  html+="function setColor() {";
  html+="r = document.getElementById('r').value; ";
  html+="g = document.getElementById('g').value; ";
  html+="b = document.getElementById('b').value; ";
  html+="l = document.getElementById('l').value; ";
  html+="window.location.href = \"/?r=\" + Math.round(r) + \"&g=\" + Math.round(g) + \"&b=\" + Math.round(b) + \"&l=\" + Math.round(l) ";
  html+="}";
  html+="function update(jscolor) { ";
  html+="console.log(jscolor.rgb[0]); ";
  html+="console.log(jscolor.rgb[1]); ";
  html+="console.log(jscolor.rgb[2]); ";
  html+="document.getElementById('r').value=jscolor.rgb[0];";
  html+="document.getElementById('g').value=jscolor.rgb[1];";
  html+="document.getElementById('b').value=jscolor.rgb[2];";
  html+="document.getElementById('rect').style.backgroundColor = '#' + jscolor";
  html+="}";
  html+="</script></body></html>";
  server.send(200, "text/html", html);
}

void digitAt(int position, int digit[]) {
  int offset = position * 7 * 2;
  if (position > 1) {
    offset += 2; // colon between numbers
  }
  for (int j = 0; j < 7; j++) {
    ledstrip.SetPixelColor(offset + j * 2, digit[j] == 1 ? pixel : pixelOff);
    ledstrip.SetPixelColor(offset + j * 2 + 1, digit[j] == 1 ? pixel : pixelOff);
  }
}

void connectWifi() {
  WiFi.begin(STASSID, STAPSK);
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected to ");
  Serial.println(STASSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void getInternetTime() {
  timeClient.update();
  Serial.print("Got ntp time: ");
  Serial.println(timeClient.getFormattedTime());
  time_t utc = timeClient.getEpochTime();
  setSummerTime(utc, "Berlin");
  RtcDateTime currentTime = RtcDateTime(00, 1, 1, tHour, tMinute, tSecond); //define date and time object
  rtcObject.SetDateTime(currentTime); //configure the RTC with object
}

// given a Timezone object, UTC and a string description, convert and print local time with time zone
void setSummerTime(time_t utc, const char *descr)
{
    char buf[40];
    char m[4];    // temporary storage for month string (DateStrings.cpp uses shared buffer)
    TimeChangeRule *tcr;        // pointer to the time change rule, use to get the TZ abbrev

    time_t t = CE.toLocal(utc, &tcr);
    strcpy(m, monthShortStr(month(t)));
    sprintf(buf, "%.2d:%.2d:%.2d %s %.2d %s %d %s",
        hour(t), minute(t), second(t), dayShortStr(weekday(t)), day(t), m, year(t), tcr -> abbrev);
    Serial.print("Summer Time: ");
    Serial.print(buf);
    Serial.print(' ');
    Serial.println(descr);
    tHour = hour(t);
    tMinute = minute(t);
    tSecond = second(t);  
}
