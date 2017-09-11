/******************************************************************************************************************************************************
 *  COPYRIGHT
 *  ---------------------------------------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  Copyright (c) Andreas Burnickl                                                                                                 All rights reserved.
 *
 *  \endverbatim
 *  ---------------------------------------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------------------------------------*/
/**     \file       TimerOne.h
 *      \brief      Main header file of TimerOne library
 *
 *      \details    Arduino library to use Timer 1
 *                  
 *
 *****************************************************************************************************************************************************/
#ifndef _TIMERONE_H_
#define _TIMERONE_H_

/******************************************************************************************************************************************************
 * INCLUDES
 *****************************************************************************************************************************************************/
#include "Arduino.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <StandardTypes.h>


/******************************************************************************************************************************************************
 *  LOCAL CONSTANT MACROS
 *****************************************************************************************************************************************************/
/* Timer1 is 16 bit */
#define TIMERONE_NUMBER_OF_BITS						16
#define TIMERONE_RESOLUTION							(1UL << TIMERONE_NUMBER_OF_BITS)

#define TIMERONE_REG_CS_GP							0
#define TIMERONE_REG_CS_GM							B111

#define TIMERONE_MAX_PRESCALER						1024

/******************************************************************************************************************************************************
 *  LOCAL FUNCTION MACROS
 *****************************************************************************************************************************************************/


/******************************************************************************************************************************************************
 *  GLOBAL DATA TYPES AND STRUCTURES
 *****************************************************************************************************************************************************/
/* Timer ISR callback function */
typedef void (*TimerIsrCallbackF_void)(void);

/* Type which describes the internal state of the TimerOne */
typedef enum {
	TIMERONE_STATE_NONE,
	TIMERONE_STATE_INIT,
	TIMERONE_STATE_READY,
	TIMERONE_STATE_RUNNING,
	TIMERONE_STATE_STOPPED
} TimerOneStateType;

/* Type which includes the values of the Clock Select Bit Group */
typedef enum {
	TIMERONE_REG_CS_NO_CLOCK,
	TIMERONE_REG_CS_NO_PRESCALER,
	TIMERONE_REG_CS_PRESCALE_8,
	TIMERONE_REG_CS_PRESCALE_64,
	TIMERONE_REG_CS_PRESCALE_256,
	TIMERONE_REG_CS_PRESCALE_1024
} TimerOneClockSelectType;


/******************************************************************************************************************************************************
 *  CLASS  TimerOne
 *****************************************************************************************************************************************************/
class TimerOne
{
  private:
    TimerOne();
    ~TimerOne();
    TimerOne(const TimerOne&);
	TimerOneStateType State;
	TimerOneClockSelectType ClockSelectBitGroup;
	unsigned int PwmPeriod;

  public:
    static TimerOne& getInstance();
	TimerIsrCallbackF_void TimerCompareCallback;
	stdReturnType init(long = 1000, TimerIsrCallbackF_void = NULL);
	stdReturnType setPeriod(unsigned long);
	stdReturnType start();
	void stop();
	stdReturnType resume();
	stdReturnType attachInterrupt(TimerIsrCallbackF_void);
	void detachInterrupt();
	stdReturnType read(unsigned long*);
};

/* TimerOne will be pre-instantiated in TimerOne source file */
extern TimerOne& Timer1;

#endif

/******************************************************************************************************************************************************
 *  E N D   O F   F I L E
 *****************************************************************************************************************************************************/