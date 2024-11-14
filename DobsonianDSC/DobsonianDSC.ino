/*
  Dual Encoder Digital Setting Circles for ESP32 microcontrollers

  This is a simple adapter to connect two inexpensive incremental optical encoders
  via WiFi and/or BlueTooth to astronomy apps such as SkySafari,
  in order to provide "Push To" style features to dobsonian style telescope mounts.

  Copyright (c) 2017-2021 Vladimir Atehortúa. All rights reserved.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the version 3 GNU General Public License as
  published by the Free Software Foundation.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License along with this program.
  If not, see <http://www.gnu.org/licenses/>
*/

/**
    Hardware used:
    ESP-32S Development Board, I used a generic one: https://www.aliexpress.com/item/32800930387.html?
    Two Optical Encoders with NPN (a.k.a. "N channel") "Open Collector" (a.k.a. "open drain") output, I used some 600 p/r generic ones: https://www.aliexpress.com/item/32669741048.html
    Dupont cables or other suitable connectors.
    Power supply (could be a 5V USB power bank)

    ESP32 pinout:
     IMPORTANT: Only encoders with NPN Open collector outputs can be connected directly to the pins of the ESP32. Other encoders might require level shifters.
     AZ_Encoder_A = GPIO18 = D18  (or change it in the #define code below)
     AZ_Encoder_B = GPIO19 = D19  (or change it in the #define code below)
     ALT_Encoder_A = GPIO25 = D25 (or change it in the #define code below)
     ALT_Encoder_B = GPIO26 = D26 (or change it in the #define code below)
     VIN = Encoder VCC (and if you don't use a USB power bank, this would also connect to your 5V power source)
     GND = Encoder GND
*/

