/****************************************************************************************************
*                                                                                                   *
*****************************************************************************************************/

#include "AllIncludes.h"

// globale Variablen ==============================================================================//

// Konfiguration für Flash
struct T_ConfigStruct { 
  char    magic[5] = CONF_MAGIC;
  uint8_t version  = CONF_VERSION;
  char    mySSID[32] = CONF_DEFAULT_SSID;
  char    myPWD[64]  = CONF_DEFAULT_PWD;
  char    myHostname[20] = CONF_HOSTNAME;
  char    myAdminUser[20] = CONF_ADMIN_USER;
  char    myAdminPWD[20] = CONF_ADMIN_PWD;
  
  uint8_t myChannel  = CONF_DEFAULT_CHANNEL;
  
} GlobalConfig;

void ConfigSave( struct T_ConfigStruct* conf) {
   EEPROM.begin(512);
   uint8_t* confAsArray = (uint8_t *) conf;
   for (uint16_t i=0; i<sizeof(T_ConfigStruct); i++) {
     EEPROM.write(i,confAsArray[i]);
   }
   EEPROM.commit();
   EEPROM.end();
}

void ConfigLoad( struct T_ConfigStruct* conf) {
   EEPROM.begin(512);
   uint8_t* confAsArray = (uint8_t*) conf;
   for (uint16_t i=0; i<sizeof(T_ConfigStruct); i++) {
     confAsArray[i]=EEPROM.read(i);
   }
   EEPROM.commit();
   EEPROM.end();
}

void LoadAndCheckConfiguration( void) {
  struct T_ConfigStruct newConf;
  ConfigLoad(&newConf);
  if (strncmp(newConf.magic, GlobalConfig.magic, sizeof(newConf.magic)) == 0){
    Serial.println("Magic string from configuration is ok, overload configuration from eeprom");
    ConfigLoad(&GlobalConfig);
  } else {
    Serial.println("Magic string mismatch, keeping with default configuration");
  }
}

char myIPaddress[18]="000.000.000.000  ";

// Zeitserver und Zeithandling
IPAddress timeServer(0, 0, 0, 0);  // wird per NTP ermittelt
AsyncUDP ntpClient;                // asynchroner UDP Socket für NTP-Abfragen
uint8_t NTPpacket[ 48];            // Anfragepaket für NTP-Anfragen
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       //Central European Standard Time
TimeChangeRule *tcr;  // Zeiger auf aktuelle Zeitzone
Timezone CE(CEST, CET);
time_t localTime;

class RotationDisplay: public PovDisplay {
  public:
    RotationDisplay( uint8_t  LedData, uint8_t  LedClk, uint8_t  LedEna, uint8_t  LedLatch, 
                                      uint8_t  M_A1, uint8_t  M_A2, uint8_t  M_B1, uint8_t  M_B2, 
                                      uint8_t steps_per_pixel, uint8_t highlighted_steps, uint8_t column_offset, 
                                      float rpm, uint8_t m_direction, uint8_t spacer);
                                      //: PovDisplay( LedData, LedClk, LedEna, LedLatch, M_A1, M_A2, M_B1, M_B2, \
                                      //  steps_per_pixel, highlighted_steps, column_offset, rpm, m_direction);
    void _loop();
    void _setBright(uint8_t brightness);
    void _setSpeed(uint8_t speedval);
    void _setMode(uint8_t mode, const char *content);
  private:
    uint8_t   _spacer;
    uint8_t   _contentMode;
    uint16_t  _contentPt;
    char      _content[200];
    File      _graphFile; 
    void      _loopDateTime();
    void      _loopText();
    void      _loopGraphics();
                                    
                                       
};  

RotationDisplay::RotationDisplay( uint8_t  LedData, uint8_t  LedClk, uint8_t  LedEna, uint8_t  LedLatch, 
                                  uint8_t  M_A1, uint8_t  M_A2, uint8_t  M_B1, uint8_t  M_B2, 
                                  uint8_t steps_per_pixel, uint8_t highlighted_steps, uint8_t column_offset, 
                                  float rpm, uint8_t m_direction, uint8_t spacer) \
                                  : PovDisplay( LedData, LedClk, LedEna, LedLatch, M_A1, M_A2, M_B1, M_B2, \
                                  steps_per_pixel, highlighted_steps, column_offset, rpm, m_direction){
  _spacer = spacer;
  _contentPt   = 0;
  _contentMode = 4;
  strlcpy( _content, "no data ", sizeof(_content));
                                    
}

