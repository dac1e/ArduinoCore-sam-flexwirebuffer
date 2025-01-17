/* ----------------------------------------------------------------------------
 *         SAM Software Package License
 * ----------------------------------------------------------------------------
 * Copyright (c) 2011-2012, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following condition is met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

/** \addtogroup rtc_module Working with RTC
 * The RTC driver provides the interface to configure and use the RTC
 * peripheral.
 *
 * It manages date, time, and alarms.\n
 * This timer is clocked by the 32kHz system clock, and is not impacted by
 * power management settings (PMC). To be accurate, it is better to use an
 * external 32kHz crystal instead of the internal 32kHz RC.\n
 *
 * It uses BCD format, and time can be set in AM/PM or 24h mode through a
 * configuration bit in the mode register.\n
 *
 * To update date or time, the user has to follow these few steps :
 * <ul>
 * <li>Set UPDTIM and/or UPDCAL bit(s) in RTC_CR,</li>
 * <li>Polling or IRQ on the ACKUPD bit of RTC_CR,</li>
 * <li>Clear ACKUPD bit in RTC_SCCR,</li>
 * <li>Update Time and/or Calendar values in RTC_TIMR/RTC_CALR (BCD format),</li>
 * <li>Clear UPDTIM and/or UPDCAL bit in RTC_CR.</li>
 * </ul>
 * An alarm can be set to happen on month, date, hours, minutes or seconds,
 * by setting the proper "Enable" bit of each of these fields in the Time and
 * Calendar registers.
 * This allows a large number of configurations to be available for the user.
 * Alarm occurence can be detected even by polling or interrupt.
 *
 * A check of the validity of the date and time format and values written by the user is automatically done.
 * Errors are reported through the Valid Entry Register.
 *
 * For more accurate information, please look at the RTC section of the
 * Datasheet.
 *
 * Related files :\n
 * \ref rtc.c\n
 * \ref rtc.h.\n
*/
/*@{*/
/*@}*/


/**
 * \file
 *
 * Implementation of Real Time Clock (RTC) controller.
 *
 */

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "chip.h"

#include <stdint.h>
#include <assert.h>

/*----------------------------------------------------------------------------
 *        Definitions
 *----------------------------------------------------------------------------*/

#define RTC_HOUR_BIT_LEN_MASK   0x3F
#define RTC_MIN_BIT_LEN_MASK    0x7F
#define RTC_SEC_BIT_LEN_MASK    0x7F
#define RTC_CENT_BIT_LEN_MASK   0x7F
#define RTC_YEAR_BIT_LEN_MASK   0xFF
#define RTC_MONTH_BIT_LEN_MASK  0x1F
#define RTC_DATE_BIT_LEN_MASK   0x3F
#define RTC_WEEK_BIT_LEN_MASK   0x07

/*----------------------------------------------------------------------------
 *        Internal functions
 *----------------------------------------------------------------------------*/
 
/**
 * \brief calculate the RTC_TIMR bcd format.
 *
 * \param ucHour    Current hour in 24 hour mode.
 * \param ucMinute  Current minute.
 * \param ucSecond  Current second.
 *
 * \return time in 32 bit RTC_TIMR bcd format on success, 0xFFFFFFFF on fail
 */