/**
   Required Additional Board Managers for the Arduino IDE (File->Preferences):
   https://dl.espressif.com/dl/package_esp32_index.json

   Required libraries for the Arduino IDE (Tools -> Manage Libraries):
   ESP32Encoder by Kevin Harrington, see https://github.com/madhephaestus/ESP32Encoder
   ArduinoJson by Benoit Blanchon, see https://arduinojson.org/
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <ESP32Encoder.h>
#include "WebConfig_DSC.h"
#include "BluetoothSerial.h"
#include "note_frequencies.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define DEBUG_PROTOCOL //for priting debug messages when handling protocol

// Choose which pins of the ESP32 to use for the Azimuth Encoder
ESP32Encoder AZencoder;
#define PIN_AZ_A 18 // Important: Not all encoders can be connected directly to the pins of the ESP32, read more about this in the project's github page
#define PIN_AZ_B 19

#define AZIMUTH_RES 1800 //ticks per rotation
#define AZIMUTH_START 0 //starts at 0 - NORTH

// Choose which pins of the ESP32 to use for the Altitude Encoder
ESP32Encoder ALTencoder;
#define PIN_ALT_A 25
#define PIN_ALT_B 26

#define ALTITUDE_RES 22140 //ticks per rotation
#define ALTITUDE_START 5624  //starts at 5624  - 90+ degrees (parked)

#define AP_WIFI_NETWORK "TelescopeGSO_DSC"

#define MAX_SRV_CLIENTS  3   // How many clients can connect simultaneously to the DSC.
#define MAX_REQUEST_LENGTH 30 // Maximum length of request from client to process 

//beeper for signalling
#define PIN_BEEPER 33
//count-reset button
#define PIN_RESET 34
#define RESTART_HOLD_TIME 3000 // 3s - 3000ms
#define PIN_RESET_WAIT_TIME 50

unsigned int melodyStart[] = {NOTE_C, NOTE_E, NOTE_G, NOTE_0, NOTE_C, NOTE_E, NOTE_G, NOTE_FINISH};
unsigned int melodyError[] = {NOTE_C, NOTE_C, NOTE_G, NOTE_G, NOTE_C, NOTE_C, NOTE_G, NOTE_G, NOTE_FINISH};
unsigned int melodySetupCompleted[] = {NOTE_C, NOTE_0, NOTE_C, NOTE_0, NOTE_C, NOTE_0, NOTE_G, NOTE_G, NOTE_G, NOTE_G, NOTE_FINISH};

BluetoothSerial SerialBT;

String params = "["
                "{"
                "'name':'ssid',"
                "'label':'WiFi SSID Name',"
                "'type':" + String(INPUTTEXT) + ","
                "'default':''"
                "},"
                "{"
                "'name':'pwd',"
                "'label':'WiFi Password',"
                "'type':" + String(INPUTPASSWORD) + ","
                "'default':''"
                "},"
                "{"
                "'name':'btname',"
                "'label':'Bluetooth device Name',"
                "'type':" + String(INPUTTEXT) + ","
                "'default':'Telescope'"
                "},"
                "{"
                "'name':'azsteps',"
                "'label':'Azimuth Steps',"
                "'type':" + String(INPUTNUMBER) + ","
                "'min':1,'max':10000000,"
                "'default':'" + AZIMUTH_RES + "'"
                "},"
                "{"
                "'name':'azstart',"
                "'label':'Azimuth Start Position',"
                "'type':" + String(INPUTNUMBER) + ","
                "'min':0,'max':10000000,"
                "'default':'" + AZIMUTH_START + "'"
                "},"
                "{"
                "'name':'alsteps',"
                "'label':'Altitude Steps',"
                "'type':" + String(INPUTNUMBER) + ","
                "'min':1,'max':10000000,"
                "'default':'"+ ALTITUDE_RES + "'"
                "},"
                "{"
                "'name':'alstart',"
                "'label':'Altitude Start Position',"
                "'type':" + String(INPUTNUMBER) + ","
                "'min':0,'max':10000000,"
                "'default':'"+ ALTITUDE_START + "'"
                "},"
                "{"
                "'name':'flpaz',"
                "'label':'Flip Azimuth',"
                "'type':" + String(INPUTCHECKBOX) + ","
                "'default':'1'"
                "},"
                "{"
                "'name':'flpalt',"
                "'label':'Flip Altitude',"
                "'type':" + String(INPUTCHECKBOX) + ","
                "'default':'1'"
                "},"
                "{"
                "'name':'apikey',"
                "'label':'Notification API Key (optional)',"
                "'type':" + String(INPUTTEXT) + ","
                "'max':35,"
                "'default':''"
                "},"
                "{"
                "'name':'userkey',"
                "'label':'Notification User Key (optional)',"
                "'type':" + String(INPUTTEXT) + ","
                "'max':35,"
                "'default':''"
                "}"
                "]";

WebServer server;
WiFiServer TCPserver(4030);                 // 4030 is the TCP port Skysafari usually expects for "Basic Encoder Systems"
WiFiClient serverClients[MAX_SRV_CLIENTS];
WebConfig conf;

boolean initWiFi() {
  boolean connected = false;
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to ");
  Serial.print(conf.values[0]);
  Serial.println(" SSID");
  if (conf.values[0] != "") {
    WiFi.begin(conf.values[0].c_str(), conf.values[1].c_str());
    uint8_t cnt = 0;
    while ((WiFi.status() != WL_CONNECTED) && (cnt < 20)) {
      delay(500);
      Serial.print(".");
      cnt++;
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("IP Adress:");
      Serial.println(WiFi.localIP());
      connected = true;
      // We can use the Pushover service to send a notification to your smartphone, an easy way to obtain the IP address that was assigned to the device:
      if (conf.getValue("apikey") != "" && conf.getValue("userkey") != "")
      {
        HTTPClient http;
        http.begin("https://api.pushover.net/1/messages.json");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        char _header[300];
        sprintf(_header, "token=%s&user=%s&message=Telescope-DSC connected to SSID:[%s] IP Address: [%s]", conf.getValue("apikey"), conf.getValue("userkey"), WiFi.SSID(), WiFi.localIP().toString().c_str());
        int httpResponseCode = http.POST(_header);
        http.end();
      }
    }
  }
  if (!connected) { // if unable to connect to an external WiFi, let's launch our own passwordless WiFi SSID:
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_WIFI_NETWORK, "", 1);
  }
  return connected;
}

// method that handles the clicks on "SAVE" and "RESET" buttons on the web config form
void handleRoot() {
  conf.handleFormRequest(&server);
  if (server.hasArg("SAVE"))
  {
    uint8_t cnt = conf.getCount();
    for (uint8_t i = 0; i < cnt; i++)
    {
      Serial.print(conf.getName(i));
      Serial.print(" = ");
      Serial.println(conf.values[i]);
    }
  }
}

/**
   Configure encoders using the ESP32Encoder library by Kevin Harrington.
   This uses the ESP32's hardware pulse counters, not polling nor interrupts!
   In my tests, two encoders can get simultaneous quadrature pulses at 20Khz without missing a step.
*/
void setupEncoders()
{
  // It's very important to enable pull up resistors in order to work with the NPN open collector outputs:
  ESP32Encoder::useInternalWeakPullResistors = puType::up;

  // flip the pins used by Altitude encoder if the web configuration says so:
  if (conf.getBool("flpalt"))
  {
    ALTencoder.attachFullQuad(PIN_ALT_A, PIN_ALT_B);
  }
  else
  {
    ALTencoder.attachFullQuad(PIN_ALT_B, PIN_ALT_A);
  }
  ALTencoder.setFilter(0); // filters are only needed for cheap mechanical/switch encoders such as the Keyes KY-040, not by the 600p/s encoders

  // flip the pins used by Azimuth encoder if the web configuration says so:
  if (conf.getBool("flpaz"))
  {
    AZencoder.attachFullQuad(PIN_AZ_B, PIN_AZ_A);
  }
  else
  {
    AZencoder.attachFullQuad(PIN_AZ_A, PIN_AZ_B);
  }
  AZencoder.setFilter(0); // filters are only needed for cheap mechanical/switch encoders such as the Keyes KY-040, not by the 600p/s encoders

  // set starting count value after attaching
  ALTencoder.clearCount(); //reset counter
  ALTencoder.setCount(conf.getInt("alstart")); //set starting point
  AZencoder.clearCount();
  AZencoder.setCount(conf.getInt("azstart"));
}

