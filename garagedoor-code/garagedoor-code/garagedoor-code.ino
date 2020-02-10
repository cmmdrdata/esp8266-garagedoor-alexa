/*
 Version 0.4 - April 26 2019
*/ 

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h> //  https://github.com/kakopappa/sinric/wiki/How-to-add-dependency-libraries
#include <ArduinoJson.h> // https://github.com/kakopappa/sinric/wiki/How-to-add-dependency-libraries (use the correct version)
#include <StreamString.h>
#include <NTPtimeESP.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
WiFiClient client;

ESP8266WebServer server(80);

NTPtime NTPch("north-america.pool.ntp.org");   // Choose server pool as required

#define MyApiKey "" // TODO: Change to your sinric API Key. Your API Key is displayed on sinric.com dashboard
#define MySSID "ATT2vAepi2" // TODO: Change to your Wifi network SSID
#define MyWifiPassword "" // TODO: Change to your Wifi network password

#define HEARTBEAT_INTERVAL 300000 // 5 Minutes 

#define SENSOR 4
#define SWITCH 5
#define DOWN HIGH
#define UP LOW

uint64_t heartbeatTimestamp = 0;
unsigned int lastChangeSecs = millis()/ 1000;
bool isConnected = false;
int lastCommand =2;
int actualState = 3;
strDateTime lastChangeTime;
strDateTime dateTime;
strDateTime bootTime;
char buffr[300];



// deviceId is the ID assgined to your smart-home-device in sinric.com dashboard. Copy it from dashboard and paste it here

void turnOn(String deviceId) {
  if (deviceId == "5e3f1de3ad93316073dedaf3") // Device ID of first device
  {  
    Serial.print("Turn open device id: ");
    Serial.println(deviceId);
    int position = digitalRead(SENSOR);

    if (position == DOWN) {
        digitalWrite(SWITCH, HIGH); 
        delay(500); 
        digitalWrite(SWITCH, LOW);
    }
  } 
  else if (deviceId == "5axxxxxxxxxxxxxxxxxxx") // Device ID of second device
  { 
    Serial.print("Turn on device id: ");
    Serial.println(deviceId);
  }
  else {
    Serial.print("Turn on for unknown device id: ");
    Serial.println(deviceId);    
  }    
  lastCommand = UP;
  lastChangeTime = dateTime;
  lastChangeSecs = millis()/ 1000;
}

void turnOff(String deviceId) {
   if (deviceId == "5e3f1de3ad93316073dedaf3") // Device ID of first device
   {  
     Serial.print("Turn close Device ID: ");
     Serial.println(deviceId);
     int position = digitalRead(SENSOR);

    if (position == UP) {
        digitalWrite(SWITCH, HIGH); 
        delay(500); 
        digitalWrite(SWITCH, LOW);
    }
 
   }
   else if (deviceId == "5axxxxxxxxxxxxxxxxxxx") // Device ID of second device
   { 
     Serial.print("Turn off Device ID: ");
     Serial.println(deviceId);
  }
  else {
     Serial.print("Turn off for unknown device id: ");
     Serial.println(deviceId);    
  }
  
    lastCommand = DOWN;
    lastChangeTime = dateTime;
    lastChangeSecs = millis()/ 1000;

}
void handleRoot() {
  char resp[5000];
  char door_status[20];
  char command[20];


  uint64_t current = millis()/ 1000;
  if (digitalRead(SENSOR) == 1) { 
      sprintf(door_status, "CLOSED");

  } else { 
      sprintf(door_status, "OPEN");

  }
  if (lastCommand == UP) { 
     sprintf(command, "OPEN");

  } else if (lastCommand == DOWN) {
      sprintf(command, "CLOSE");

  } else { 
     sprintf(command, "NONE");
  }
  
  char change[100]; 
  char boot[100]; 

  if ( formatDateTime(lastChangeTime, change) == 0) { 
       sprintf(change, "");
  }

  if ( formatDateTime(bootTime, boot)) { 
        sprintf(resp, "<h1>Hello from esp8266! <h2>garage door status <br><b>%s<br> last alexa command was %s @ %s<br>boot @%s" , 
          door_status,
          command,
          lastChangeTime.year, change,
          boot
          );
  }
  
  

  server.send(200, "text/html", resp);
  
}
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      isConnected = false;    
      Serial.printf("[WSc] Webservice disconnected from sinric.com!\n");
      break;
    case WStype_CONNECTED: {
      isConnected = true;
      Serial.printf("[WSc] Service connected to sinric.com at url: %s\n", payload);
      Serial.printf("Waiting for commands from sinric.com ...\n");        
      }
      break;
    case WStype_TEXT: {
        Serial.printf("[WSc] get text: %s\n", payload);
        // Example payloads

        // For Switch or Light device types
        // {"deviceId": xxxx, "action": "setPowerState", value: "ON"} // https://developer.amazon.com/docs/device-apis/alexa-powercontroller.html

        // For Light device type
        // Look at the light example in github
#if ARDUINOJSON_VERSION_MAJOR == 5
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject((char*)payload);
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6        
        DynamicJsonDocument json(1024);
        deserializeJson(json, (char*) payload);      
#endif        
        String deviceId = json ["deviceId"];     
        String action = json ["action"];
        
        if(action == "setPowerState") { // Switch or Light
            String value = json ["value"];
            if(value == "ON") {
                turnOn(deviceId);
            } else {
                turnOff(deviceId);
            }
        }
        else if (action == "SetTargetTemperature") {
            String deviceId = json ["deviceId"];     
            String action = json ["action"];
            String value = json ["value"];
        }
        else if (action == "test") {
            Serial.println("[WSc] received test command from sinric.com");
        }
      }
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  
  WiFiMulti.addAP(MySSID, MyWifiPassword);
  Serial.println();
  Serial.print("Connecting to Wifi: ");
  Serial.println(MySSID);  

  // Waiting for Wifi connect
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  if(WiFiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("WiFi connected. ");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  pinMode(2, OUTPUT); 

  pinMode(SWITCH, OUTPUT); 
  pinMode(SENSOR, INPUT_PULLUP);

  digitalWrite(SWITCH, LOW);

  server.on("/", handleRoot);

  // server address, port and URL
  webSocket.begin("iot.sinric.com", 80, "/");

  // event handler
  webSocket.onEvent(webSocketEvent);
  webSocket.setAuthorization("apikey", MyApiKey);
  
  // try again every 5000ms if connection has failed
  webSocket.setReconnectInterval(5000);   // If you see 'class WebSocketsClient' has no member named 'setReconnectInterval' error update arduinoWebSockets
  
  server.begin();
  Serial.println("HTTP server started");
}

