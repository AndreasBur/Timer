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
/**     \file       TimerOne.c
 *      \brief      Main file of TimerOne library
 *
 *      \details    Arduino library to use Timer 1
 *                  
 *
 *****************************************************************************************************************************************************/
#define _TIMERONE_SOURCE_

/******************************************************************************************************************************************************
 * INCLUDES
 *****************************************************************************************************************************************************/
#include "TimerOne.h"
#include <util/atomic.h>

/******************************************************************************************************************************************************
 * GLOBAL DATA
 *****************************************************************************************************************************************************/
TimerOne& Timer1 = TimerOne::getInstance();              // pre-instantiate TimerOne


/******************************************************************************************************************************************************
 * C O N S T R U C T O R S
 *****************************************************************************************************************************************************/

/******************************************************************************************************************************************************
  CONSTRUCTOR OF TimerOne
******************************************************************************************************************************************************/
/*! \brief          TimerOne constructor
 *  \details        Instantiation of the TimerOne library
 *    
 *  \return         -
 *****************************************************************************************************************************************************/
TimerOne::TimerOne()
{
	State = TIMERONE_STATE_NONE;
	TimerCompareCallback = NULL;
	ClockSelectBitGroup = TIMERONE_REG_CS_NO_CLOCK;
} /* TimerOne */


/******************************************************************************************************************************************************
  DESTRUCTOR OF TimerOne
******************************************************************************************************************************************************/
TimerOne::~TimerOne()
{

} /* ~TimerOne */


/******************************************************************************************************************************************************
  COPY CONSTRUCTOR OF TimerTwo
******************************************************************************************************************************************************/
TimerOne& TimerOne::getInstance()
{
	static TimerOne SingletonInstance;
	return SingletonInstance;
}


/******************************************************************************************************************************************************
 * P U B L I C   F U N C T I O N S
 *****************************************************************************************************************************************************/

/******************************************************************************************************************************************************
  init()
******************************************************************************************************************************************************/
/*! \brief          initialization of the Timer1 hardware
 *  \details        this functions initializes the Timer1 hardware
 *                  
 *  \param[in]      Microseconds				period of the timer overflow interrupt
 *  \param[in]      sTimerOverflowCallback      Callback function which should be called when timer overflow interrupt occurs
 *  \return         E_OK
 *                  E_NOT_OK
 *  \pre			Timer has to be in NONE STATE
 *****************************************************************************************************************************************************/
stdReturnType TimerOne::init(long Microseconds, TimerIsrCallbackF_void sTimerCompareCallback)
{
	stdReturnType ReturnValue = E_NOT_OK;

	if(TIMERONE_STATE_NONE == State) {
        ReturnValue = E_OK;
		State = TIMERONE_STATE_INIT;
		/* clear control register */
	    TCCR1A = 0;
	    TCCR1B = 0;
	    
	    /* set mode 12: clear timer on compare match (CTC) */
	    writeBit(TCCR1A, WGM10, 0);
	    writeBit(TCCR1A, WGM11, 0);
	    writeBit(TCCR1B, WGM12, 1);
		writeBit(TCCR1B, WGM13, 1);
		
		if(setPeriod(Microseconds) == E_NOT_OK) ReturnValue = E_NOT_OK;
		if(sTimerCompareCallback != NULL) if(attachInterrupt(sTimerCompareCallback) == E_NOT_OK) ReturnValue = E_NOT_OK;

		State = TIMERONE_STATE_READY;
	} 
	return ReturnValue;
} /* init */


/******************************************************************************************************************************************************
  setPeriod()
******************************************************************************************************************************************************/
/*! \brief          set period of Timer1 overflow interrupt
 *  \details        this functions sets the period of the Timer1 overflow interrupt therefore 
 *                  prescaler and timer top value will be calculated
 *  \param[in]      Microseconds				period of the timer overflow interrupt
 *  \return         E_OK
 *                  E_NOT_OK
 *****************************************************************************************************************************************************/