void processBBoxCommand(char command,char details[], char response[]) 
{
   switch (command)
        {
          case 'Q':   // the Query command, sent by SkySafari and others as the "Basic Encoder protocol" to query for encoder values.
            sprintf(response, "%lld\t%lld\t\n", AZencoder.getCount(), ALTencoder.getCount());
            break;
          case 'G': // G Command (Get Encoder Resolution) -- duplicate to 'H' ??
            snprintf(response, 20, "%d-%d\n", conf.getInt("azsteps"), conf.getInt("alsteps"));
            break;
          case 'H':   // 'H' - request for encoder resolution, e.g. 10000-10000\n
            //snprintf(response, 20, "%d-%d\n", conf.getInt("azsteps"), conf.getInt("alsteps"));
            snprintf(response, 20, "%d\t%d\t\n", conf.getInt("azsteps"), conf.getInt("alsteps"));
            break;
          case 'Z': // 'Z' - Z Command (Reset Encoders)
            ALTencoder.clearCount();
            AZencoder.clearCount();
            sprintf(response, "OK\n");
            break;
          case 'S': // 'S' S Command (Set Encoder Resolution) - S azimuth_resolution, altitude_resolution
            //S azimuth_resolution, altitude_resolution
            int altRes, azRes;
            if (sscanf(details, "%d,%d", &azRes, &altRes) == 2) {
              char strval[20];
              itoa(azRes,strval,10);
              conf.setValue("azsteps",strval);
              itoa(altRes,strval,10);
              conf.setValue("alsteps",strval);
              sprintf(response, "OK\n");
            }
            break;
          default: sprintf(response,"\n");
            #ifdef DEBUG_PROTOCOL
              Serial.print("*** UNKNOWN COMMAND ****");
            #endif
        }
    #ifdef DEBUG_PROTOCOL
    Serial.printf("[%c]{%s}(%s)\n",command,details,response);
    #endif

}

