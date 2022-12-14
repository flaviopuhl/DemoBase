

/*+--------------------------------------------------------------------------------------+
 *| Libraries                                                                            |
 *+--------------------------------------------------------------------------------------+ */

#include <Arduino.h>

#include <TaskScheduler.h>                // Scheduler Library

#include <WiFi.h>

#include <WiFiUDP.h>                     // Network Time Protocol NPT library
#include <NTPClient.h>                   // Network Time Protocol NPT library

#include <ESP32Time.h>                   // ESP32 internal RTC library

#include <esp_task_wdt.h>                // Watchdog library

#include <rom/rtc.h>                     // Wakeup reason

#include <WiFiMulti.h>                   // http update
#include <HTTPClient.h>                  // http update
#include <HTTPUpdate.h>                  // http update
#include <WiFiClientSecure.h>            // http update

#include <PubSubClient.h>                // MQTT
#include <ArduinoJson.h>                 // MQTT


/*+--------------------------------------------------------------------------------------+
 *| Constants declaration                                                                |
 *+--------------------------------------------------------------------------------------+ */
 
 // Insert here the wifi network credentials
const char *ssid                              = "CasaDoTheodoro1";                        // name of your WiFi network
const char *password                          = "09012011";                               // password of the WiFi network

// Insert here the *.bin file repository path
#define OTAFIRMWAREREPO                       "https://firebasestorage.googleapis.com/v0/b/firmwareota-a580e.appspot.com/o/ESP32OTAexample%2Ffirmware.bin?alt=media"

// Insert here MQTT device name and broker
const char *ID                                = "DemoBase";                               // Name of our device, must be unique
const char* BROKER_MQTT                       = "broker.hivemq.com";                      // MQTT Cloud Broker URL

// Insert here topics that the device will publish to broker
const char *TopicsToPublish[]                 = { 
                                                "DemoBase/data",
                                                "DemoBase/info"
                                              };

// Insert here the topics the device will listen from broker
const char *TopicsToSubscribe[]               = { 
                                                "DemoBase/reset", 
                                                "DemoBase/update",  
                                                "DemoBase/builtinled"
                                              };



String DeviceName                             = "DemoBase";
String FirmWareVersion                        = "DemoBase_002";

String WakeUpReasonCPU0                       = "";
String WakeUpReasonCPU1                       = "";

int UptimeHours                               = 0;
RTC_DATA_ATTR int UptimeHoursLifeTime;

#define LED 2       // Built In Led pin

#define WATCHDOGTIMEOUT                       10    // Watchdog set to 10 sec


/*+--------------------------------------------------------------------------------------+
 *| Callback methods prototypes                                                          |
 *+--------------------------------------------------------------------------------------+ */
 
void VerifyWifi();
void DateAndTimeNPT();
void SerializeAndPublish();
void Uptime();
void IAmAlive();

/*+--------------------------------------------------------------------------------------+
 *| Tasks lists                                                                          |
 *+--------------------------------------------------------------------------------------+ */

Task t0(01*01*1000, TASK_FOREVER, &IAmAlive);
Task t1(01*30*1000, TASK_FOREVER, &VerifyWifi);
Task t2(60*60*1000, TASK_FOREVER, &DateAndTimeNPT);
Task t3(01*60*1000, TASK_FOREVER, &SerializeAndPublish);
Task t4(60*60*1000, TASK_FOREVER, &Uptime);

/*+--------------------------------------------------------------------------------------+
 *| Objects                                                                              |
 *+--------------------------------------------------------------------------------------+ */

Scheduler runner;                                           // Scheduler
ESP32Time rtc;

WiFiClient wclient;
PubSubClient MQTTclient(wclient);                           // Setup MQTT client


/*+--------------------------------------------------------------------------------------+
 *| Method to reset the device                                                           |
 *+--------------------------------------------------------------------------------------+ */
 
void deviceReset() {

  MQTTclient.publish(TopicsToPublish[1], "Restarting device");
  delay(1000);
  ESP.restart();
  
}


/*+--------------------------------------------------------------------------------------+
 *| Method to drive the built-in led                                                     |
 *+--------------------------------------------------------------------------------------+ */
 