stdReturnType TimerOne::setPeriod(unsigned long Microseconds)
{
	stdReturnType ReturnValue = E_NOT_OK;
	unsigned long TimerCycles;
    
    /* was request out of bounds? */
	if(Microseconds <= ((TIMERONE_RESOLUTION / (F_CPU / 1000000)) * TIMERONE_MAX_PRESCALER)) {
        ReturnValue = E_OK;
        /* calculate timer cycles to reach timer period, the counter runs backwards after TOP, interrupt is at BOTTOM so divide microseconds by 2 */
		TimerCycles = (F_CPU / 1000000) * Microseconds;
        /* calculate timer prescaler */
		if(TimerCycles < TIMERONE_RESOLUTION)              ClockSelectBitGroup = TIMERONE_REG_CS_NO_PRESCALER;
		else if((TimerCycles >>= 3) < TIMERONE_RESOLUTION) ClockSelectBitGroup = TIMERONE_REG_CS_PRESCALE_8;
		else if((TimerCycles >>= 3) < TIMERONE_RESOLUTION) ClockSelectBitGroup = TIMERONE_REG_CS_PRESCALE_64;
		else if((TimerCycles >>= 2) < TIMERONE_RESOLUTION) ClockSelectBitGroup = TIMERONE_REG_CS_PRESCALE_256;
		else if((TimerCycles >>= 2) < TIMERONE_RESOLUTION) ClockSelectBitGroup = TIMERONE_REG_CS_PRESCALE_1024;
		else {
			/* request was out of bounds, set as maximum */
			TimerCycles = TIMERONE_RESOLUTION - 1;
			ClockSelectBitGroup = TIMERONE_REG_CS_PRESCALE_1024;
			ReturnValue = E_NOT_OK;
		}
		/* ICR1 is TOP in mode 12: clear timer on compare match (CTC) */
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { ICR1 = TimerCycles; }

		if(TIMERONE_STATE_RUNNING == State)
		{
			/* reset clock select register, and starts the clock */
			writeBitGroup(TCCR1B, TIMERONE_REG_CS_GM, TIMERONE_REG_CS_GP, ClockSelectBitGroup);						
		}
	}
	return ReturnValue;
} /* setPeriod */


/******************************************************************************************************************************************************
  start()
******************************************************************************************************************************************************/
/*! \brief          start timer
 *  \details        
 *                  
 *  \return         E_OK
 *                  E_NOT_OK
 *  \pre			Timer has to be in READY or STOPPED STATE
 *****************************************************************************************************************************************************/
stdReturnType TimerOne::start()
{
	if(TIMERONE_STATE_READY == State || TIMERONE_STATE_STOPPED == State) {
		/* reset counter value */
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { TCNT1 = 0; }
		/* start counter by setting clock select register */
		writeBitGroup(TCCR1B, TIMERONE_REG_CS_GM, TIMERONE_REG_CS_GP, ClockSelectBitGroup);
		/* set compare interrupt, if callback is set */
		if(TimerCompareCallback != NULL) {
			/* enable timer compare interrupt */
			writeBit(TIMSK1, OCIE1A, 1);
		}
		State = TIMERONE_STATE_RUNNING;
		return E_OK;
	} else {
		return E_NOT_OK;
	}
} /* start */


/******************************************************************************************************************************************************
  stop()
******************************************************************************************************************************************************/
/*! \brief          stop timer
 *  \details        
 *                  
 *  \return         -
 *****************************************************************************************************************************************************/
void TimerOne::stop()
{
	/* stop counter by clearing clock select register */
	writeBitGroup(TCCR1B, TIMERONE_REG_CS_GM, TIMERONE_REG_CS_GP, TIMERONE_REG_CS_NO_CLOCK);
	State = TIMERONE_STATE_STOPPED;
} /* stop */


