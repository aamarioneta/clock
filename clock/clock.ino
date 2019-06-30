#include <NeoPixelBus.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include "credentials.h"

#define NUM_LEDS 58

int s = 0;
int h = 20;
int m = 22;
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
  delay(500);
  ledstrip.Begin();//Begin output
  ledstrip.Show();//Clear the strip for use
  digitalWrite(led, HIGH);
  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();
  s++;
  if (s == 60) {
    s = 0;
    m++;
  }
  if (m == 60) {
    m = 0;
    h++;
    // the timer is not very precise so refresh time every hour
    getInternetTime();
  }
  if (h == 24) {
    h = 0;
  }
  
  int d0 = h / 10;
  int d1 = h % 10;
  int d2 = m / 10;
  int d3 = m % 10;
  
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
        h = payload.substring(i + 29, i + 31).toInt();
        m = payload.substring(i + 32, i + 34).toInt();
        Serial.print(h); Serial.print(":"); Serial.print(m);
        Serial.println("i: " + i);
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.printf("[HTTP} Unable to connect\n");
  }
}