static uint32_t calculate_dwTime( Rtc* pRtc, uint8_t ucHour, uint8_t ucMinute, uint8_t ucSecond) {
    uint32_t dwAmpm=0 ;
    uint8_t ucHour_bcd ;
    uint8_t ucMin_bcd ;
    uint8_t ucSec_bcd ;

    /* if 12-hour mode, set AMPM bit */
    if ( (pRtc->RTC_MR & RTC_MR_HRMOD) == RTC_MR_HRMOD )
    {
      // RTC is running in 12-hour mode
      if ( ucHour > 12 )
      {
        // PM Time
        dwAmpm |= RTC_TIMR_AMPM ;
        ucHour -= 12 ; // convert PM time to 12-hour mode
      }
      else
      {
        // AM Time
        if(ucHour == 0) // midnight ?
        {
          ucHour = 12; // midnight is 12:00h in 12-hour mode
        }
      }
    }
    ucHour_bcd = (ucHour%10)   | ((ucHour/10)<<4) ;
    ucMin_bcd  = (ucMinute%10) | ((ucMinute/10)<<4) ;
    ucSec_bcd  = (ucSecond%10) | ((ucSecond/10)<<4) ;

    /* value overflow */
    if ( (ucHour_bcd & (uint8_t)(~RTC_HOUR_BIT_LEN_MASK)) |
         (ucMin_bcd & (uint8_t)(~RTC_MIN_BIT_LEN_MASK)) |
         (ucSec_bcd & (uint8_t)(~RTC_SEC_BIT_LEN_MASK)))
    {
        return 0xFFFFFFFF ;
    }
    return dwAmpm | ucSec_bcd | (ucMin_bcd << 8) | (ucHour_bcd<<16) ;
}

/**
 * \brief calculate the RTC_CALR bcd format.
 *
 * \param wYear  Current year.
 * \param ucMonth Current month.
 * \param ucDay   Current day.
 * \param ucWeek  Day number in current week.
 *
 * \return date in 32 bit RTC_CALR bcd format on success, 0xFFFFFFFF on fail
 */
static uint32_t calculate_dwDate( Rtc* pRtc, uint16_t wYear, uint8_t ucMonth, uint8_t ucDay, uint8_t ucWeek ) {
    uint8_t ucCent_bcd ;
    uint8_t ucYear_bcd ;
    uint8_t ucMonth_bcd ;
    uint8_t ucDay_bcd ;
    uint8_t ucWeek_bcd ;

    ucCent_bcd  = ((wYear/100)%10) | ((wYear/1000)<<4);
    ucYear_bcd  = (wYear%10) | (((wYear/10)%10)<<4);
    ucMonth_bcd = ((ucMonth%10) | (ucMonth/10)<<4);
    ucDay_bcd   = ((ucDay%10) | (ucDay/10)<<4);
    ucWeek_bcd  = ((ucWeek%10) | (ucWeek/10)<<4);

    /* value over flow */
    if ( (ucCent_bcd & (uint8_t)(~RTC_CENT_BIT_LEN_MASK)) |
         (ucYear_bcd & (uint8_t)(~RTC_YEAR_BIT_LEN_MASK)) |
         (ucMonth_bcd & (uint8_t)(~RTC_MONTH_BIT_LEN_MASK)) |
         (ucWeek_bcd & (uint8_t)(~RTC_WEEK_BIT_LEN_MASK)) |
         (ucDay_bcd & (uint8_t)(~RTC_DATE_BIT_LEN_MASK))
       )
    {
        return 0xFFFFFFFF ;
    }

    /* return date register value */
    return  (uint32_t)ucCent_bcd |
            (ucYear_bcd << 8) |
            (ucMonth_bcd << 16) |
            (ucWeek_bcd << 21) |
            (ucDay_bcd << 24);
}

/**
 * \brief Convert the RTC_TIMR bcd format to hour, minute and second in 24-hour mode
 *
 * \param pucHour    If not null, current hour is stored in this variable.
 * \param pucMinute  If not null, current minute is stored in this variable.
 * \param pucSecond  If not null, current second is stored in this variable.
 */
