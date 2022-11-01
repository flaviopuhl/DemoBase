

/*+--------------------------------------------------------------------------------------+
 *| Libraries                                                                            |
 *+--------------------------------------------------------------------------------------+ */

#include <Arduino.h>

#include <TaskScheduler.h>                // Scheduler Library

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  extern "C" {
  #include "user_interface.h"
  } 
#else
  #include <WiFi.h>
#endif

#include <WiFiUDP.h>                     // Network Time Protocol NPT library
#include <NTPClient.h>                   // Network Time Protocol NPT library

#include <ESP32Time.h>                   // ESP32 internal RTC library

/*+--------------------------------------------------------------------------------------+
 *| Constants declaration                                                                |
 *+--------------------------------------------------------------------------------------+ */
 
const char *ssid                              = "CasaDoTheodoro1";                        // name of your WiFi network
const char *password                          = "09012011";                               // password of the WiFi network

unsigned int previousMillis = 0;

/*+--------------------------------------------------------------------------------------+
 *| Callback methods prototypes                                                          |
 *+--------------------------------------------------------------------------------------+ */
 
void VerifyWifi();
void DateAndTimeNPT();

/*+--------------------------------------------------------------------------------------+
 *| Tasks lists                                                                          |
 *+--------------------------------------------------------------------------------------+ */

Task t1(01*30*1000, TASK_FOREVER, &VerifyWifi);
Task t2(60*60*1000, TASK_FOREVER, &DateAndTimeNPT);

/*+--------------------------------------------------------------------------------------+
 *| Objects                                                                          |
 *+--------------------------------------------------------------------------------------+ */

Scheduler runner;                                           // Scheduler
ESP32Time rtc;

/*+--------------------------------------------------------------------------------------+
 *| Connect to WiFi network                                                              |
 *+--------------------------------------------------------------------------------------+ */

void setup_wifi() {
  Serial.print("\nConnecting to ");
  Serial.println(ssid);
    WiFi.mode(WIFI_STA);                              // Setup ESP in client mode
    //WiFi.setSleepMode(WIFI_NONE_SLEEP);
    WiFi.begin(ssid, password);                       // Connect to network

    int wait_passes = 0;
    while (WiFi.status() != WL_CONNECTED) {           // Wait for connection
      delay(500);
      Serial.print(".");
      if (++wait_passes >= 20) { ESP.restart(); }     // Restart in case of no wifi connection   
    }

  Serial.print("\nWiFi connected");
  Serial.print("\nIP address: ");
    Serial.println(WiFi.localIP());

  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

}

/*+--------------------------------------------------------------------------------------+
 *| Verify and Manage WiFi network                                                       |
 *+--------------------------------------------------------------------------------------+ */

void VerifyWifi() {

  if (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0)){          // Check for network health
      
    Serial.printf("error: WiFi not connected, reconnecting \n");
            
      WiFi.disconnect();
      setup_wifi();             

  } 

}

/*+--------------------------------------------------------------------------------------+
 *| Get Date & Time                                                                      |
 *+--------------------------------------------------------------------------------------+ */

void DateAndTimeNPT(){

  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP); 

  timeClient.begin();                                                                     // Initialize a NTPClient to get time

  timeClient.setTimeOffset(-10800);                                                       // Set offset time in seconds to Brazil timezone GMT-3

    while(!timeClient.update()) {
      timeClient.forceUpdate();
    }

  time_t epochTime = timeClient.getEpochTime();                                             // The time_t type is just an integer.  It is the number of seconds since the Epoch.

  rtc.setTime(epochTime);                                                                   // Update internal RTC

  Serial.println("Internal RTC  : [ Updated ]");
 
}

int DateAndTimeEpochRTC(){

  time_t epochTime = rtc.getEpoch();

  return epochTime;
}


String DateAndTimeFormattedRTC(){

  time_t epochTime = rtc.getEpoch();                                                     // The time_t type is just an integer. 

  struct tm * tm = localtime(&epochTime);
  char DateAndTimeFormated[22];
    strftime(DateAndTimeFormated, sizeof(DateAndTimeFormated), "%d%b%Y %H-%M-%S", tm);   // https://www.cplusplus.com/reference/ctime/strftime/
  
  return DateAndTimeFormated;
}



/*+--------------------------------------------------------------------------------------+
 *| Setup                                                                                |
 *+--------------------------------------------------------------------------------------+ */
 
void setup() {

  Serial.begin(115200);                                                                     // Start serial communication at 115200 baud
  delay(1000);

  runner.init();
    Serial.println("Initialized scheduler");

  runner.addTask(t1);
    Serial.println("Added task    : [ VerifyWifi ]");

  t1.enable();
    Serial.println("Enabled task  : [ VerifyWifi ]");

  runner.addTask(t2);
    Serial.println("Added task    : [ DateAndTimeNPT ]");

  t2.enable();
    Serial.println("Enabled task  : [ DateAndTimeNPT ]");

  setup_wifi(); 

  DateAndTimeNPT();

  Serial.print("RTC Unix       : ");          // debug only
  Serial.println(DateAndTimeEpochRTC());      // debug only

  Serial.print("RTC formatted  : ");          // debug only
  Serial.println(DateAndTimeFormattedRTC());  // debug only
 
 

  Serial.print("\n\nSetup Finished\n\n");

}



/*+--------------------------------------------------------------------------------------+
 *| main loop                                                                            |
 *+--------------------------------------------------------------------------------------+ */
 
void loop() {

  runner.execute();

  
  
  
  
  
  unsigned int currentMillis = millis();        // debug only

  if (millis() - previousMillis >= 1000) {                                                  
     

    Serial.print("RTC formatted  : ");
  Serial.println(DateAndTimeFormattedRTC());

    previousMillis = millis();
  }

  
  

}