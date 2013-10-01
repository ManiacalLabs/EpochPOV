/*
Version 1.0
Copyright (c) 2013 Adam Haile & Dan Ternes.  All right reserved.
http://ManiacalLabs.com
*/

/*
Pinout Information
PC0 - PC3: Common cathode Outputs for display
PINB0-PINB5,PIND6-PIND7: Display row anodes
PC4 & PC5: SDA * SCL for I2C to RTC 
PIND0 & PIND1: RX & TX for Serial
PIND2 & PIND3: Mode & Change buttons
PIND5: Reset Enable/Disable
*/

#include <Wire.h>
#include "EEPROM.h"

#include "globals.h"

#include "Image.h"


void setup()
{
	//init reference for time delays. used with TimeElapsed()  calls.
	//set to 0 instead of millis() so it always triggers right away the first time
	timeRef = 0;

	//Setup common cathodes as outputs
	DDRC |= (_BV(PINC0) | _BV(PINC1) | _BV(PINC2) | _BV(PINC3));
	//Setup rows as outputs
	DDRB |= (_BV(PINB0) | _BV(PINB1) | _BV(PINB2) | _BV(PINB3) | _BV(PINB4) | _BV(PINB5));
	DDRD |= (_BV(PIND6) | _BV(PIND7));

	//Set PORTD2 and PORTD3 as inputs
	DDRD &= ~(_BV(PIND2) | _BV(PIND3));
	//Enable set/change button pullups
	PORTD |= (_BV(PIND2) | _BV(PIND3));
	//Enable I2C pullups - probably not really necessary since twi.h does this for you
	PORTC |= (_BV(PINC4) | _BV(PINC5));

	//see setResetDisable(bool) for info
	DDRD &= ~(_BV(PIND5)); //Set PIND5 as input initially
	PORTD |= (_BV(PIND5)); //Set high

	//check for buttons held down at power up
	bSave = BUTTON_STATE;

	//init global time value
	povA = povB = 0;
	povData = &povA;
	setInterrupts();

	Serial.begin(SERIAL_BAUD);
}

/*
Used for software disable of reset on Serial connection.
Setting PIND5 to an output through a 110 ohm resistor 
later will place 5V at low impedence on the reset pin, 
preventing it from reseting when RTS is pulsed on connection.
*/
inline void setResetDisable(bool state)
{
	if(state)
		DDRD |= (_BV(PIND5));
	else
		DDRD &= ~(_BV(PIND5));  
}  

//Setup all things interrupt related
inline void setInterrupts()
{
	//disable interrupt  timers
	cli();
	// Set up interrupt-on-change for buttons.
	EICRA = _BV(ISC10)  | _BV(ISC00);  // Trigger on any logic change
	EIMSK = _BV(INT1)   | _BV(INT0);   // Enable interrupts on pins
	bSave = BUTTON_STATE; // Get initial button state

	//Setup Display Refresh Interrupt
	TCCR1A = 0;// set entire TCCR1A register to 0
	TCCR1B = 0;// same for TCCR1B
	TCNT1  = 0;//initialize counter value to 0

	//set compare match register for 12,800 Hz (1600 Hz screen refresh) increments
	OCR1A = 1250;// = (16*10^6) / (1*12800) - 1 
	// turn on CTC mode
	TCCR1B |= _BV(WGM12);

	TCCR1B |= PRESCALE1_1;  
	// enable timer compare interrupt
	TIMSK1 |= _BV(OCIE1A);

	//Setup Timer2 interrupts for button handling
	//Runs at about 60Hz, which is as slow as we can go
	TCCR2A = 0;// set entire TCCR1A register to 0
	TCCR2B = 0;// same for TCCR1B
	TCNT2  = 0;//initialize counter value to 0

	// set compare match register for max
	OCR2A = 255;
	// turn on CTC mode
	TCCR2B |= _BV(WGM21);

	TCCR2B |= PRESCALE2_1024;  
	// enable timer compare interrupt
	TIMSK2 |= _BV(OCIE2A);

	//enable interrupt timers
	sei();
}

//Timer2 interrupt for handling button presses
ISR(TIMER2_COMPA_vect)
{

	// Check for button 'hold' conditions
	if(bSave != BUTTON_MASK) 
	{ // button(s) held
		if(bCount >= holdMax && !holdFlag) 
		{ //held passed 1 second
			holdFlag = true;
			bCount = 0;
			if(bSave & ~BUTTON_A)
			{

			}
			else if(bSave & ~BUTTON_B)
			{

			}
			else
			{
				if(curState == STATE_NONE)
				{
					Serial.println("Enter Serial!!");
					setResetDisable(true);
					Serial.begin(SERIAL_BAUD);
					_serialScan = 16; //start in middle
					_serialScanDir = false;
					timeOutRef = millis();
					curState = STATE_SERIAL_DATA;
				}
				else if(curState == STATE_SERIAL_DATA)
				{
					Serial.end();
					setResetDisable(false);
					curState = STATE_NONE;
				}
			}
		} 
		else bCount++; // else keep counting...
	} 

}

//Button external interrupts
ISR(INT0_vect) {

	uint8_t state = BUTTON_STATE;
	if(state == BUTTON_MASK) //both are high meaning they've been released
	{
		if(holdFlag)
		{
			holdFlag = false;
			bCount = 0;
		}
		else if(bCount > 3) //past debounce threshold
		{
			if(bSave & ~BUTTON_A)
			{
				Serial.println("Press A!");
			}
			else if(bSave & ~BUTTON_B)
			{
				Serial.println("Press B!");
			}
		}
		bCount = 0;
	}
	else if(state != bSave) {
		bCount = 0; 
	}

	bSave = state;

}

