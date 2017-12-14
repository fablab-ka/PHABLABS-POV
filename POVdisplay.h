#include <Arduino.h>

#pragma once


#define CCW   1
#define CW   -1
#define STEPS_PER_ROTATION 4096  // steps needed for 360°


extern "C"
{
  #include "user_interface.h"
}


class PovDisplay {
  public:
    PovDisplay( uint8_t  LedData, uint8_t  LedClk, uint8_t  LedEna, uint8_t  LedLatch, 
                uint8_t  M_A1, uint8_t  M_A2, uint8_t  M_B1, uint8_t  M_B2, 
                uint8_t steps_per_pixel, uint8_t highlighted_steps,  uint8_t column_offset, 
                float rpm, uint8_t m_direction);
    static PovDisplay * mySelf;
    static os_timer_t myTimer;
    void _start_rotating();
    void _stop_rotating();
    void _set_rotation(uint8_t dir);
    void _set_highlighted_steps(uint8_t stepcount);
    void _set_speed(float rpm);
    bool _set_next_column (uint8_t value);
    void _debug_me();
 
  private:
    const uint8_t _stepValues[8] = {0b1010, 0b0010, 0b0110, 0b0100, 0b0101, 0b0001, 0b1001, 0b1000};
    uint8_t  _pinLedData, _pinLedClk, _pinLedEna, _pinLedLatch; // shift register for LEDs
    uint8_t  _motorPins[4];                                     // pins for two stepper motor coils
    uint8_t  _steps_per_pixel, _highlighted_steps;              // steps per pixel, thereof number of steps with LED on
    uint8_t  _column_offset;                                    // offset between the two columns of 4 LEDs each in steps
    uint16_t _step_delay;                                       // delay between each step (calculated from rpm)
    uint8_t  _motor_direction;                                  // 1, -1 : cw, ccw
    uint8_t  _MotorStepState;
    uint8_t  _currPixelStep;
    uint8_t  _currBackStep;
    uint8_t  _ringbuffer[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};           // buffer for outputting data
    volatile uint8_t  _bufstart=0;                              // pointer to buffer start
    volatile uint8_t  _backstart;                               // pointer to buffer start for backpixel
    volatile uint8_t  _bufend=1;                                // pointer to buffer end
    volatile bool _bufferfull=false;
    uint8_t  _bufsize;
    static void _callback_helper(void *pArg);
    void _do_next_step();                                   // function for ISR timer
      
};

//#endif