static void dwTime2time(Rtc* pRtc,  uint32_t dwTime, uint8_t *pucHour, uint8_t *pucMinute, uint8_t *pucSecond )
{
    /* Hour */
    if ( pucHour )
    {
        *pucHour = ((dwTime & 0x00300000) >> 20) * 10 + ((dwTime & 0x000F0000) >> 16);

        if( (pRtc->RTC_MR & RTC_MR_HRMOD) == RTC_MR_HRMOD ) {
          // RTC is running in 12 hour mode
          if ( (dwTime & RTC_TIMR_AMPM) == RTC_TIMR_AMPM )
          {
            // PM Time
            if(*pucHour < 12)
            {
              *pucHour += 12; // convert PM time to 24 hour mode.
            }
          }
          else
          {
            // AM Time
            if(*pucHour == 12) // midnight ?
            {
              *pucHour = 0; // midnight is 0:00h in 24 hour mode.
            }
          }
        }
    }

    /* Minute */
    if ( pucMinute )
    {
        *pucMinute = ((dwTime & 0x00007000) >> 12) * 10
                   + ((dwTime & 0x00000F00) >> 8);
    }

    /* Second */
    if ( pucSecond )
    {
        *pucSecond = ((dwTime & 0x00000070) >> 4) * 10
                   + (dwTime & 0x0000000F);
    }
}

/**
 * \brief Convert the RTC_CALR bcd format to year, month, day, week.
 *
 * \param pYwear  Current year (optional).
 * \param pucMonth  Current month (optional).
 * \param pucDay  Current day (optional).
 * \param pucWeek  Current day in current week (optional).
 */
extern void dwDate2date( uint32_t dwDate, uint16_t *pwYear, uint8_t *pucMonth, uint8_t *pucDay, uint8_t *pucWeek )
{
    /* Retrieve year */
    if ( pwYear )
    {
        *pwYear = (((dwDate  >> 4) & 0x7) * 1000)
                 + ((dwDate & 0xF) * 100)
                 + (((dwDate >> 12) & 0xF) * 10)
                 + ((dwDate >> 8) & 0xF);
    }

    /* Retrieve month */
    if ( pucMonth )
    {
        *pucMonth = (((dwDate >> 20) & 1) * 10) + ((dwDate >> 16) & 0xF);
    }

    /* Retrieve day */
    if ( pucDay )
    {
        *pucDay = (((dwDate >> 28) & 0x3) * 10) + ((dwDate >> 24) & 0xF);
    }

    /* Retrieve week */
    if ( pucWeek )
    {
        *pucWeek = ((dwDate >> 21) & 0x7);
    }
}

/*----------------------------------------------------------------------------
 *        Exported functions
 *----------------------------------------------------------------------------*/

/**
 * \brief Sets the RTC in either 12 or 24 hour mode.
 *
 * \param mode  Hour mode.
 */
extern void RTC_SetHourMode( Rtc* pRtc, uint32_t dwMode )
{
    assert((dwMode & 0xFFFFFFFE) == 0);

    pRtc->RTC_MR = dwMode ;
}

/**
 * \brief Gets the RTC mode.
 *
 * \return Hour mode.
 */
extern uint32_t RTC_GetHourMode( Rtc* pRtc )
{
    uint32_t dwMode ;

    dwMode = pRtc->RTC_MR;
    dwMode &= 0xFFFFFFFE;

    return dwMode ;
}

/**
 * \brief Enables the selected interrupt sources of the RTC.
 *
 * \param sources  Interrupt sources to enable.
 */
extern void RTC_EnableIt( Rtc* pRtc, uint32_t dwSources )
{
    assert((dwSources & (uint32_t)(~0x1F)) == 0);

    pRtc->RTC_IER = dwSources ;
}

/**
* \brief Disables the selected interrupt sources of the RTC.
*
* \param sources  Interrupt sources to disable.
*/
extern void RTC_DisableIt( Rtc* pRtc, uint32_t dwSources )
{
    assert((dwSources & (uint32_t)(~0x1F)) == 0);

    pRtc->RTC_IDR = dwSources ;
}

/**
 * \brief Sets the current time in the RTC.
 *
 * \note In successive update operations, the user must wait at least one second
 * after resetting the UPDTIM/UPDCAL bit in the RTC_CR before setting these
 * bits again. Please look at the RTC section of the datasheet for detail.
 *
 * \param ucHour    Current hour in 24 hour mode.
 * \param ucMinute  Current minute.
 * \param ucSecond  Current second.
 *
 * \return 0 sucess, 1 fail to set
 */