void builtInLedTest(int input) {

  if (input == 0 ) { 
    digitalWrite(LED,LOW);  
    Serial.println("Turning LED off"); 
    MQTTclient.publish(TopicsToPublish[1], "Turning LED off");
    }     
  if (input == 1 ) { 
    digitalWrite(LED,HIGH); 
    Serial.println("Turning LED on");  
    MQTTclient.publish(TopicsToPublish[1], "Turning LED on");
    }   
  
}


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
      if (++wait_passes >= 20) { deviceReset(); }     // Restart in case of no wifi connection   
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

String print_reset_reason(RESET_REASON reason)
{
  String rst_reason;

  switch ( reason)
  {
    case 1 : rst_reason = String(reason) + " POWERON_RESET";break;          /**<1, Vbat power on reset*/
    case 3 : rst_reason = String(reason) + " SW_RESET";break;               /**<3, Software reset digital core*/
    case 4 : rst_reason = String(reason) + " OWDT_RESET";break;             /**<4, Legacy watch dog reset digital core*/
    case 5 : rst_reason = String(reason) + " DEEPSLEEP_RESET";break;        /**<5, Deep Sleep reset digital core*/
    case 6 : rst_reason = String(reason) + " SDIO_RESET";break;             /**<6, Reset by SLC module, reset digital core*/
    case 7 : rst_reason = String(reason) + " TG0WDT_SYS_RESET";break;       /**<7, Timer Group0 Watch dog reset digital core*/
    case 8 : rst_reason = String(reason) + " TG1WDT_SYS_RESET";break;       /**<8, Timer Group1 Watch dog reset digital core*/
    case 9 : rst_reason = String(reason) + " RTCWDT_SYS_RESET";break;       /**<9, RTC Watch dog Reset digital core*/
    case 10 : rst_reason = String(reason) + " INTRUSION_RESET";break;       /**<10, Instrusion tested to reset CPU*/
    case 11 : rst_reason = String(reason) + " TGWDT_CPU_RESET";break;       /**<11, Time Group reset CPU*/
    case 12 : rst_reason = String(reason) + " SW_CPU_RESET";break;          /**<12, Software reset CPU*/
    case 13 : rst_reason = String(reason) + " RTCWDT_CPU_RESET";break;      /**<13, RTC Watch dog Reset CPU*/
    case 14 : rst_reason = String(reason) + " EXT_CPU_RESET";break;         /**<14, for APP CPU, reseted by PRO CPU*/
    case 15 : rst_reason = String(reason) + " RTCWDT_BROWN_OUT_RESET";break;/**<15, Reset when the vdd voltage is not stable*/
    case 16 : rst_reason = String(reason) + " RTCWDT_RTC_RESET";break;      /**<16, RTC Watch dog reset digital core and rtc module*/
    default : rst_reason = String(reason) + " UNKNOW";
  }

  return rst_reason;
}


void verbose_print_reset_reason(int reason)
{
  switch ( reason)
  {
    case 1  : Serial.println (" Vbat power on reset");break;
    case 3  : Serial.println (" Software reset digital core");break;
    case 4  : Serial.println (" Legacy watch dog reset digital core");break;
    case 5  : Serial.println (" Deep Sleep reset digital core");break;
    case 6  : Serial.println (" Reset by SLC module, reset digital core");break;
    case 7  : Serial.println (" Timer Group0 Watch dog reset digital core");break;
    case 8  : Serial.println (" Timer Group1 Watch dog reset digital core");break;
    case 9  : Serial.println (" RTC Watch dog Reset digital core");break;
    case 10 : Serial.println (" Instrusion tested to reset CPU");break;
    case 11 : Serial.println (" Time Group reset CPU");break;
    case 12 : Serial.println (" Software reset CPU");break;
    case 13 : Serial.println (" RTC Watch dog Reset CPU");break;
    case 14 : Serial.println (" for APP CPU, reseted by PRO CPU");break;
    case 15 : Serial.println (" Reset when the vdd voltage is not stable");break;
    case 16 : Serial.println (" RTC Watch dog reset digital core and rtc module");break;
    default : Serial.println (" NO_MEAN");
  }
}


/*+--------------------------------------------------------------------------------------+
 *| Remote HTTP OTA                                                                      |
 *+--------------------------------------------------------------------------------------+ */