void getTime() {

  // first parameter: Time zone in floating point (for India); second parameter: 1 for European summer time; 2 for US daylight saving time; 0 for no DST adjustment; (contributed by viewwer, not tested by me)
  dateTime = NTPch.getNTPtime(-6.0, 1);

  // check dateTime.valid before using the returned time
  // Use "setSendInterval" or "setRecvTimeout" if required
  if(dateTime.valid){
    NTPch.printDateTime(dateTime);

    byte actualHour = dateTime.hour;
    byte actualMinute = dateTime.minute;
    byte actualsecond = dateTime.second;
    int actualyear = dateTime.year;
    byte actualMonth = dateTime.month;
    byte actualday =dateTime.day;
    byte actualdayofWeek = dateTime.dayofWeek;
  }
}

int formatDateTime( strDateTime dateTime, char *buff){ 

    if (dateTime.valid) { 
    byte actualHour = dateTime.hour;
    byte actualMinute = dateTime.minute;
    byte actualsecond = dateTime.second;
    int actualyear = dateTime.year;
    byte actualMonth = dateTime.month;
    byte actualday =dateTime.day;
    byte actualdayofWeek = dateTime.dayofWeek;

    sprintf(buff, "%02d/%02d/%d %02d:%02d:%02d", actualMonth, actualday, actualyear, actualHour, actualMinute, actualsecond);
    //Serial.println(buffer);
    return 1;
    }
    return 0;
}


void setPowerStateOnServer(String deviceId, String value) {
#if ARDUINOJSON_VERSION_MAJOR == 5
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6        
  DynamicJsonDocument root(1024);
#endif        
  root["deviceId"] = "5e3f1de3ad93316073dedaf3";
  root["action"] = "setPowerState";
  root["value"] = value;
  StreamString databuf;
#if ARDUINOJSON_VERSION_MAJOR == 5
  root.printTo(databuf);
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6        
  serializeJson(root, databuf);
#endif  
  
  webSocket.sendTXT(databuf);
}

void loop() {
  webSocket.loop();
  
  if(isConnected) {
      uint64_t now = millis();
      
      // Send heartbeat in order to avoid disconnections during ISP resetting IPs over night. Thanks @MacSass
      if((now - heartbeatTimestamp) > HEARTBEAT_INTERVAL) {
          heartbeatTimestamp = now;
          webSocket.sendTXT("H");          
      }
      strDateTime mn = NTPch.getNTPtime(-6.0, 1);
  
      // check dateTime.valid before using the returned time
      // Use "setSendInterval" or "setRecvTimeout" if required
      if(mn.valid){
        //NTPch.printDateTime(dateTime);
        dateTime = mn;
        if (bootTime.year == 0) { 
            bootTime = dateTime;
        }
      }
      if (digitalRead(SENSOR)) { 
           digitalWrite(2, HIGH); 
           actualState = DOWN;
           
      } else { 
           digitalWrite(2, LOW);
           actualState = UP;

      }
      if (lastCommand == 2 ) { 
          lastCommand = actualState;
      }
      Serial.print("actual state = ");
      Serial.println(actualState);
      Serial.print("last command = ");
      Serial.println(lastCommand);
         
      unsigned int elapsed= (millis() / 1000) - lastChangeSecs;
            Serial.print("elapsed = ");
      Serial.println(elapsed);
      if (actualState != lastCommand and lastCommand < 2 and elapsed > 30) { 
         
  
         if (actualState == DOWN) { 
             Serial.println("actual state != lastCommand and actualState is down sending off ");

             setPowerStateOnServer("","Off");
             lastChangeSecs = millis()/ 1000;
         } else if (actualState == UP) { 
             Serial.println("actual state != lastCommand and actualState is up sending on ");

             setPowerStateOnServer("","On");
             lastChangeSecs = millis()/ 1000;

         }
         //lastCommand = actualState;

      }
      delay(500);
      //Serial.println(WiFi.localIP());
      server.handleClient();
      MDNS.update();
  }   
  //Serial.println(digitalRead(SENSOR));

}

// If you want a push button: https://github.com/kakopappa/sinric/blob/master/arduino_examples/switch_with_push_button.ino  