void RotationDisplay::_setMode(uint8_t mode, const char *content){
  strlcpy( _content, content, sizeof(_content));
  if(0 == strlen(_content)) { strlcpy(_content, "no text! ", sizeof(_content)); }
  _contentMode = mode;
  _contentPt   = 0;
  if (0 == _contentMode) {  // normal Text
    for( uint16_t j=0; j<=strlen(_content); j++){
      // map certain latin1 characters to extended ascii cp437
      // as that is the standard code from most fixed size fonts
      switch(_content[j]) {
        case 0xA7: _content[j]=0x15; break; // §
        case 0xB5: _content[j]=0xE5; break; // µ
        case 0xB0: _content[j]=0xF7; break; // °
        case 0xB2: _content[j]=0xFC; break; // ²
        case 0xFC: _content[j]=0x81; break; // ü
        case 0xE4: _content[j]=0x84; break; // ä
        case 0xEB: _content[j]=0x89; break; // ë
        case 0xC4: _content[j]=0x8E; break; // Ä
        case 0xCB: _content[j]=0x45; break; // Ë->E
        case 0xD8: _content[j]=0xEC; break; // 
        case 0xF6: _content[j]=0x94; break; // ö
        case 0xD6: _content[j]=0x99; break; // Ö
        case 0xDF: _content[j]=0xE1; break; // ß
        case 0xDC: _content[j]=0x9A; break; // Ü
        case 0xF7: _content[j]=0xF5; break; // 
      }
    }
  }
  if (5 == _contentMode) {  // Graphics
    if (_graphFile.available()) { _graphFile.close(); }
    _graphFile = SPIFFS.open(_content, "r");
  }  
  if (6 == _contentMode) {  // IP-Address
    strlcpy(_content, myIPaddress, sizeof(_content)); strlcat(_content, "  ", sizeof(_content));  
  }  
  if ((0 < _contentMode) && (4 > _contentMode)) {
   ntpClient.write(NTPpacket, sizeof(NTPpacket)); 
   strlcpy(_content, "receiving time ", sizeof(_content)); 
  }
}  

void RotationDisplay::_setBright(uint8_t brightness){
  uint8_t highlight = map(brightness, 0,15,0, STEPS_PER_PIXEL);
  _set_highlighted_steps(highlight);
}
   
void RotationDisplay::_setSpeed(uint8_t speedval){
  _set_speed(1.0+(speedval/28.0));
}

void RotationDisplay::_loop(){
  switch( _contentMode) {
    case  0: _loopText(); break;
    case  1:
    case  2:
    case  3: _loopDateTime(); break;
    case  5: _loopGraphics(); break;
    default: _loopText();  // 6 is IP address
  }

}

char* byte2text(uint16_t value){
  static char convbuffer[6];
  memset(convbuffer, 0, sizeof(convbuffer));
  if (value < 10) {
    convbuffer[0]=0x30;
    convbuffer[1]=value+0x30;
  } else 
  itoa(value,convbuffer, 10);
  return convbuffer;
}

