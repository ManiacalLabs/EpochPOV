#ifndef __GLOBALS__
#define __GLOBALS__

#include "Arduino.h"

volatile uint32_t *povData;
volatile uint32_t povA, povB;
volatile uint8_t povStep = 0;

unsigned long timeRef;
unsigned long timeOutRef;

volatile uint8_t bSave = 0;
volatile uint16_t bCount = 0;
volatile boolean holdFlag = false;
uint16_t holdMax = 60;

//Valid state machine states
#define STATE_NONE        0
#define STATE_SERIAL_DATA 1

volatile uint8_t curState = STATE_NONE;

//helpers for button handling
#define BUTTON_A _BV(PIND2)
#define BUTTON_B _BV(PIND3)
#define BUTTON_MASK (BUTTON_A | BUTTON_B)
#define BUTTON_STATE PIND & BUTTON_MASK

//Who can ever remember what the prescaler combinations are?
//These are for Timer0
#define PRESCALE0_1 _BV(CS00)
#define PRESCALE0_8 _BV(CS01)
#define PRESCALE0_64 (_BV(CS01) | _BV(CS00))
#define PRESCALE0_256 _BV(CS02)
#define PRESCALE0_1024 (_BV(CS02) | _BV(CS00))

//These are for Timer1
#define PRESCALE1_1 _BV(CS10)
#define PRESCALE1_8 _BV(CS11)
#define PRESCALE1_64 (_BV(CS11) | _BV(CS10))
#define PRESCALE1_256 _BV(CS12)
#define PRESCALE1_1024 (_BV(CS12) | _BV(CS10))

//These are for Timer2
#define PRESCALE2_1 _BV(CS20)
#define PRESCALE2_8 _BV(CS21)
#define PRESCALE2_32 (_BV(CS21) | _BV(CS20))
#define PRESCALE2_64 _BV(CS22)
#define PRESCALE2_128 (_BV(CS20) | _BV(CS22))
#define PRESCALE2_256 (_BV(CS22) | _BV(CS21))
#define PRESCALE2_1024 (_BV(CS22) | _BV(CS21) | _BV(CS20))

//Used for the larson scanner effect during Serial Set mode.
volatile byte _serialScan = 0;
volatile byte _serialScanStep = 0;
bool _serialScanDir = false;
const uint8_t scanLevels[] = {10,3,1}; 

//For getting the time over serial connection
#define SYNC_LEN 5  // time sync is 1 byte header + 4 byte time_t
#define SYNC_HEADER 't'  // header for time sync

//Change this to whatever your computer serial connection is set to
//TODO: Change this to whatever the arduino drivers default to.
#define SERIAL_BAUD 115200

//Helpers for Serial printing
#define OD(x) Serial.print(x, DEC)
#define OS(x) Serial.print(x)
#endif //__GLOBALS__