extern int RTC_SetTime( Rtc* pRtc, uint8_t ucHour, uint8_t ucMinute, uint8_t ucSecond )
{
    const uint32_t dwTime=calculate_dwTime( pRtc, ucHour, ucMinute, ucSecond ) ;
    if( dwTime == 0xFFFFFFFF )
    {
      return 1 ;
    }

    pRtc->RTC_CR |= RTC_CR_UPDTIM ;
    while ((pRtc->RTC_SR & RTC_SR_ACKUPD) != RTC_SR_ACKUPD) ;
    pRtc->RTC_SCCR = RTC_SCCR_ACKCLR ;
    pRtc->RTC_TIMR = dwTime ;
    pRtc->RTC_CR &= (uint32_t)(~RTC_CR_UPDTIM) ;
    pRtc->RTC_SCCR |= RTC_SCCR_SECCLR ;

    return (int)(pRtc->RTC_VER & RTC_VER_NVTIM) ;
}

/**
 * \brief Retrieves the current time in 24 hour mode. Note that
 *        the time must be returned in 24 hour mode, because
 *        the AM/PM information would be lost otherwise.
 *
 * \param pucHour    If not null, current hour is stored in this variable.
 * \param pucMinute  If not null, current minute is stored in this variable.
 * \param pucSecond  If not null, current second is stored in this variable.
 */
extern void RTC_GetTime( Rtc* pRtc, uint8_t *pucHour, uint8_t *pucMinute, uint8_t *pucSecond )
{
    /* Get current RTC time */
    uint32_t dwTime = pRtc->RTC_TIMR ;
    while ( dwTime != pRtc->RTC_TIMR )
    {
        dwTime = pRtc->RTC_TIMR ;
    }
    dwTime2time( dwTime, pucHour, pucMinute, pucSecond ) ;
}

/**
 * \brief Sets a time alarm on the RTC.
 * The match is performed only on the provided variables;
 * Setting all pointers to 0 disables the time alarm.
 *
 * \note In AM/PM mode, the hour value must have bit #7 set for PM, cleared for
 * AM (as expected in the time registers).
 *
 * \param pucHour    If not null, the time alarm will hour-match this value.
 * \param pucMinute  If not null, the time alarm will minute-match this value.
 * \param pucSecond  If not null, the time alarm will second-match this value.
 *
 * \return 0 success, 1 fail to set
 */
extern int RTC_SetTimeAlarm( Rtc* pRtc, const uint8_t *pucHour, const uint8_t *pucMinute, const uint8_t *pucSecond )
{
    uint32_t dwAlarm=0 ;

    /* Hour */
    if ( pucHour )
    {
        dwAlarm |= RTC_TIMALR_HOUREN | ((*pucHour / 10) << 20) | ((*pucHour % 10) << 16);
    }

    /* Minute */
    if ( pucMinute )
    {
        dwAlarm |= RTC_TIMALR_MINEN | ((*pucMinute / 10) << 12) | ((*pucMinute % 10) << 8);
    }

    /* Second */
    if ( pucSecond )
    {
        dwAlarm |= RTC_TIMALR_SECEN | ((*pucSecond / 10) << 4) | (*pucSecond % 10);
    }

    pRtc->RTC_TIMALR = dwAlarm ;

    return (int)(pRtc->RTC_VER & RTC_VER_NVTIMALR) ;
}

/**
 * \brief Retrieves the current year, month and day from the RTC.
 * Month, day and week values are numbered starting at 1.
 *
 * \param pYwear  Current year (optional).
 * \param pucMonth  Current month (optional).
 * \param pucDay  Current day (optional).
 * \param pucWeek  Current day in current week (optional).
 */
