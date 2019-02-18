boolean pending_reboot = false;
volatile boolean scanForWLANS = true;
volatile uint8_t semaWLAN = 0;

uint8_t setWLANsema() {
  uint8_t result=0;
  noInterrupts();
    if (0 == semaWLAN) {
      semaWLAN=1;
      result=1;
    }
  interrupts();
  return( result);  
}

uint8_t clearWLANsema() {
  uint8_t result=1;
  noInterrupts();
    if (1 == semaWLAN) {
      semaWLAN=0;
      result=0;
    }
  interrupts();
  return( result);  
}

// Webserver
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws



void setupWebserver() { 
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.addHandler(new SPIFFSEditor(GlobalConfig.myAdminUser,GlobalConfig.myAdminPWD));
  server.on("/contentsave", HTTP_POST|HTTP_GET, handleOnContentSave);
  server.on("/saveConfig", HTTP_POST|HTTP_GET, handleSaveConfig);
  server.on("/getConfigData", HTTP_POST|HTTP_GET, handleGetConfigData);
  server.on("/wifiRestartAP", HTTP_POST, handleWiFiRestartAP);
  server.on("/wifiRestartSTA", HTTP_GET, handleWiFiRestartSTA);
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  server.begin();
}

void printFreeHeap(int pagesize) {
  Serial.print("Free Heap: ");
  Serial.print(system_get_free_heap_size());
  Serial.print("  Lenght page: ");
  Serial.println(pagesize);
}




void handleGetConfigData( AsyncWebServerRequest *request){
  if(!request->authenticate( GlobalConfig.myAdminUser, GlobalConfig.myAdminPWD))
    return request->requestAuthentication();

  struct T_ConfigStruct tempConf;
  char myAdminPWD2[20];
  char buffer[200];
  snprintf(buffer, sizeof(buffer), "{\"SSID\": \"%s\", \"WLANPWD\": \"%s\", \"CHANNEL\": \"%u\", \"ADMPWD\": \"%s\"}\n", 
           GlobalConfig.mySSID, GlobalConfig.myPWD, GlobalConfig.myChannel, GlobalConfig.myAdminPWD);
  request->send(200, "text/html", buffer);
}





void handleSaveConfig(AsyncWebServerRequest *request){
  if(!request->authenticate(GlobalConfig.myAdminUser, GlobalConfig.myAdminPWD))
    return request->requestAuthentication();
  Serial.println("save config");

  struct T_ConfigStruct tempConf;
  char myAdminPWD2[20];
  
  int params = request->params();
  for(int i=0;i<params;i++){
    AsyncWebParameter* p = request->getParam(i);
    if(! p->isFile()) {  // ONLY for GET and POST
      if (! strcmp(p->name().c_str(), "SSID")) {
        strlcpy( tempConf.mySSID, p->value().c_str(), sizeof(tempConf.mySSID));
        Serial.print("SSID: "); Serial.println(tempConf.mySSID);
      } 
      if (! strcmp(p->name().c_str(), "WLANPWD")) {
        strlcpy( tempConf.myPWD, p->value().c_str(), sizeof(tempConf.myPWD));
        Serial.print("WLAN-PWD: "); Serial.println(tempConf.myPWD);
      } 
      if (! strcmp(p->name().c_str(), "Kanal")) {
         tempConf.myChannel = atoi(p->value().c_str());
        Serial.print("Channel: "); Serial.println(tempConf.myChannel);
      } 
      if (! strcmp(p->name().c_str(), "ADMPWD")) {
        strlcpy( tempConf.myAdminPWD, p->value().c_str(), sizeof(tempConf.myAdminPWD));
        Serial.print("ADM: "); Serial.println(tempConf.myAdminPWD);
      } 
      if (! strcmp(p->name().c_str(), "ADMPWD2")) {
        strlcpy( myAdminPWD2, p->value().c_str(), sizeof(myAdminPWD2));
        Serial.print("ADMPWD2: "); Serial.println(myAdminPWD2);
      } 
    }
  }
  if (params >2)  {
    Serial.println("Configuration complete");
    if((strlen(tempConf.myAdminPWD) > 0) and 
       (strlen(myAdminPWD2) >0) and
       (0 == strncmp(tempConf.myAdminPWD, myAdminPWD2, sizeof(myAdminPWD2))))  {
      strlcpy(tempConf.magic, CONF_MAGIC, sizeof(tempConf.magic));
      tempConf.version=CONF_VERSION;
      ConfigSave(&tempConf);
      LoadAndCheckConfiguration();
      request->send(204, "text/plain", "OK");
      return;
    } else {
      request->send(204, "text/plain", "ERROR");
    }
  } else {
    request->send(204, "text/plain", "ERROR");
  }
}