//Use the same handler for both INT0 and INT1 interrupts
ISR(INT1_vect, ISR_ALIASOF(INT0_vect));

/*
Where the magic happens. All the multiplexing is done here.
We start by disabling all the "columns" which actually means turning them to high
since this is common ground. 
If in serial set mode we do a larson scanner instead of show the time.
Otherwise set the states of each of the 4 LEDs in each row and then finally
re-enable the column for this pass through the loop.
It requires 8 passes (1 per column) through this to update the display once.
As this is called at 6400Hz, we update the display fully at 800Hz.
*/
volatile uint8_t col = 0, row = 0;
volatile uint8_t scan_low = 0, scan_high = 0;
#define SCAN_WIDTH 2
ISR(TIMER1_COMPA_vect)
{
	//Turn all columns off (High is off in this case since it's common cathode)
	PORTB |= (_BV(PINB0) | _BV(PINB1) | _BV(PINB2) | _BV(PINB3) | _BV(PINB4) | _BV(PINB5));
	PORTD |= (_BV(PIND6) | _BV(PIND7));


	if(curState == STATE_SERIAL_DATA)
	{
		scan_low = _serialScan >= SCAN_WIDTH ? _serialScan - SCAN_WIDTH : 0;
		scan_high = _serialScan <= (31 - SCAN_WIDTH) ? _serialScan + SCAN_WIDTH : 31;
		byte _scanOffset = 0;
		byte _curStep = 0; 

		//set the 4 rows
		for(row=0; row<4; row++)
		{
			_curStep = (row + (col * 4));

			if(_curStep < _serialScan) _scanOffset = (_serialScan - _curStep);
			else if(_curStep > _serialScan) _scanOffset = (_curStep - _serialScan);
			else _scanOffset = 0;

			if((_curStep >= scan_low && _curStep <= scan_high) &&
				(_serialScanStep < scanLevels[_scanOffset]))
				PORTC |= _BV(row);
			else
				PORTC &= ~_BV(row);
		}

		//Enable the current column
		if(col < 6)
			PORTB &= ~_BV(col);
		else
			PORTD &= ~_BV(col);
	}
	else
	{
		{
			//set the 4 rows
			for(row=0; row<4; row++)
			{
				if((*povData) & (1UL << (row + (col * 4))))
					PORTC |= _BV(row);
				else
					PORTC &= ~_BV(row);
			} 

			//Enable the current column
			if(col < 6)
				PORTB &= ~_BV(col);
			else
				PORTD &= ~_BV(col);
		}
	}

	col++;
	if(col == 8)
	{ 
		col = 0;

		if(curState == STATE_SERIAL_DATA)
		{
			_serialScanStep++;
			if(_serialScanStep == 10) _serialScanStep = 0; 
		}

	}
}

//Helper for time delays without actually pausing execution
bool TimeElapsed(unsigned long ref, unsigned long wait)
{
	unsigned long now = millis();

	if(now < ref || ref == 0) //for the 50 day rollover or first boot
		return true;  

	if((now - ref) > wait)
		return true;
	else
		return false;
}

/*
Get time from serial connection
Time data is just the letter 't' followed by 4 bytes 
representing a unix epoch 32-bit time stamp. 
The time is assumed to already be adjusted for local 
time zone and DST by the sending computer as RTC does not 
store time as UTC.
*/
/*
bool getPCTimeSync()
{
bool result = false;
if(Serial.available() >= SYNC_LEN)
{
byte buf[5];
memset(buf, 0, 5);
Serial.readBytes((char *)buf, 5);
if(buf[0] == SYNC_HEADER)
{
uint32_t t = 0;
memcpy(&t, buf+1, 4);
RTC->adjust(DateTime(t));
Serial.write(42); //Writes out to confirm we got it (sends as a *)
Serial.flush();
result = true;
while(Serial.available()) Serial.read(); //clear the bufer
Serial.end();
setResetDisable(false);
}
}

return result;
}
*/

/*
The main program state machine.
Most of the time this will just be updating the time every 500ms
(1000ms can cause noticable jumps if program execution takes longer than normal).
*/
void loop()
{
	if(curState == STATE_NONE)
	{
		if(TimeElapsed(timeRef, frameDelay))
		{
			timeRef = millis();

			povStep++;
			if(povStep >= imageSize)
				povStep = 0;

			if(povStep % 2)
			{
				povB = pgm_read_dword(&imageData[povStep]);
				povData = &povB;
			}
			else
			{
				povA = pgm_read_dword(&imageData[povStep]);
				povData = &povA;
			}
		}
	}
	else if(curState == STATE_SERIAL_DATA)
	{
		if(TimeElapsed(timeOutRef, 30000))
		{
			Serial.end();
			setResetDisable(false);
			curState = STATE_NONE;
		}
		else
		{
			if(TimeElapsed(timeRef, 40))
			{
				Serial.println("Serial!!");
				timeRef = millis();
				if(_serialScan == 31) _serialScanDir = false;
				else if(_serialScan == 0) _serialScanDir = true;
				_serialScan += (_serialScanDir ? 1 : -1);
			}

			//check for serial data for time sync
			//if(getPCTimeSync())
			//	curState = STATE_NONE;
		}
	}
}


