extern void RTC_GetDate( Rtc* pRtc, uint16_t *pwYear, uint8_t *pucMonth, uint8_t *pucDay, uint8_t *pucWeek )
{
    uint32_t dwDate ;

    /* Get current date (multiple reads are necessary to insure a stable value) */
    do
    {
        dwDate = pRtc->RTC_CALR ;
    }
    while ( dwDate != pRtc->RTC_CALR ) ;

    dwDate2date( dwDate, pwYear, *pucMonth, *pucDay, *pucWeek ) ;
}

/**
 * \brief Sets the current year, month and day in the RTC.
 * Month, day and week values must be numbered starting from 1.
 *
 * \note In successive update operations, the user must wait at least one second
 * after resetting the UPDTIM/UPDCAL bit in the RTC_CR before setting these
 * bits again. Please look at the RTC section of the datasheet for detail.
 *
 * \param wYear  Current year.
 * \param ucMonth Current month.
 * \param ucDay   Current day.
 * \param ucWeek  Day number in current week.
 *
 * \return 0 success, 1 fail to set
 */
extern int RTC_SetDate( Rtc* pRtc, uint16_t wYear, uint8_t ucMonth, uint8_t ucDay, uint8_t ucWeek )
{
    const uint32_t dwDate = calculate_dwDate(pRtc, wYear, ucMonth, ucDay, ucWeek);
    if ( dwDate == 0xFFFFFFFF )
    {
        return 1 ;
    }

    /* Update calendar register  */
    pRtc->RTC_CR |= RTC_CR_UPDCAL ;
    while ((pRtc->RTC_SR & RTC_SR_ACKUPD) != RTC_SR_ACKUPD) ;

    pRtc->RTC_SCCR = RTC_SCCR_ACKCLR;
    pRtc->RTC_CALR = dwDate ;
    pRtc->RTC_CR &= (uint32_t)(~RTC_CR_UPDCAL) ;
    pRtc->RTC_SCCR |= RTC_SCCR_SECCLR; /* clear SECENV in SCCR */

    return (int)(pRtc->RTC_VER & RTC_VER_NVCAL) ;
}

/**
 * \brief Sets a date alarm in the RTC.
 * The alarm will match only the provided values;
 * Passing a null-pointer disables the corresponding field match.
 *
 * \param pucMonth If not null, the RTC alarm will month-match this value.
 * \param pucDay   If not null, the RTC alarm will day-match this value.
 *
 * \return 0 success, 1 fail to set
 */
extern int RTC_SetDateAlarm( Rtc* pRtc, const uint8_t *pucMonth, const uint8_t *pucDay )
{
    uint32_t dwAlarm ;

    dwAlarm = ((pucMonth) || (pucDay)) ? (0) : (0x01010000);

    /* Compute alarm field value */
    if ( pucMonth )
    {
        dwAlarm |= RTC_CALALR_MTHEN | ((*pucMonth / 10) << 20) | ((*pucMonth % 10) << 16);
    }

    if ( pucDay )
    {
        dwAlarm |= RTC_CALALR_DATEEN | ((*pucDay / 10) << 28) | ((*pucDay % 10) << 24);
    }

    /* Set alarm */
    pRtc->RTC_CALALR = dwAlarm ;

    return (int)(pRtc->RTC_VER & RTC_VER_NVCALALR) ;
}

/**
 * \brief Clear flag bits of status clear command register in the RTC.
 *
 * \param mask Bits mask of cleared events
 */
extern void RTC_ClearSCCR( Rtc* pRtc, uint32_t dwMask )
{
    /* Clear all flag bits in status clear command register */
    dwMask &= RTC_SCCR_ACKCLR | RTC_SCCR_ALRCLR | RTC_SCCR_SECCLR | RTC_SCCR_TIMCLR | RTC_SCCR_CALCLR ;

    pRtc->RTC_SCCR = dwMask ;
}

/**
 * \brief Get flag bits of status register in the RTC.
 *
 * \param mask Bits mask of Status Register
 *
 * \return Status register & mask
 */
