#include <NeoPixelBus.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include "credentials.h"
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <Timezone.h>    // https://github.com/JChristensen/Timezone
// Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone CE(CEST, CET);
TimeChangeRule *tcr;

#define NUM_LEDS 58

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

int tHour = 12;
int tMinute = 34;
int tSecond = 56;

int divisor = 1024;

int DEFAULT_R = 128;
int DEFAULT_G = 0;
int DEFAULT_B = 128;
static int MIN_R = 0;
static int MIN_G = 0;
static int MIN_B = 1;

boolean dot = true;

static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;

const uint8_t PixelPin = 3;  // make sure to set this to the correct pin, ignored for Esp8266(set to 3 by default for DMA)
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> ledstrip(NUM_LEDS, D4);

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

void loop() {
  server.handleClient();
  int light = analogRead(A0);
  float ledValue = 1 - ((float)light) / ((float)1024);
  
  Serial.print(" ledValue: ");
  Serial.print(ledValue);
  
  int rl = DEFAULT_R * ledValue;
  int gl = DEFAULT_G * ledValue;
  int bl = DEFAULT_B * ledValue;
  if (rl < 5) rl = MIN_R;
  if (gl < 5) gl = MIN_G;
  if (bl < 5) bl = MIN_B;
  
  Serial.print(" r: ");
  Serial.print(rl);
  Serial.print(" g: ");
  Serial.print(gl);
  Serial.print(" b: ");
  Serial.println(bl);
  
  pixel = RgbColor(rl, gl, bl);

  tSecond++;
  if (tSecond == 60) {
    tSecond = 0;
    tMinute++;
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
    DEFAULT_R = server.arg("r").toInt();
    DEFAULT_G = server.arg("g").toInt();
    DEFAULT_B = server.arg("b").toInt();
    Serial.print("r=");Serial.print(DEFAULT_R);
    Serial.print(", g=");Serial.print(DEFAULT_G);
    Serial.print(", b=");Serial.println(DEFAULT_B);
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
  time_t local = CE.toLocal(utc, &tcr);
  tHour = hour(local);
  tMinute = minute(local);
  tSecond = second(local);
  Serial.print(day(local));
  Serial.print('/');
  Serial.print(month(local));
  Serial.print('/');
  Serial.print(year(local));
  Serial.print(' ');
  Serial.print(tHour);
  Serial.print(':');
  Serial.print(tMinute);
  Serial.print(':');
  Serial.println(tSecond);
}