/******************************************************************************************************************************************************
  resume()
******************************************************************************************************************************************************/
/*! \brief          resume timer
 *  \details        
 *                  
 *  \return         E_OK
 *                  E_NOT_OK
 *  \pre			Timer has to be in STOPPED STATE
 *****************************************************************************************************************************************************/
stdReturnType TimerOne::resume()
{
	if(TIMERONE_STATE_STOPPED == State) {
		/* resume counter by setting clock select register */
		writeBitGroup(TCCR1B, TIMERONE_REG_CS_GM, TIMERONE_REG_CS_GP, ClockSelectBitGroup);
		return E_OK;
	} else {
		return E_NOT_OK;
	}
} /* resume */


/******************************************************************************************************************************************************
  attachInterrupt()
******************************************************************************************************************************************************/
/*! \brief          set timer compare interrupt callback
 *  \details        
 *                  
 *  \param[in]      sTimerOverflowCallback				timer compare callback function
 *  \return         E_OK
 *                  E_NOT_OK
 *****************************************************************************************************************************************************/
stdReturnType TimerOne::attachInterrupt(TimerIsrCallbackF_void sTimerCompareCallback)
{
	if(sTimerCompareCallback != NULL) {
		TimerCompareCallback = sTimerCompareCallback;
		/* enable timer compare interrupt */
		if(State == TIMERONE_STATE_RUNNING) writeBit(TIMSK1, OCIE1A, 1);
		return E_OK;
	} else {
		return E_NOT_OK;
	}
} /* attachInterrupt */


/******************************************************************************************************************************************************
  detachInterrupt()
******************************************************************************************************************************************************/
/*! \brief          clear timer compare interrupt callback
 *  \details        
 *                  
 *  \return         -
 *****************************************************************************************************************************************************/
void TimerOne::detachInterrupt()
{
	/* clears the timer compare interrupt enable bit */
	writeBit(TIMSK1, OCIE1A, 0);
} /* detachInterrupt */


/******************************************************************************************************************************************************
  read()
******************************************************************************************************************************************************/
/*! \brief          read current timer value in microseconds
 *  \details        this function returns the current timer value in microseconds
 *                  
 *  \param[out]     Microseconds		current timer value
 *  \return         E_OK
 *                  E_NOT_OK
 *  \pre			Timer has to be in RUNNING or STOPPED STATE
 *****************************************************************************************************************************************************/
stdReturnType TimerOne::read(unsigned long* Microseconds)
{
	stdReturnType ReturnValue = E_NOT_OK;
	unsigned int CounterValue;
	char PrescaleShiftScale = 0;

	if(TIMERONE_STATE_RUNNING == State || TIMERONE_STATE_STOPPED == State) {
		ReturnValue = E_OK;
        /* save current timer value */
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { CounterValue = TCNT1; }
		switch (ClockSelectBitGroup)
		{
			case TIMERONE_REG_CS_NO_PRESCALER:
				PrescaleShiftScale = 0;
				break;
			case TIMERONE_REG_CS_PRESCALE_8:
				PrescaleShiftScale = 3;
				break;
			case TIMERONE_REG_CS_PRESCALE_64:
				PrescaleShiftScale = 6;
				break;
			case TIMERONE_REG_CS_PRESCALE_256:
				PrescaleShiftScale = 8;
				break;
			case TIMERONE_REG_CS_PRESCALE_1024:
				PrescaleShiftScale = 10;
				break;
			default:
				ReturnValue = E_NOT_OK;
		}
		/* transform counter value to microseconds in an efficient way */
		*Microseconds = ((CounterValue * 1000UL) / (F_CPU / 1000UL)) << PrescaleShiftScale;
	}
	return ReturnValue;
} /* read */


/******************************************************************************************************************************************************
  I S R   F U N C T I O N S
******************************************************************************************************************************************************/
ISR(TIMER1_COMPA_vect)
{
	Timer1.TimerCompareCallback();
}


/******************************************************************************************************************************************************
 *  E N D   O F   F I L E
 *****************************************************************************************************************************************************/