void handleWiFiRestartAP(AsyncWebServerRequest *request){
  handleWiFiRestart(request, true);
}
void handleWiFiRestartSTA(AsyncWebServerRequest *request){
  handleWiFiRestart(request, false);
}

void handleWiFiRestart(AsyncWebServerRequest *request, boolean asAP){
  char  clientSSID[32] = "";
  char  clientPWD[64]  = "";  
  Serial.println("WiFiSave start");
  int params = request->params();
  String page = "";
  if (asAP) {
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(! p->isFile()) {  // ONLY for GET and POST
        if (! strcmp(p->name().c_str(), "s")) {
          strlcpy(  clientSSID, p->value().c_str(), sizeof( clientSSID));
          Serial.println("SSID received!");
        }
        if (! strcmp(p->name().c_str(), "p")) {
          strlcpy(  clientPWD, p->value().c_str(), sizeof( clientPWD));
          Serial.println("PWD received!");
        }
      }
    }
  }
  File menuFile = SPIFFS.open("/decodeIP.html", "r");
  while (menuFile.available()){
    Serial.println("  reading line...");
    page += menuFile.readStringUntil('\n');
  }
  request->send(200, "text/html", page);
  Serial.println("Request done");
  struct station_config newConf;  memset(&newConf, 0, sizeof(newConf));
  strlcpy((char *)newConf.ssid, clientSSID, strlen(clientSSID)+1);
  strlcpy((char *)newConf.password, clientPWD, strlen(clientPWD)+1);
  ETS_UART_INTR_DISABLE();
  wifi_station_set_config(&newConf);
  ETS_UART_INTR_ENABLE();
  Serial.println("New config is written");
  pending_reboot=true; //make sure, ESP gets rebooted
}





void handleOnContentSave(AsyncWebServerRequest *request){
  Serial.println("ContentSave");
  if (request->hasArg("content")) {
    AsyncWebParameter* p = request->getParam("content", true);
    Serial.println("request content ");
    uint8_t myContent=strtol(p->value().c_str(), NULL, 10);
    if (myContent > 5) {myContent=4;}
    switch (myContent) {
      case 0:  p = request->getParam("text", true);
               Serial.println("content text");Serial.println(p->value().c_str());
               myDisplay._setMode(0, p->value().c_str());
               break;
      case 1:
      case 2:
      case 3:  myDisplay._setMode(myContent, "");          
               break;
      case 5:  p = request->getParam("graphics", true);
               Serial.print("graphics ");//Serial.println(p->value().c_str());
               myDisplay._setMode(myContent, p->value().c_str());
               break;
      default: myDisplay._setMode(myContent, "");                     
               break;
    }
  }  
  if (request->hasArg("speed")) {
    AsyncWebParameter* p = request->getParam("speed", true);
    Serial.println("request speed ");
    myDisplay._setSpeed(strtol(p->value().c_str(), NULL, 10));
  }
  if (request->hasArg("brightness")) {
    AsyncWebParameter* p = request->getParam("brightness", true);
    Serial.println("request brightness ");
    myDisplay._setBright(strtol(p->value().c_str(), NULL, 10));
  }
  request->send(204, "text/html");
}