void RotationDisplay::_loopDateTime(){
  time_t t = CE.toLocal(now(), &tcr);
  if (0 == _contentPt) {
    memset(_content, 0, sizeof(_content));
    switch (_contentMode) {
      case 1: 
        strlcat(_content, dayStr(weekday(t)),  sizeof(_content));
        strlcat(_content, " ",  sizeof(_content));
        strlcat(_content, byte2text(day(t)),  sizeof(_content));
        strlcat(_content, ". ",  sizeof(_content));
        strlcat(_content, monthStr(month(t)),  sizeof(_content));
        strlcat(_content, " ",  sizeof(_content));
        strlcat(_content, byte2text(year(t)),  sizeof(_content));
        strlcat(_content, " ",  sizeof(_content));
        strlcat(_content, byte2text(hour(t)),  sizeof(_content));
        strlcat(_content, ":",  sizeof(_content));
        strlcat(_content, byte2text(minute(t)),  sizeof(_content));
        strlcat(_content, "  ",  sizeof(_content));
        break;
      case 2:
        strlcat(_content, byte2text(day(t)),  sizeof(_content));
        strlcat(_content, "-",  sizeof(_content));
        strlcat(_content, byte2text(month(t)),  sizeof(_content));
        strlcat(_content, "-",  sizeof(_content));
        strlcat(_content, byte2text(year(t)),  sizeof(_content));
        strlcat(_content, " ",  sizeof(_content));
        strlcat(_content, byte2text(hour(t)),  sizeof(_content));
        strlcat(_content, ":",  sizeof(_content));
        strlcat(_content, byte2text(minute(t)),  sizeof(_content));
        strlcat(_content, "  ",  sizeof(_content));
        break;
      case 3: 
        strlcat(_content, byte2text(hour(t)),  sizeof(_content));
        strlcat(_content, ":",  sizeof(_content));
        strlcat(_content, byte2text(minute(t)),  sizeof(_content));
        strlcat(_content, "  ",  sizeof(_content));
        break;
      default: strlcpy(_content,  "no valid time ", sizeof(_content)); break;
    }    
  }  
  if(((minute(t)) % 20) == 0){
    ntpClient.write(NTPpacket, sizeof(NTPpacket));
  }
  _loopText();
}

void RotationDisplay::_loopText(){
  //Serial.printf("Content: [%s] Pos: [%d] Char: [%c]\n",_content, _contentPt, _content[_contentPt]);
  for ( int j = 0; j < FONTCOLS; j++) {
    while ( not _set_next_column(font[_content[_contentPt]][j])) {yield();};
    /*
    Serial.print("  ");
    for( uint8_t i = 0; i < 8; Serial.print( bitRead(font[_content[_contentPt]][j], i++) ? "#" : " "));
    Serial.println();
    */
  }
  
  _contentPt = (_contentPt + 1) % strlen(_content);
}


void RotationDisplay::_loopGraphics(){
  uint8_t graphLine[1];
  for ( int j = 0; j < 10; j++) {
    _graphFile.seek(_contentPt,SeekSet);
    _graphFile.read(graphLine, sizeof(graphLine));
    while ( not _set_next_column(graphLine[0]) ) {yield();};
    _contentPt = (_contentPt + 1) % _graphFile.size() ; 
  }
}


RotationDisplay myDisplay(LED_DATA, LED_CLK, LED_ENABLE, LED_LATCH, 
                          MOTOR_A1, MOTOR_B1, MOTOR_A2, MOTOR_B2, 
                          STEPS_PER_PIXEL, 18, PIXEL_OFFSET, 2, CW, SPACER);






void setup(){
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  LoadAndCheckConfiguration();
  WiFi.hostname(GlobalConfig.myHostname);
  ETS_UART_INTR_DISABLE();
  wifi_station_disconnect();
  ETS_UART_INTR_ENABLE();
  SPIFFS.begin();
  WiFi.setOutputPower(20.5); // möglicher Range: 0.0f ... 20.5f
  WiFi.mode(WIFI_STA);
  Serial.println("Start in Client Mode!");
  WiFi.begin();
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Start in Access Point Mode!");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(GlobalConfig.mySSID, GlobalConfig.myPWD, GlobalConfig.myChannel);
    strlcpy(myIPaddress, WiFi.softAPIP().toString().c_str(), sizeof(myIPaddress));
    Serial.println(GlobalConfig.mySSID);
  } else {
    WiFi.setAutoReconnect(true);
    strlcpy(myIPaddress, WiFi.localIP().toString().c_str(), sizeof(myIPaddress));
  }
   
  int16_t numWLANs=  WiFi.scanComplete();
  if (numWLANs == -2) { // es gab bislang noch keinen WLAN-Scan
    WiFi.scanNetworks(true);
  }
  setupTime();
  setupWebserver();
  myDisplay._setMode(6,"");
}



void loop() {
  myDisplay._loop();
  do_pending_Webserver_Actions();
}