void attendTcpRequests()  // handle connections from SkySafari and similar software via TCP on port 4030
{
  uint8_t i;
  // check for new clients trying to connect
  if (TCPserver.hasClient()) {
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      // find a free or disconnected spot:
      if (!serverClients[i] || !serverClients[i].connected()) {
        if (serverClients[i]) {
          serverClients[i].stop();
        }
        serverClients[i] = TCPserver.available();
        // the new client has connected
        #ifdef DEBUG_PROTOCOL
          Serial.print("New client connected: ");
          Serial.println(serverClients[i].remoteIP());
        #endif
        break;
      }
    }
    // when there are no free/disconnected spots, reject the new client:
    if (i == MAX_SRV_CLIENTS) {
      WiFiClient serverClient = TCPserver.available();
      serverClient.stop();
      #ifdef DEBUG_PROTOCOL
          Serial.println("Client rejected. No more free spots.");
      #endif
    }
  }

  // check what the client sends for "Basic Encoder protocol" commands:
  for (i = 0; i < MAX_SRV_CLIENTS; i++)
  {
    if (serverClients[i] && serverClients[i].connected())
    {
      if (serverClients[i].available())
      {
        #ifdef DEBUG_PROTOCOL
        Serial.printf("TCP [%s:%u]",serverClients[i].remoteIP().toString(),serverClients[i].remotePort());
        #endif
        char command = serverClients[i].read(); // read the first character received, usually the command
        char requestDetails[MAX_REQUEST_LENGTH+1];
        int count=0;
        if (serverClients[i].available() && count < MAX_REQUEST_LENGTH){
          requestDetails[count] = serverClients[i].read();
          count++;
        }
        requestDetails[count]='\0';
        while (serverClients[i].available())
        {
            serverClients[i].read(); // flushing the remaining input buffer (if there are more chars than fit to request details)
        }
        char response[30];
        processBBoxCommand(command,requestDetails,response);
        serverClients[i].println(response);
      }
    }
  }
}

void attendBTRequests()   // handle connections from SkySafari and similar software via BlueTooth
{
  if (SerialBT.available()) {
    //WiFi.disconnect();
    //WiFi.mode(WIFI_OFF);

    char command = SerialBT.read();
    char requestDetails[MAX_REQUEST_LENGTH+1];
    int count=0;
    while (SerialBT.available() && count < MAX_REQUEST_LENGTH)
    {
      requestDetails[count] = SerialBT.read();
      count++;
    }
    requestDetails[count]='\0';
    while (SerialBT.available())
    {
      SerialBT.read();  // flushing the remaining input buffer (if there are more chars than fit to request details)
    }

    #ifdef DEBUG_PROTOCOL
    Serial.print("BT: ");
    #endif
    char response[30];
    processBBoxCommand(command,requestDetails,response);

    #ifdef DEBUG_PROTOCOL
    if (command=='?') printConfig();
    #endif

    SerialBT.println(response);
  }
}

void printConfig(){
    for(int i=0; i<conf.getCount();i++){
      SerialBT.print(conf.getName(i));
      SerialBT.print(" : ");
      SerialBT.println(conf.getValue(conf.getName(i).c_str()));
    }
}

void playMelody(const unsigned int melody[]){
   for(int i=0; melody[i]!=NOTE_FINISH; i++){
     tone(PIN_BEEPER, melody[i], 300);
   }
}

/**
 * Arduino main setup method
 */
void setup() {
  Serial.begin(115200);
  Serial.println(params);
  playMelody(melodyStart);
  pinMode(PIN_RESET, INPUT_PULLDOWN);

  conf.setDescription(params);
  conf.readConfig();
  initWiFi();
  char dns[30];
  sprintf(dns, "%s.local", conf.getApName());
  if (MDNS.begin(dns))
  {
    Serial.println("DNS active");
  }
  server.on("/", handleRoot);
  server.begin(80); // starts web server for configuration page

  TCPserver.begin(); // starts TCP server for skysafari via WiFi

  setupEncoders();

  if (conf.getValue("btname") != "")  // starts Bluetooth service only if a name has been configured for our device
  {
    SerialBT.begin(conf.getValue("btname"));
    Serial.println("BT started");
  }
  playMelody(melodySetupCompleted);

}

void loop() 
{
  if (digitalRead(PIN_RESET) == HIGH)
  {
    //reset counters - AZ - 0 ; ALT - 0
    ALTencoder.clearCount(); 
    AZencoder.clearCount();
    Serial.println("ALT&AZ counters zeroed");
    tone(PIN_BEEPER, NOTE_A2);
    delay(300);
    noTone(PIN_BEEPER);
    delay(2000);
    if (digitalRead(PIN_RESET) == HIGH)
    {
      Serial.println("Going to restart");
      tone(PIN_BEEPER, NOTE_A);
      delay(600);
      ESP.restart();      
    }
  }
  server.handleClient();
  attendBTRequests();
  attendTcpRequests();
}
