#ifndef display_h
#define display_h

#define PINRCK   00  //
#define PINSRCK  00  //
#define PINENA   00  //
#define PINDIN   00  //

#define PINMOTOR-A1 00
#define PINMOTOR-A2 00 
#define PINMOTOR-B1 00 
#define PINMOTOR-B2 00

#define FONT_W     5  // Breite der Zeichen in Pixel


#include <Max72xxPanel.h>       // https://github.com/markruys/arduino-Max72xxPanel.git

class LEDmatrix {
  public:
    LEDmatrix( uint8_t spacer);
    void loop();
    void setSpeed(uint8_t speedval);
    void setMode(uint8_t mode, const char *content);

  private:
    uint8_t   _pinRCK;
    uint8_t   _pinSRCK;
    uint8_t   _pinENA;
    uint8_t   _pinDIN;
    uint8_t   _pinMOTOR-A1;
    uint8_t   _pinMOTOR-A2;
    uint8_t   _pinMOTOR-B1;
    uint8_t   _pinMOTOR-B2;
    uint8_t   _width;     // width of character incl. spacers
    uint8_t   _spacer;    // distance between characters
    uint16_t  _wait;      // time to wait between single frames, calculated from speed
    uint32_t  _lastRun;
    int16_t   _currPos;
    uint8_t   _contentMode;
    uint16_t  _contentPt;
    char      _content[200];
    File      _graphFile; 
    int16_t   _nextGraphPos;
    
    void      _loopDateTime();
    void      _loopTime();
    void      _loopText(boolean doLoop);
    void      _loopGraphics();
    void      _loopIP();
    void      _drawLine(int16_t x, uint8_t matrixElem, uint8_t color, uint8_t bg);

};



#endif //display_h
