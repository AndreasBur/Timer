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
TimerOne Timer1;              // pre-instantiate TimerOne


/******************************************************************************************************************************************************
 * P U B L I C   F U N C T I O N S
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
	TimerOverflowCallback = NULL;
	ClockSelectBitGroup = TIMERONE_REG_CS_NO_CLOCK;
} /* TimerOne */


/******************************************************************************************************************************************************
  DESTRUCTOR OF TimerOne
******************************************************************************************************************************************************/
TimerOne::~TimerOne()
{

} /* ~TimerOne */


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
stdReturnType TimerOne::init(long Microseconds, TimerIsrCallbackF_void sTimerOverflowCallback)
{
	stdReturnType ReturnValue = E_OK;

	if(TIMERONE_STATE_NONE == State) {
		State = TIMERONE_STATE_INIT;
		/* clear control register */
	    TCCR1A = 0;
	    TCCR1B = 0;
	    
	    /* set mode 8: phase and frequency correct pwm */
	    writeBit(TCCR1A, WGM10, 0);
	    writeBit(TCCR1A, WGM11, 0);
	    writeBit(TCCR1B, WGM12, 0);
		writeBit(TCCR1B, WGM13, 1);
		
		if(E_NOT_OK == setPeriod(Microseconds)) ReturnValue = E_NOT_OK;
		if(sTimerOverflowCallback != NULL) if(E_NOT_OK == attachInterrupt(sTimerOverflowCallback)) ReturnValue = E_NOT_OK;

		State = TIMERONE_STATE_READY;
	} else {
		ReturnValue = E_NOT_OK;
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
stdReturnType TimerOne::setPeriod(long Microseconds)
{
	stdReturnType ReturnValue = E_OK;
	/* the counter runs backwards after TOP, interrupt is at BOTTOM so divide microseconds by 2 */
	unsigned long TimerCycles;
    
    if(Microseconds <= ((TIMERONE_RESOLUTION / (F_CPU / 1000000)) * TIMERONE_MAX_PRESCALER * 2) {
        /*  */
        TimerCycles = (F_CPU / 2000000) * Microseconds;
        
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
        /* ICR1 is TOP in phase correct pwm mode */
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { ICR1 = TimerCycles; }

        if(TIMERONE_STATE_RUNNING == State)
        {
            /* reset clock select register, and starts the clock */
            writeBitGroup(TCCR1B, TIMERONE_REG_CS_GM, TIMERONE_REG_CS_GP, ClockSelectBitGroup);						
        }
    } else {
        ReturnValue = E_NOT_OK;
    }
	return ReturnValue;
} /* setPeriod */


/******************************************************************************************************************************************************
  enablePwm()
******************************************************************************************************************************************************/
/*! \brief          enable Pwm on given Pin
 *  \details        this function enables Pwm on given Pin with given duty cycle
 *                  period of timer can also be set
 *  \param[in]      PwmPin					pin where pwm should be enabled
 *  \param[in]      DutyCycle				duty cycle of pwm
 *  \param[in]      Microseconds			period of the timer overflow interrupt
 *  \return         E_OK
 *                  E_NOT_OK
 *  \pre			Timer has to be in READY, RUNNING or STOPPED STATE
 *****************************************************************************************************************************************************/
stdReturnType TimerOne::enablePwm(TimerOnePwmPinType PwmPin, unsigned int DutyCycle, long Microseconds) 
{
	stdReturnType ReturnValue = E_OK;

	if(TIMERONE_STATE_READY == State || TIMERONE_STATE_RUNNING == State || TIMERONE_STATE_STOPPED == State)
	{	/* if optional parameter is set, set period of timer overflow interrupt */
		if(Microseconds > 0) if(E_NOT_OK == setPeriod(Microseconds)) ReturnValue = E_NOT_OK;

		if(TIMERONE_PWM_PIN_9 == PwmPin) {
			pinMode(TIMERONE_PWM_PIN_9, OUTPUT);
			/* activate compare output mode in timer control register */
			writeBit(TCCR1A, COM1A1, 1);
		} else if(TIMERONE_PWM_PIN_10 == PwmPin) {
			pinMode(TIMERONE_PWM_PIN_10, OUTPUT);
			/* activate compare output mode in timer control register */
			writeBit(TCCR1A, COM1B1, 1);
		} else {
			ReturnValue = E_NOT_OK;
		}
		if(E_NOT_OK == setPwmDuty(PwmPin, DutyCycle)) ReturnValue = E_NOT_OK;
	} else {
		ReturnValue = E_NOT_OK;
	}
	return ReturnValue;
} /* enablePwm */


/******************************************************************************************************************************************************
  disablePwm()
******************************************************************************************************************************************************/
/*! \brief          disable Pwm on given Pin
 *  \details        
 *  \param[in]      PwmPin					pin where pwm should be disabled
 *  \return         E_OK
 *                  E_NOT_OK
 *****************************************************************************************************************************************************/
stdReturnType TimerOne::disablePwm(TimerOnePwmPinType PwmPin)
{
	stdReturnType ReturnValue = E_NOT_OK;

	if(TIMERONE_PWM_PIN_9 == PwmPin) {
		/* deactivate compare output mode in timer control register */
		writeBit(TCCR1A, COM1A1, 0);
		ReturnValue = E_OK;
	}
	if(TIMERONE_PWM_PIN_10 == PwmPin) {
		/* deactivate compare output mode in timer control register */
		writeBit(TCCR1B, COM1B1, 0);
		ReturnValue = E_OK;
	}

	return ReturnValue;
} /* disablePwm */


/******************************************************************************************************************************************************
  setPwmDuty()
******************************************************************************************************************************************************/
/*! \brief          set pwm duty cycle on given pin
 *  \details        
 *                  
 *  \param[in]      PwmPin					pin where pwm duty cycle should be set
 *  \param[in]      DutyCycle				duty cycle of pwm
 *  \return         E_OK
 *                  E_NOT_OK
 *  \pre			Timer has to be in READY, RUNNING or STOPPED STATE
 *****************************************************************************************************************************************************/
stdReturnType TimerOne::setPwmDuty(TimerOnePwmPinType PwmPin, unsigned int DutyCycle)
{
	stdReturnType ReturnValue = E_OK;
	unsigned long DutyCycleTrans;

	if(TIMERONE_STATE_READY == State || TIMERONE_STATE_RUNNING == State || TIMERONE_STATE_STOPPED == State) {
		/* duty cycle out of bound? */
		if(DutyCycle <= TIMERONE_RESOLUTION) {	
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				/* use rule of three to calculate duty cycle related to timer top value */
				DutyCycleTrans = (unsigned long) ICR1 * DutyCycle;
				DutyCycleTrans >>= TIMERONE_NUMBER_OF_BITS;
				/* set output compare register value for given pwm pin */
				if(TIMERONE_PWM_PIN_9 == PwmPin) OCR1A = DutyCycleTrans;
				else if(TIMERONE_PWM_PIN_10 == PwmPin) OCR1B = DutyCycleTrans;
				else ReturnValue = E_NOT_OK;
			}
		} else {
			ReturnValue = E_NOT_OK;
		}
	} else {
		ReturnValue = E_NOT_OK;
	}
	return ReturnValue;
} /* setPwmDuty */


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
	unsigned int TCNT1_tmp;

	if(TIMERONE_STATE_READY == State || TIMERONE_STATE_STOPPED == State) {
		/* reset counter value */
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { TCNT1 = 0; }
		/* start counter by setting clock select register */
		writeBitGroup(TCCR1B, TIMERONE_REG_CS_GM, TIMERONE_REG_CS_GP, ClockSelectBitGroup);
		/* set overflow interrupt, if callback is set */
		if(TimerOverflowCallback != NULL) {
			/* wait until timer moved on from zero, otherwise get phantom interrupt */
			do { ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {TCNT1_tmp = TCNT1; }} while (TCNT1_tmp == 0);
			/* enable timer overflow interrupt */
			writeBit(TIMSK1, TOIE1, 1);
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
/*! \brief          set timer overflow interrupt callback
 *  \details        
 *                  
 *  \param[in]      sTimerOverflowCallback				timer overflow callback function
 *  \return         E_OK
 *                  E_NOT_OK
 *****************************************************************************************************************************************************/
stdReturnType TimerOne::attachInterrupt(TimerIsrCallbackF_void sTimerOverflowCallback)
{
	if(sTimerOverflowCallback != NULL) {
		TimerOverflowCallback = sTimerOverflowCallback;
		/* enable timer overflow interrupt */
		if(State == TIMERONE_STATE_RUNNING) writeBit(TIMSK1, TOIE1, 1);
		return E_OK;
	} else {
		return E_NOT_OK;
	}
} /* attachInterrupt */


/******************************************************************************************************************************************************
  detachInterrupt()
******************************************************************************************************************************************************/
/*! \brief          clear timer overflow interrupt callback
 *  \details        
 *                  
 *  \return         -
 *****************************************************************************************************************************************************/
void TimerOne::detachInterrupt()
{
	/* clears the timer overflow interrupt enable bit */
	writeBit(TIMSK1, TOIE1, 0);
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
 *  \pre			Timer has to be in RUNNING STATE
 *****************************************************************************************************************************************************/
stdReturnType TimerOne::read(unsigned long* Microseconds)
{
	stdReturnType ReturnValue = E_OK;
	unsigned long TCNT1_tmp;
	unsigned int CounterValue;
	char PrescaleShiftScale = 0;

	if(TIMERONE_STATE_RUNNING == State) {
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
		/* wait one counter tick, needed to find out counter counting up or down */
		do { ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { TCNT1_tmp = TCNT1; }} while (TCNT1_tmp == CounterValue);
		/* if counter counting down, add top value to current value */
		if(TCNT1_tmp < CounterValue) { ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { CounterValue = (int) (ICR1 - CounterValue) + (int) ICR1; }}
		/* transform counter value to microseconds in an efficient way */
		*Microseconds = ((CounterValue * 1000UL) / (F_CPU / 1000UL)) << PrescaleShiftScale;
	} else {
		ReturnValue = E_NOT_OK;
	}
	return ReturnValue;
} /* read */


/******************************************************************************************************************************************************
  I S R   F U N C T I O N S
******************************************************************************************************************************************************/
ISR(TIMER1_OVF_vect)
{
	Timer1.TimerOverflowCallback();
}


/******************************************************************************************************************************************************
 *  E N D   O F   F I L E
 *****************************************************************************************************************************************************/
 