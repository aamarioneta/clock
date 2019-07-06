#include <NeoPixelBus.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include "credentials.h"
#include <Wire.h>
#include <RtcDS3231.h>

#define NUM_LEDS 58

RtcDS3231<TwoWire> rtcObject(Wire);

int second = 0;
int hour = 20;
int minute = 22;
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
  getInternetTime();
  rtcObject.Begin();
  RtcDateTime currentTime = RtcDateTime(00, 1, 1, hour, minute, second); //define date and time object
  rtcObject.SetDateTime(currentTime); //configure the RTC with object
  delay(500);
  ledstrip.Begin();//Begin output
  ledstrip.Show();//Clear the strip for use
  digitalWrite(led, HIGH);
  server.on("/", handleRoot);
  server.begin();
}

void getRTCTime() {
  RtcDateTime currentTime = rtcObject.GetDateTime();
  char str[20];
  sprintf(str, "%d/%d/%d %d:%d:%d",
          currentTime.Year(),   //get year method
          currentTime.Month(),  //get month method
          currentTime.Day(),    //get day method
          currentTime.Hour(),   //get hour method
          currentTime.Minute(), //get minute method
          currentTime.Second()  //get second method
         );
  Serial.println(str); //print the string to the serial port
  second = currentTime.Second();
  minute = currentTime.Minute();
  hour = currentTime.Hour();
}

void loop() {
  server.handleClient();
  int light = analogRead(A0);
  int ledValue = (int)(255 - (light * 255 / 1023)); 
  ledValue = ledValue / 2; // because i never want max brightness, max 128
  Serial.print("Light: ");
  Serial.print(light);
  Serial.print(" ledValue: ");
  Serial.println(ledValue);
  if (ledValue < 10) {
    ledValue = 2;
  }
  pixel = RgbColor((uint8_t)ledValue, (uint8_t)ledValue, (uint8_t)ledValue);

  second++;
  if (second == 60) {
    getRTCTime();
    second = 0;
    minute++;
  }
  if (minute == 60) {
    minute = 0;
    hour++;
  }
  if (hour == 24) {
    hour = 0;
  }
  
  int d0 = hour / 10;
  int d1 = hour % 10;
  int d2 = minute / 10;
  int d3 = minute % 10;
  
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

void handleRoot() { 
  String sr = server.arg("r");
  String sg = server.arg("g");
  String sb = server.arg("b");
  if (sr != "" && sg != "" && sb != "") {
    int r = server.arg("r").toInt();
    int g = server.arg("g").toInt();
    int b = server.arg("b").toInt();
    Serial.print("r=");Serial.print(r);
    Serial.print(", g=");Serial.print(g);
    Serial.print(", b=");Serial.println(b);
    pixel = RgbColor(r,g,b);
  }
  String html = "";
  html+="<!DOCTYPE html><html><head>";
  html+="  <link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css'>";
  html+="  <script src='https://cdnjs.cloudflare.com/ajax/libs/jscolor/2.0.4/jscolor.min.js'></script>";
  html+="  <style>input{font-size:48px;}</style></head> ";
  html+="<body>";
  html+="<input onchange='update(this.jscolor)' value=\"ffcc00\" class=\"jscolor {width:800, height:600}\">";
  html+="<br><input id='r'>";
  html+="<br><input id='g'>";
  html+="<br><input id='b'>";
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
  html+="window.location.href = \"/?r=\" + Math.round(r) + \"&g=\" + Math.round(g) + \"&b=\" + Math.round(b) ";
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
  WiFiClient client;
  HTTPClient http;
  if (http.begin(client, "http://worldclockapi.com/api/json/cet/now")) {
    Serial.print("[HTTP] GET...\n");
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        Serial.println(payload);
        Serial.print(">");
        Serial.println(payload.indexOf("currentDateTime"));
        Serial.print("<");
        int i = payload.indexOf("currentDateTime");
        Serial.println("i: " + i);
        hour = payload.substring(i + 29, i + 31).toInt();
        minute = payload.substring(i + 32, i + 34).toInt();
        Serial.print(hour); Serial.print(":"); Serial.print(minute);
        Serial.println("i: " + i);
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %second\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.printf("[HTTP} Unable to connect\n");
  }
}
