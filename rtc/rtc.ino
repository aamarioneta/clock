#include <Wire.h>       //I2C library
#include <RtcDS3231.h>  //RTC library
 
//RtcDS3231 rtcObject;              //Uncomment for version 1.0.1 of the rtc library
RtcDS3231<TwoWire> rtcObject(Wire); //Uncomment for version 2.0.0 of the rtc library
 
void setup() {
  Serial.begin(115200);  //Starts serial connection
  Serial.println("starting ...");
  rtcObject.Begin();     //Starts I2C
 
  RtcDateTime currentTime = RtcDateTime(16, 05, 18, 21, 20, 0); //define date and time object
  rtcObject.SetDateTime(currentTime); //configure the RTC with object
 
}
 
void loop() {
 
  RtcDateTime currentTime = rtcObject.GetDateTime();    //get the time from the RTC
 
  char str[20];   //declare a string as an array of chars
 
  sprintf(str, "%d/%d/%d %d:%d:%d",     //%d allows to print an integer to the string
          currentTime.Year(),   //get year method
          currentTime.Month(),  //get month method
          currentTime.Day(),    //get day method
          currentTime.Hour(),   //get hour method
          currentTime.Minute(), //get minute method
          currentTime.Second()  //get second method
         );
 
  Serial.println(str); //print the string to the serial port
 
  delay(1000); //20 seconds delay
 
}