void sendWLANscanResult( AsyncWebSocketClient * client) {
  if ( 1 == setWLANsema()) {
    int16_t numWLANs=  WiFi.scanComplete();
    if (0 < numWLANs) {
      for (uint16_t i=0; i<numWLANs; i++) {
        char buffer[200];
        snprintf(buffer, sizeof(buffer), "JSONDATA{\"SSID\": \"%s\", \"encType\": %u, \"RSSI\": \"%d\", \"BSSID\": \"%s\", \"Channel\": %u, \"Hidden\": \"%s\"}",
                WiFi.SSID(i).c_str(), WiFi.encryptionType(i), WiFi.RSSI(i), WiFi.BSSIDstr(i).c_str(), WiFi.channel(i), WiFi.isHidden(i) ? "true" : "false");
        client->text(buffer);
      }
    } else {
      client->text("[\"no WLAN found\"]\n");
    }
    clearWLANsema();
  } else {
    client->text("[\"no WLAN found\"]\n");
  }
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  StaticJsonBuffer<300> jsonBuffer;
  Serial.println("Websockets fired");
  if(type == WS_EVT_CONNECT){
    //client connected
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
  } else if(type == WS_EVT_ERROR){
  } else if(type == WS_EVT_PONG){
    // pong message was received (in response to a ping request maybe)
    // os_printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    //data packet
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      if(info->opcode == WS_TEXT) {
        data[len] = 0;
        JsonObject &root = jsonBuffer.parseObject((char*)data);
       if ( !root.success() ) {
        } else {
          const char* magic = root["magic"];
          const char* cmd = root["cmd"];
          if (0 == strncmp("FLKA", magic, 4)) {
            if(0 == strncmp("LISTWLANS", cmd, 9)) {
              sendWLANscanResult(client);
              scanForWLANS = true;
              Serial.println("scan4wlan true!");
            }
          }
        }
      } else {
        // we currently dont react on binary messages
      }  
    } else {
      // we don't expect fragmentation either
    }
  }
}


void do_pending_Actions(void){
  // do all things, which need to be done after an asynchronous request
  if (pending_reboot) {
    Serial.println("Restarting system, hold on");
    delay(4000);
    // Sicherstellen, dass die GPIO-Pins dem Bootloader das erneute Starten des Programmes erlauben!
    pinMode( 0,OUTPUT);
    pinMode( 2,OUTPUT);
    pinMode(15,OUTPUT);
    WiFi.forceSleepBegin(); 
    wdt_reset();  
    digitalWrite(15, LOW);
    digitalWrite( 0,HIGH);
    digitalWrite( 2,HIGH);
    delay(50);
    ESP.restart();
  }  
  if (scanForWLANS) {
    if ( 1==setWLANsema()) {
      int16_t noWlans= WiFi.scanComplete();
      if ( noWlans != -1 ) {
        WiFi.scanNetworks(true, true);
        scanForWLANS = false;
      }  
      clearWLANsema();
    }  
  }
   
}

void setupTime()
{
  memset(NTPpacket, 0, sizeof(NTPpacket));
  NTPpacket[0]=0b11100011;
  NTPpacket[1]=0;
  NTPpacket[2]=6;
  NTPpacket[3]=0xEC;
  NTPpacket[12]=49;
  NTPpacket[13]=0x4E;
  NTPpacket[14]=49;
  NTPpacket[15]=52;
  DNSClient dns;
  dns.begin(WiFi.dnsIP());
  Serial.println("dns started");
  WiFi.hostByName(TimeServerName,timeServer);
  if(ntpClient.connect(timeServer, TimeServerPort)) {
    Serial.print("NTP UDP connected: "); Serial.println(timeServer);
    ntpClient.onPacket([](AsyncUDPPacket packet) {
        unsigned long highWord = word(packet.data()[40], packet.data()[41]);
        unsigned long lowWord = word(packet.data()[42], packet.data()[43]);
        time_t UnixUTCtime = (highWord << 16 | lowWord)-2208988800UL;
        setTime(UnixUTCtime);
        Serial.println("received NTP packet!");
      });
    } else {Serial.println("No connection to NTP Server possible");} 
  ntpClient.write(NTPpacket, sizeof(NTPpacket));  
}