void RemoteHTTPOTA(){

  MQTTclient.publish(TopicsToPublish[1], "Updating device");

  //if ((WiFi.status() == WL_CONNECTED && updateSoftareOnNextReboot == 1)) {
  if ((WiFi.status() == WL_CONNECTED)) {

    Serial.println("SW Update     :  [ Started ]");

    esp_task_wdt_init(WATCHDOGTIMEOUT, false); 


    WiFiClientSecure client; 
    client.setInsecure();       

    // The line below is optional. It can be used to blink the LED on the board during flashing
    // The LED will be on during download of one buffer of data from the network. The LED will
    // be off during writing that buffer to flash
    // On a good connection the LED should flash regularly. On a bad connection the LED will be
    // on much longer than it will be off. Other pins than LED_BUILTIN may be used. The second
    // value is used to put the LED on. If the LED is on with HIGH, that value should be passed
    httpUpdate.setLedPin(LED_BUILTIN, LOW);

    t_httpUpdate_return ret = httpUpdate.update(client, OTAFIRMWAREREPO);
    

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("SW Download   :  [ Done, will update on next reboot ]");
        break;
    }
  } else{
    Serial.println("SW Download   : [ Not requested ]");
  }
}


/*+--------------------------------------------------------------------------------------+
 *| Reconnect to MQTT client                                                             |
 *+--------------------------------------------------------------------------------------+ */
 
void MQTTconnect() {

  if(!MQTTclient.connected()) {                               // Check if MQTT client is connected
  
  Serial.println();
  Serial.println("MQTT Client   : [ not connected ]");

  MQTTclient.setServer(BROKER_MQTT, 1883);                    // MQTT port, unsecure
  MQTTclient.setBufferSize(1024);
                      
    
    Serial.println("MQTT Client   : [ trying connection ]");
    
    if (MQTTclient.connect(ID)) {
      Serial.println("MQTT Client   : [ broker connected ]");

      for(int i=0; i<=((sizeof(TopicsToPublish) / sizeof(TopicsToPublish[0]))-1); i++){

        Serial.print("MQTT Client   : [ publishing to ");
        Serial.print(TopicsToPublish[i]);
        Serial.println(" ]");
        MQTTclient.subscribe(TopicsToPublish[i]);
      }

      for(int i=0; i<=((sizeof(TopicsToSubscribe) / sizeof(TopicsToSubscribe[0]))-1); i++){

        Serial.print("MQTT Client   : [ subscribing to ");
        Serial.print(TopicsToSubscribe[i]);
        Serial.println(" ]");
        MQTTclient.subscribe(TopicsToSubscribe[i]);
      }

    } else {
      Serial.print("MQTT Client   : [ failed, rc= ");
      Serial.print(MQTTclient.state());
      Serial.println(" ]");

      delay(5000);
      setup_wifi();
    }
  }
}

/*+--------------------------------------------------------------------------------------+
 *| Serialize JSON and publish MQTT                                                      |
 *+--------------------------------------------------------------------------------------+ */

void SerializeAndPublish() {

  if (!MQTTclient.connected())                            // Reconnect if connection to MQTT is lost 
  {    MQTTconnect();      }

  char buff[10];                                      // Buffer to allocate decimal to string conversion 
  char buffer[1024];                                   // JSON serialization 
  
    StaticJsonDocument<256> doc;                      // See ArduinoJson Assistant V6 
    
      doc["Device"]             = DeviceName;
      doc["Version"]            = FirmWareVersion;
      doc["RSSI (db)"]          = WiFi.RSSI();
      doc["IP"]                 = WiFi.localIP();
      doc["LastRoll"]           = DateAndTimeFormattedRTC();
      doc["UpTime (h)"]         = UptimeHours;
      doc["UptimeLife (h)"]     = UptimeHoursLifeTime;
      doc["WakeUpReasonCPU0"]   = WakeUpReasonCPU0;
      doc["WakeUpReasonCPU1"]   = WakeUpReasonCPU1;
      doc["freeHeapMem"]        = ESP.getFreeHeap();
      //doc["Last Picture"] = fileName;
      //doc["Temp (??C)"] = dtostrf(getTemp(), 2, 1, buff);
    
    serializeJson(doc, buffer);
      Serial.printf("\nJSON Payload:");
      Serial.printf("\n");
    serializeJsonPretty(doc, Serial);                 // Print JSON payload on Serial port        
      Serial.printf("\n");
      Serial.println("MQTT Client   : [ Sending message to MQTT topic ]"); 
      Serial.println("");         

    if(MQTTclient.publish(TopicsToPublish[0], buffer)){    // Publish data to MQTT Broker 
      Serial.println("MQTT Client   : [ failed to send ]"); 
    };                    

}


