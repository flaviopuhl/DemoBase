

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

#include <esp_task_wdt.h>                // Watchdog library

#include <rom/rtc.h>

/*+--------------------------------------------------------------------------------------+
 *| Constants declaration                                                                |
 *+--------------------------------------------------------------------------------------+ */
 
const char *ssid                              = "CasaDoTheodoro1";                        // name of your WiFi network
const char *password                          = "09012011";                               // password of the WiFi network

#define WATCHDOGTIMEOUT                       10

unsigned int previousMillis = 0;
int i=0;
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
 *| Method to print the reason by which ESP32 has been awaken from sleep                 |
 *+--------------------------------------------------------------------------------------+ */

void print_reset_reason(RESET_REASON reason)
{
  switch ( reason)
  {
    case 1 : Serial.println ("POWERON_RESET");break;          /**<1, Vbat power on reset*/
    case 3 : Serial.println ("SW_RESET");break;               /**<3, Software reset digital core*/
    case 4 : Serial.println ("OWDT_RESET");break;             /**<4, Legacy watch dog reset digital core*/
    case 5 : Serial.println ("DEEPSLEEP_RESET");break;        /**<5, Deep Sleep reset digital core*/
    case 6 : Serial.println ("SDIO_RESET");break;             /**<6, Reset by SLC module, reset digital core*/
    case 7 : Serial.println ("TG0WDT_SYS_RESET");break;       /**<7, Timer Group0 Watch dog reset digital core*/
    case 8 : Serial.println ("TG1WDT_SYS_RESET");break;       /**<8, Timer Group1 Watch dog reset digital core*/
    case 9 : Serial.println ("RTCWDT_SYS_RESET");break;       /**<9, RTC Watch dog Reset digital core*/
    case 10 : Serial.println ("INTRUSION_RESET");break;       /**<10, Instrusion tested to reset CPU*/
    case 11 : Serial.println ("TGWDT_CPU_RESET");break;       /**<11, Time Group reset CPU*/
    case 12 : Serial.println ("SW_CPU_RESET");break;          /**<12, Software reset CPU*/
    case 13 : Serial.println ("RTCWDT_CPU_RESET");break;      /**<13, RTC Watch dog Reset CPU*/
    case 14 : Serial.println ("EXT_CPU_RESET");break;         /**<14, for APP CPU, reseted by PRO CPU*/
    case 15 : Serial.println ("RTCWDT_BROWN_OUT_RESET");break;/**<15, Reset when the vdd voltage is not stable*/
    case 16 : Serial.println ("RTCWDT_RTC_RESET");break;      /**<16, RTC Watch dog reset digital core and rtc module*/
    default : Serial.println ("NO_MEAN");
  }
}


/*+--------------------------------------------------------------------------------------+
 *| Setup                                                                                |
 *+--------------------------------------------------------------------------------------+ */
 
void setup() {

  Serial.begin(115200);                                                                     // Start serial communication at 115200 baud
  delay(1000);

  esp_task_wdt_init(WATCHDOGTIMEOUT, true);               // Enable watchdog                                               
  esp_task_wdt_add(NULL);                                 // No tasks attached to watchdog
    Serial.println("Watchdog      : [ initialized ]");

  runner.init();
    Serial.println("Scheduler     : [ initialized ]");

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
 
  Serial.print("CPU0 rst reason: ");
  print_reset_reason(rtc_get_reset_reason(0));

  Serial.print("CPU1 rst reason: ");
  print_reset_reason(rtc_get_reset_reason(1));
 

  Serial.print("\n\nSetup Finished\n\n");
  esp_task_wdt_reset();

}



/*+--------------------------------------------------------------------------------------+
 *| main loop                                                                            |
 *+--------------------------------------------------------------------------------------+ */
 
void loop() {

  runner.execute();

  
  
  
  
  
  unsigned int currentMillis = millis();        // debug only

  if (millis() - previousMillis >= 1000) {                                                  
     
    //i++;

    //if(i>=10){
    //  Serial.println("Teste wtd");
    //  delay(15000);
    //}
    
    
    Serial.print("RTC formatted  : ");
    Serial.println(DateAndTimeFormattedRTC());

    previousMillis = millis();
  }

  
  esp_task_wdt_reset();  

}