extern uint32_t RTC_GetSR( Rtc* pRtc, uint32_t dwMask )
{
    uint32_t dwEvent ;

    dwEvent = pRtc->RTC_SR ;

    return (dwEvent & dwMask) ;
}

/**
 * \brief Retrieves the current time and current date. Note that
 *        the time must be returned in 24 hour mode, because
 *        the AM/PM information would be lost otherwise.
 * Month, day and week values are numbered starting at 1.
 *
 * \param pucHour    If not null, current hour is stored in this variable.
 * \param pucMinute  If not null, current minute is stored in this variable.
 * \param pucSecond  If not null, current second is stored in this variable.
 * \param pwYwear    If not null, current year is stored in this variable.
 * \param pucMonth   If not null, current month is stored in this variable.
 * \param pucDay     If not null, current day is stored in this variable.
 * \param pucWeek    If not null, current week is stored in this variable.
 */
extern void RTC_GetTimeAndDate( Rtc* pRtc,
    uint8_t *pucHour, uint8_t *pucMinute, uint8_t *pucSecond,
    uint16_t *pwYear, uint8_t *pucMonth, uint8_t *pucDay, uint8_t *pucWeek )
{
    uint32_t dwTime;
    uint32_t dwDate;
    // Re-read time and date as long as we get unstable time.
    do
    {
      dwTime = pRtc->RTC_TIMR;
      do
      {
        dwDate = pRtc->RTC_CALR;
      }
      while ( dwDate != pRtc->RTC_CALR );
    }
    while( dwTime != pRtc->RTC_TIMR );

    dwTime2time( dwTime, pucHour, pucMinute, pucSecond ) ;
    dwDate2date( dwDate, pwYear, pucMonth, pucDay, pucWeek ) ;
}

/**
 * \brief Sets the current time and date in the RTC.
 * Month, day and week values must be numbered starting from 1.
 *
 * \note In successive update operations, the user must wait at least one second
 * after resetting the UPDTIM/UPDCAL bit in the RTC_CR before setting these
 * bits again. Please look at the RTC section of the datasheet for detail.
 *
 * \param ucHour    Current hour in 24 hour mode.
 * \param ucMinute  Current minute.
 * \param ucSecond  Current second.
 * \param wYear  Current year.
 * \param ucMonth Current month.
 * \param ucDay   Current day.
 * \param ucWeek  Day number in current week.
 *
 * \return 0 sucess, 1 fail to set
 */
extern int RTC_SetTimeAndDate( Rtc* pRtc,
    uint8_t ucHour, uint8_t ucMinute, uint8_t ucSecond,
    uint16_t wYear, uint8_t ucMonth, uint8_t ucDay, uint8_t ucWeek )
{
  const uint32_t dwTime = calculate_dwTime(pRtc, ucHour, ucMinute, ucSecond) ;
  if ( dwTime == 0xFFFFFFFF )
  {
      return 1 ;
  }

  const uint32_t dwDate = calculate_dwDate(pRtc, wYear, ucMonth, ucDay, ucWeek);
  /* value over flow */
  if ( dwDate == 0xFFFFFFFF)
  {
      return 1 ;
  }

  /* Update calendar and time register together */
  pRtc->RTC_CR |= (RTC_CR_UPDTIM | RTC_CR_UPDCAL);
  while ((pRtc->RTC_SR & RTC_SR_ACKUPD) != RTC_SR_ACKUPD) ;

  pRtc->RTC_SCCR = RTC_SCCR_ACKCLR;
  pRtc->RTC_TIMR = dwTime ;
  pRtc->RTC_CALR = dwDate ;
  pRtc->RTC_CR &= ~((uint32_t)RTC_CR_UPDTIM | (uint32_t)RTC_CR_UPDCAL) ;
  pRtc->RTC_SCCR |= RTC_SCCR_SECCLR; /* clear SECENV in SCCR */

  return (int)(pRtc->RTC_VER & RTC_VER_NVCAL) ;
}