/*+--------------------------------------------------------------------------------------+
 *| MQTT Callback                                                                        |
 *+--------------------------------------------------------------------------------------+ */

String MQTTcallback(char* topicSubscribed, byte* payload, unsigned int length) {
  
  String payloadReceived = "";
  
  Serial.print("Message arrived [");
  Serial.print(topicSubscribed);
  Serial.print("] ");

  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    payloadReceived += (char)payload[i];
  }
  Serial.println();

  Serial.println(payloadReceived);

  if (strcmp (topicSubscribed, TopicsToSubscribe[0]) == 0 ) { deviceReset();}
  if (strcmp (topicSubscribed, TopicsToSubscribe[1]) == 0 ) { RemoteHTTPOTA();}
  if (strcmp (topicSubscribed, TopicsToSubscribe[2]) == 0 ) { builtInLedTest(payloadReceived.toInt());}

  return payloadReceived;

}


/*+--------------------------------------------------------------------------------------+
 *| Uptime counter in hours                                                              |
 *+--------------------------------------------------------------------------------------+ */
 
void Uptime() {

  UptimeHours++;            // Uptime counts the time the device is working since last power on
  UptimeHoursLifeTime++;    // Life time counts the total time the device is been working

}


/*+--------------------------------------------------------------------------------------+
 *| Simple method to show that program is running                                        |
 *+--------------------------------------------------------------------------------------+ */

void IAmAlive(){

  Serial.print("RTC formatted  : ");
  Serial.println(DateAndTimeFormattedRTC());
  
}


/*+--------------------------------------------------------------------------------------+
 *| Setup                                                                                |
 *+--------------------------------------------------------------------------------------+ */
 
void setup() {

  Serial.begin(115200);                                                                     // Start serial communication at 115200 baud
  delay(1000);

  pinMode(LED,OUTPUT);
  digitalWrite(LED,LOW);

  Serial.println();
  Serial.println(FirmWareVersion);
  Serial.println();

  runner.init();

    runner.addTask(t0);
    t0.enable();  
      Serial.println("Added task    : [ IAmAlive ]");

    runner.addTask(t1);
    t1.enable();  
      Serial.println("Added task    : [ VerifyWifi ]");

    runner.addTask(t2);
    t2.enable();
      Serial.println("Added task    : [ DateAndTimeNPT ]");

    runner.addTask(t3);
    t3.enable();
      Serial.println("Added task    : [ SerializeAndPublish ]");

    runner.addTask(t4);
    t4.enable();
      Serial.println("Added task    : [ Uptime ]");

  setup_wifi();         // Start wifi

  esp_task_wdt_init(WATCHDOGTIMEOUT, true);               // Enable watchdog                                               
  esp_task_wdt_add(NULL);                                 // No tasks attached to watchdog
     Serial.println("Watchdog      : [ initialized ]");

  DateAndTimeNPT();     // Get time from NPT and update ESP RTC

  MQTTconnect();        // Connect to MQTT Broker

  MQTTclient.setCallback(MQTTcallback);       // MQTT callback method to subscribing topics
 
  WakeUpReasonCPU0 = print_reset_reason(rtc_get_reset_reason(0));
    Serial.print("CPU0 rst reason: ");
    Serial.print(WakeUpReasonCPU0);
      verbose_print_reset_reason(rtc_get_reset_reason(0));

  WakeUpReasonCPU1 = print_reset_reason(rtc_get_reset_reason(1));
    Serial.print("CPU1 rst reason: ");
    Serial.print(WakeUpReasonCPU1);
      verbose_print_reset_reason(rtc_get_reset_reason(1));
   
  UptimeHours--;

  //SerializeAndPublish();

  Serial.println("");
  Serial.println("Setup         : [ finished ]");
  Serial.println("");
  
  esp_task_wdt_reset();   // feed watchdog

}


/*+--------------------------------------------------------------------------------------+
 *| main loop                                                                            |
 *+--------------------------------------------------------------------------------------+ */
 
void loop() {

  runner.execute();         // Scheduler stuff

  MQTTclient.loop();        // Needs to be in the loop to keep client connection alive

  esp_task_wdt_reset();     // feed watchdog

}