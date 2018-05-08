#include "PovDisplay.h"

/*  Die Ansteuerung des Displays ist darauf vorbereitet, dass die LEDs in zwei Spalten (A und B) versetzt angeordnet sind.
 *   A B
 *   O
 *     O
 *   O
 *     O
 *   O
 *     O
 *   O
 *     O
 *     
 *  Mit einer solchen Anordnung können die LEDs das Raster sehr kompakt darstellen.  
 *  In einem solchen Fall wird mit column_offset der Schrittabstandd zwischen beiden   
 *  Spalten konfiguriert.  Mit einem Offset von 0 liegen die LEDs in einer Reihe.
 */

PovDisplay::PovDisplay( uint8_t  LedData, uint8_t  LedClk, uint8_t  LedEna, uint8_t  LedLatch, 
            uint8_t  M_A1, uint8_t  M_A2, uint8_t  M_B1, uint8_t  M_B2, 
            uint8_t steps_per_pixel, uint8_t highlighted_steps, uint8_t column_offset, 
            float rpm, uint8_t m_direction) {
  _motorPins[0] = M_A1; _motorPins[1] = M_A2;
  _motorPins[2] = M_B1; _motorPins[3] = M_B2;
  _pinLedData        = LedData; 
  _pinLedClk         = LedClk;
  _pinLedEna         = LedEna;
  _pinLedLatch       = LedLatch;
  _steps_per_pixel   = steps_per_pixel;
  _highlighted_steps = highlighted_steps;
  _column_offset     = column_offset;
  _step_delay        = uint16_t  (1000/(rpm*STEPS_PER_ROTATION/60))  ;
  _motor_direction   = m_direction;
  _MotorStepState    = 0;
  _currPixelStep     = 0;
  _bufsize           = sizeof(_ringbuffer);
  pinMode(_pinLedData, OUTPUT);
  pinMode(_pinLedClk, OUTPUT);
  pinMode(_pinLedEna, OUTPUT);
  pinMode(_pinLedLatch, OUTPUT);
  digitalWrite(_pinLedEna, LOW);
  for (uint8_t i=0; i<4; i++) {
    pinMode(_motorPins[i], OUTPUT);
  } 
  _currBackStep = ( 3*_steps_per_pixel - _column_offset) % _steps_per_pixel;
  _backstart = _bufsize - ((_column_offset+_steps_per_pixel-1) / _steps_per_pixel); 
  os_timer_setfn( &_myTimer, [](void *pArg){PovDisplay *currClass = static_cast<PovDisplay*>(pArg);currClass->_do_next_step();}, this);
  _start_rotating();
}


void PovDisplay::_start_rotating(){
  os_timer_arm(&_myTimer, _step_delay, true);
}

void PovDisplay::_stop_rotating(){
  os_timer_disarm(&_myTimer);
}

void PovDisplay::_set_rotation(uint8_t dir){
  _motor_direction=dir;
}

void PovDisplay::_set_highlighted_steps(uint8_t stepcount){
  _highlighted_steps = min(stepcount, _steps_per_pixel); //make sure, that there are not more highlited pixels than steps!
}

void PovDisplay::_set_speed(float rpm){
  _step_delay        = uint16_t  (1000 / (rpm*STEPS_PER_ROTATION/60))  ;
  _stop_rotating(); _start_rotating();
}

bool PovDisplay::_set_next_column (uint8_t value){
  // store another column in ringbuffer, if ringbuffer has space left
  // return true, if storing was succesful, otherwise return false
  if( (_bufend % _bufsize) != ((_bufstart+_bufsize-2) % _bufsize ) ) {
    _ringbuffer[_bufend]=value;
    _bufend=(_bufend+1) % _bufsize;
    _bufferfull=false;
    return true;
  } else {
    return false;
  }
}


void PovDisplay::_do_next_step( ) {
  uint8_t output = 0b00000000;
  
  if (_currPixelStep <= _highlighted_steps) {
    output = (_ringbuffer[_bufstart] & 0b10101010);
  } else {
    output = output & 0b01010101;
  }
  if (_currBackStep <= _highlighted_steps) {
    output = output | (_ringbuffer[_backstart] & 0b01010101);
  } else {
    output = output & 0b10101010;
  }
  digitalWrite(_pinLedClk, LOW);
  shiftOut(_pinLedData, _pinLedClk, LSBFIRST, output);
  digitalWrite(_pinLedLatch, HIGH);
  digitalWrite(_pinLedLatch, LOW);
  
  _MotorStepState=(_MotorStepState+8+_motor_direction) % 8;
  _currPixelStep = (_currPixelStep+1) % _steps_per_pixel;
  _currBackStep  = (_currBackStep+1)  % _steps_per_pixel;
  if (_currPixelStep == 0 ) {
    if(  _bufstart !=  _bufend ) {
      _bufstart=(_bufstart+1) % _bufsize;
    } else {
      _bufferfull=true;
    }
  }
  if ((_currBackStep == 0) &&(! _bufferfull) ) {
      _backstart=(_backstart+1) % _bufsize;
  }
  if (! _bufferfull ) {
    for (uint8_t i=0; i<4; i++) {
      digitalWrite(_motorPins[i], bitRead(_stepValues[_MotorStepState],i));
    } 
  }  
}

void PovDisplay::_debug_me(){
  // put Debug output of private members here
}  

