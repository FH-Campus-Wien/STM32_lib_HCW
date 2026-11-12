/**
  **************************************************************************************
  * File Name          : SiebenSeg.c
  * Description        : Library for 7-Seg Display on FH training board system
  **************************************************************************************
  *
  * COPYRIGHT(c) 2017, 2023 FH Campus Wien University of Applied Sciences,
  *              FH-Prof. Dipl.-Ing. Herbert Paulis
  *              Extension to Hex Values by Pirmin Karger, modified by Herbert Paulis
  *
  * See SiebenSeg.h for instructions how to use 
  *
  **************************************************************************************
  */

#include "main.h"
#include "SiebenSeg.h"

#pragma GCC optimize("-O3")

#define DP_ON   GPIO_PIN_SET
#define DP_OFF  GPIO_PIN_RESET
#define LOCK_TIMEOUT  100

typedef struct 
{
  GPIO_TypeDef* Port;
  uint16_t      Pin;
} GPIO;

const GPIO SegCtrl[4] =
{
  {GPIOA, GPIO_PIN_7},		// digit 0 (leftmost digit)
  {GPIOA, GPIO_PIN_6},		// digit 1
  {GPIOC, GPIO_PIN_5},		// digit 2
  {GPIOE, GPIO_PIN_3}		// digit 3
};

const GPIO SegLED[8] =
{
  {GPIOC, GPIO_PIN_12},		//a
  {GPIOD, GPIO_PIN_0 },		//b
  {GPIOA, GPIO_PIN_8 },		//c
  {GPIOC, GPIO_PIN_11},		//d
  {GPIOC, GPIO_PIN_10},		//e
  {GPIOD, GPIO_PIN_6 },		//f
  {GPIOD, GPIO_PIN_7 },		//g
  {GPIOC, GPIO_PIN_9 }		//DP
};

typedef enum {UNLOCKED = 0, LOCKED} WriteLock;
typedef enum {DEC = 0, HEX} DisplayMode;

__IO uint32_t    __ValueToDisplay = 0;
__IO uint16_t    __DP_Position    = 4;
__IO WriteLock   __ChangeDispLock = UNLOCKED;
__IO DisplayMode __DispMode       = DEC;

static void ActivateDigit(const uint16_t DigitNo);
static void Write_7Seg_Dark(const uint16_t DigitNo);
static void Write_7Seg_0(const uint16_t DigitNo, const GPIO_PinState dp);
static void Write_7Seg_1(const uint16_t DigitNo, const GPIO_PinState dp);
static void Write_7Seg_2(const uint16_t DigitNo, const GPIO_PinState dp);
static void Write_7Seg_3(const uint16_t DigitNo, const GPIO_PinState dp);
static void Write_7Seg_4(const uint16_t DigitNo, const GPIO_PinState dp);
static void Write_7Seg_5(const uint16_t DigitNo, const GPIO_PinState dp);
static void Write_7Seg_6(const uint16_t DigitNo, const GPIO_PinState dp);
static void Write_7Seg_7(const uint16_t DigitNo, const GPIO_PinState dp);
static void Write_7Seg_8(const uint16_t DigitNo, const GPIO_PinState dp);
static void Write_7Seg_9(const uint16_t DigitNo, const GPIO_PinState dp);
static void Write_7Seg_A(const uint16_t DigitNo, const GPIO_PinState dp);
static void Write_7Seg_B(const uint16_t DigitNo, const GPIO_PinState dp);
static void Write_7Seg_C(const uint16_t DigitNo, const GPIO_PinState dp);
static void Write_7Seg_D(const uint16_t DigitNo, const GPIO_PinState dp);
static void Write_7Seg_E(const uint16_t DigitNo, const GPIO_PinState dp);
static void Write_7Seg_F(const uint16_t DigitNo, const GPIO_PinState dp);


void HAL_SYSTICK_Hook(void)
{
  static uint16_t ActDigitPos = 0;
  uint16_t ActDigit = 0;
  GPIO_PinState DP_Value;
  uint16_t ValueToDisplay, DP_Position, DispMode;
  uint16_t Timeout = LOCK_TIMEOUT;

  while (__ChangeDispLock == LOCKED)
  { 
    if (--Timeout == 0) return;
  }
    
  __ChangeDispLock = LOCKED;
  ValueToDisplay   = __ValueToDisplay;
  DP_Position      = __DP_Position;
  DispMode         = __DispMode;
  __ChangeDispLock = UNLOCKED;

  ActDigitPos = (ActDigitPos + 1) % 4;

  if (DispMode == DEC)
  {
    switch (ActDigitPos)
    {
      case 0:
        ActDigit = ValueToDisplay / 1000;
        break;
      case 1:
        ActDigit = (ValueToDisplay / 100) % 10;
        break;
      case 2:
        ActDigit = (ValueToDisplay / 10) % 10;
        break;
      case 3:
        ActDigit = ValueToDisplay % 10;
        break;
      default:
        ActDigit = 0;
    }
  }
  else
  {
	switch (ActDigitPos)
	{
	  case 0:
	    ActDigit = ValueToDisplay >> 12;
	    break;
	  case 1:
	    ActDigit = (ValueToDisplay >> 8) & 0xf;
	    break;
	  case 2:
	    ActDigit = (ValueToDisplay >> 4) & 0xf;
	    break;
	  case 3:
	    ActDigit = ValueToDisplay & 0xf;
	    break;
	  default:
	    ActDigit = 0;
	}
  }
  
  if ((DP_Position & 0x8000) == 0)	// single decimal point
  {									// kept for backward compatibility
    if (DP_Position == ActDigitPos)
    {
      DP_Value = DP_ON;
    }
    else
    {
      DP_Value = DP_OFF;
    }
  }
  else	// set for multiple points
  {
    if ((1 << ActDigitPos) & DP_Position)
    {
      DP_Value = DP_ON;
    }
    else
    {
      DP_Value = DP_OFF;
    }
  }
  
  switch (ActDigit)
  {
    case 0:
      Write_7Seg_0(ActDigitPos, DP_Value);
      break;
    case 1:
      Write_7Seg_1(ActDigitPos, DP_Value);
      break;
    case 2:
      Write_7Seg_2(ActDigitPos, DP_Value);
      break;
    case 3:
      Write_7Seg_3(ActDigitPos, DP_Value);
      break;
    case 4:
      Write_7Seg_4(ActDigitPos, DP_Value);
      break;
    case 5:
      Write_7Seg_5(ActDigitPos, DP_Value);
      break;
    case 6:
      Write_7Seg_6(ActDigitPos, DP_Value);
      break;
    case 7:
      Write_7Seg_7(ActDigitPos, DP_Value);
      break;
    case 8:
      Write_7Seg_8(ActDigitPos, DP_Value);
      break;
    case 9:
      Write_7Seg_9(ActDigitPos, DP_Value);
      break;
    case 0xA:
      Write_7Seg_A(ActDigitPos, DP_Value);
      break;
    case 0xB:
      Write_7Seg_B(ActDigitPos, DP_Value);
      break;
    case 0xC:
      Write_7Seg_C(ActDigitPos, DP_Value);
      break;
    case 0xD:
      Write_7Seg_D(ActDigitPos, DP_Value);
      break;
    case 0xE:
      Write_7Seg_E(ActDigitPos, DP_Value);
      break;
    case 0xF:
      Write_7Seg_F(ActDigitPos, DP_Value);
      break;
    default:
      break;
  }
}

void Write_7Seg(const uint16_t Value, const uint16_t DP_Pos)
{
  uint16_t Timeout = LOCK_TIMEOUT;

  while (__ChangeDispLock == LOCKED)
  { 
    if (--Timeout == 0) return;
  }
    
  __ChangeDispLock = LOCKED;
  if (Value > 9999)
  {
    __ValueToDisplay = 9999;
  }
  else
  {
    __ValueToDisplay = Value;
  }
  __DP_Position    = DP_Pos;
  __ChangeDispLock = UNLOCKED;
  __DispMode       = DEC;
}

void Write_7Seg_Hex(const uint16_t Value, const uint16_t DP_Pos)
{
  uint16_t Timeout = LOCK_TIMEOUT;

  while (__ChangeDispLock == LOCKED)
  {
    if (--Timeout == 0) return;
  }

  __ChangeDispLock = LOCKED;
  if (Value > 9999)
  {
    __ValueToDisplay = 9999;
  }
  else
  {
    __ValueToDisplay = Value;
  }
  __DP_Position    = DP_Pos;
  __ChangeDispLock = UNLOCKED;
  __DispMode       = HEX;
}

__INLINE static void ActivateDigit(const uint16_t DigitNo)
{
  switch (DigitNo)
  {
    case 0:   
      HAL_GPIO_WritePin(SegCtrl[1].Port, SegCtrl[1].Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(SegCtrl[2].Port, SegCtrl[2].Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(SegCtrl[3].Port, SegCtrl[3].Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(SegCtrl[0].Port, SegCtrl[0].Pin, GPIO_PIN_RESET);
      break;
    case 1:   
      HAL_GPIO_WritePin(SegCtrl[0].Port, SegCtrl[0].Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(SegCtrl[2].Port, SegCtrl[2].Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(SegCtrl[3].Port, SegCtrl[3].Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(SegCtrl[1].Port, SegCtrl[1].Pin, GPIO_PIN_RESET);
      break;
    case 2:   
      HAL_GPIO_WritePin(SegCtrl[0].Port, SegCtrl[0].Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(SegCtrl[1].Port, SegCtrl[1].Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(SegCtrl[3].Port, SegCtrl[3].Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(SegCtrl[2].Port, SegCtrl[2].Pin, GPIO_PIN_RESET);
      break;
    case 3:   
      HAL_GPIO_WritePin(SegCtrl[0].Port, SegCtrl[0].Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(SegCtrl[1].Port, SegCtrl[1].Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(SegCtrl[2].Port, SegCtrl[2].Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(SegCtrl[3].Port, SegCtrl[3].Pin, GPIO_PIN_RESET);
      break;
    default:	// silence compiler
      break;
  }
}

void Reset_7Seg(void)
{
  Write_7Seg_Dark(0);
  Write_7Seg_Dark(1);
  Write_7Seg_Dark(2);
  Write_7Seg_Dark(3);
}

__INLINE static void Write_7Seg_Dark(const uint16_t DigitNo)
{      
  ActivateDigit(DigitNo);
  HAL_GPIO_WritePin(SegLED[0].Port, SegLED[0].Pin, GPIO_PIN_RESET);    // a
  HAL_GPIO_WritePin(SegLED[1].Port, SegLED[1].Pin, GPIO_PIN_RESET);    // b
  HAL_GPIO_WritePin(SegLED[2].Port, SegLED[2].Pin, GPIO_PIN_RESET);    // c
  HAL_GPIO_WritePin(SegLED[3].Port, SegLED[3].Pin, GPIO_PIN_RESET);    // d
  HAL_GPIO_WritePin(SegLED[4].Port, SegLED[4].Pin, GPIO_PIN_RESET);    // e
  HAL_GPIO_WritePin(SegLED[5].Port, SegLED[5].Pin, GPIO_PIN_RESET);    // f
  HAL_GPIO_WritePin(SegLED[6].Port, SegLED[6].Pin, GPIO_PIN_RESET);    // g
  HAL_GPIO_WritePin(SegLED[7].Port, SegLED[7].Pin, DP_OFF);            // dp
}

__INLINE static void Write_7Seg_0(const uint16_t DigitNo, const GPIO_PinState dp)
{      
  ActivateDigit(DigitNo);
  HAL_GPIO_WritePin(SegLED[0].Port, SegLED[0].Pin, GPIO_PIN_SET);      // a
  HAL_GPIO_WritePin(SegLED[1].Port, SegLED[1].Pin, GPIO_PIN_SET);      // b
  HAL_GPIO_WritePin(SegLED[2].Port, SegLED[2].Pin, GPIO_PIN_SET);      // c
  HAL_GPIO_WritePin(SegLED[3].Port, SegLED[3].Pin, GPIO_PIN_SET);      // d
  HAL_GPIO_WritePin(SegLED[4].Port, SegLED[4].Pin, GPIO_PIN_SET);      // e
  HAL_GPIO_WritePin(SegLED[5].Port, SegLED[5].Pin, GPIO_PIN_SET);      // f
  HAL_GPIO_WritePin(SegLED[6].Port, SegLED[6].Pin, GPIO_PIN_RESET);    // g
  HAL_GPIO_WritePin(SegLED[7].Port, SegLED[7].Pin, dp);                // dp
}

__INLINE static void Write_7Seg_1(const uint16_t DigitNo, const GPIO_PinState dp)
{      
  ActivateDigit(DigitNo);
  HAL_GPIO_WritePin(SegLED[0].Port, SegLED[0].Pin, GPIO_PIN_RESET);    // a
  HAL_GPIO_WritePin(SegLED[1].Port, SegLED[1].Pin, GPIO_PIN_SET);      // b
  HAL_GPIO_WritePin(SegLED[2].Port, SegLED[2].Pin, GPIO_PIN_SET);      // c
  HAL_GPIO_WritePin(SegLED[3].Port, SegLED[3].Pin, GPIO_PIN_RESET);    // d
  HAL_GPIO_WritePin(SegLED[4].Port, SegLED[4].Pin, GPIO_PIN_RESET);    // e
  HAL_GPIO_WritePin(SegLED[5].Port, SegLED[5].Pin, GPIO_PIN_RESET);    // f
  HAL_GPIO_WritePin(SegLED[6].Port, SegLED[6].Pin, GPIO_PIN_RESET);    // g
  HAL_GPIO_WritePin(SegLED[7].Port, SegLED[7].Pin, dp);                // dp
}

__INLINE static void Write_7Seg_2(const uint16_t DigitNo, const GPIO_PinState dp)
{      
  ActivateDigit(DigitNo);
  HAL_GPIO_WritePin(SegLED[0].Port, SegLED[0].Pin, GPIO_PIN_SET);      // a
  HAL_GPIO_WritePin(SegLED[1].Port, SegLED[1].Pin, GPIO_PIN_SET);      // b
  HAL_GPIO_WritePin(SegLED[2].Port, SegLED[2].Pin, GPIO_PIN_RESET);    // c
  HAL_GPIO_WritePin(SegLED[3].Port, SegLED[3].Pin, GPIO_PIN_SET);      // d
  HAL_GPIO_WritePin(SegLED[4].Port, SegLED[4].Pin, GPIO_PIN_SET);      // e
  HAL_GPIO_WritePin(SegLED[5].Port, SegLED[5].Pin, GPIO_PIN_RESET);    // f
  HAL_GPIO_WritePin(SegLED[6].Port, SegLED[6].Pin, GPIO_PIN_SET);      // g
  HAL_GPIO_WritePin(SegLED[7].Port, SegLED[7].Pin, dp);                // dp
}

__INLINE static void Write_7Seg_3(const uint16_t DigitNo, const GPIO_PinState dp)
{      
  ActivateDigit(DigitNo);
  HAL_GPIO_WritePin(SegLED[0].Port, SegLED[0].Pin, GPIO_PIN_SET);      // a
  HAL_GPIO_WritePin(SegLED[1].Port, SegLED[1].Pin, GPIO_PIN_SET);      // b
  HAL_GPIO_WritePin(SegLED[2].Port, SegLED[2].Pin, GPIO_PIN_SET);      // c
  HAL_GPIO_WritePin(SegLED[3].Port, SegLED[3].Pin, GPIO_PIN_SET);      // d
  HAL_GPIO_WritePin(SegLED[4].Port, SegLED[4].Pin, GPIO_PIN_RESET);    // e
  HAL_GPIO_WritePin(SegLED[5].Port, SegLED[5].Pin, GPIO_PIN_RESET);    // f
  HAL_GPIO_WritePin(SegLED[6].Port, SegLED[6].Pin, GPIO_PIN_SET);      // g
  HAL_GPIO_WritePin(SegLED[7].Port, SegLED[7].Pin, dp);                // dp
}

__INLINE static void Write_7Seg_4(const uint16_t DigitNo, const GPIO_PinState dp)
{      
  ActivateDigit(DigitNo);
  HAL_GPIO_WritePin(SegLED[0].Port, SegLED[0].Pin, GPIO_PIN_RESET);    // a
  HAL_GPIO_WritePin(SegLED[1].Port, SegLED[1].Pin, GPIO_PIN_SET);      // b
  HAL_GPIO_WritePin(SegLED[2].Port, SegLED[2].Pin, GPIO_PIN_SET);      // c
  HAL_GPIO_WritePin(SegLED[3].Port, SegLED[3].Pin, GPIO_PIN_RESET);    // d
  HAL_GPIO_WritePin(SegLED[4].Port, SegLED[4].Pin, GPIO_PIN_RESET);    // e
  HAL_GPIO_WritePin(SegLED[5].Port, SegLED[5].Pin, GPIO_PIN_SET);      // f
  HAL_GPIO_WritePin(SegLED[6].Port, SegLED[6].Pin, GPIO_PIN_SET);      // g
  HAL_GPIO_WritePin(SegLED[7].Port, SegLED[7].Pin, dp);                // dp
}

__INLINE static void Write_7Seg_5(const uint16_t DigitNo, const GPIO_PinState dp)
{      
  ActivateDigit(DigitNo);
  HAL_GPIO_WritePin(SegLED[0].Port, SegLED[0].Pin, GPIO_PIN_SET);      // a
  HAL_GPIO_WritePin(SegLED[1].Port, SegLED[1].Pin, GPIO_PIN_RESET);    // b
  HAL_GPIO_WritePin(SegLED[2].Port, SegLED[2].Pin, GPIO_PIN_SET);      // c
  HAL_GPIO_WritePin(SegLED[3].Port, SegLED[3].Pin, GPIO_PIN_SET);      // d
  HAL_GPIO_WritePin(SegLED[4].Port, SegLED[4].Pin, GPIO_PIN_RESET);    // e
  HAL_GPIO_WritePin(SegLED[5].Port, SegLED[5].Pin, GPIO_PIN_SET);      // f
  HAL_GPIO_WritePin(SegLED[6].Port, SegLED[6].Pin, GPIO_PIN_SET);      // g
  HAL_GPIO_WritePin(SegLED[7].Port, SegLED[7].Pin, dp);                // dp
}

__INLINE static void Write_7Seg_6(const uint16_t DigitNo, const GPIO_PinState dp)
{      
  ActivateDigit(DigitNo);
  HAL_GPIO_WritePin(SegLED[0].Port, SegLED[0].Pin, GPIO_PIN_SET);      // a
  HAL_GPIO_WritePin(SegLED[1].Port, SegLED[1].Pin, GPIO_PIN_RESET);    // b
  HAL_GPIO_WritePin(SegLED[2].Port, SegLED[2].Pin, GPIO_PIN_SET);      // c
  HAL_GPIO_WritePin(SegLED[3].Port, SegLED[3].Pin, GPIO_PIN_SET);      // d
  HAL_GPIO_WritePin(SegLED[4].Port, SegLED[4].Pin, GPIO_PIN_SET);      // e
  HAL_GPIO_WritePin(SegLED[5].Port, SegLED[5].Pin, GPIO_PIN_SET);      // f
  HAL_GPIO_WritePin(SegLED[6].Port, SegLED[6].Pin, GPIO_PIN_SET);      // g
  HAL_GPIO_WritePin(SegLED[7].Port, SegLED[7].Pin, dp);                // dp
}

__INLINE static void Write_7Seg_7(const uint16_t DigitNo, const GPIO_PinState dp)
{      
  ActivateDigit(DigitNo);
  HAL_GPIO_WritePin(SegLED[0].Port, SegLED[0].Pin, GPIO_PIN_SET);      // a
  HAL_GPIO_WritePin(SegLED[1].Port, SegLED[1].Pin, GPIO_PIN_SET);      // b
  HAL_GPIO_WritePin(SegLED[2].Port, SegLED[2].Pin, GPIO_PIN_SET);      // c
  HAL_GPIO_WritePin(SegLED[3].Port, SegLED[3].Pin, GPIO_PIN_RESET);    // d
  HAL_GPIO_WritePin(SegLED[4].Port, SegLED[4].Pin, GPIO_PIN_RESET);    // e
  HAL_GPIO_WritePin(SegLED[5].Port, SegLED[5].Pin, GPIO_PIN_RESET);    // f
  HAL_GPIO_WritePin(SegLED[6].Port, SegLED[6].Pin, GPIO_PIN_RESET);    // g
  HAL_GPIO_WritePin(SegLED[7].Port, SegLED[7].Pin, dp);                // dp
}

__INLINE static void Write_7Seg_8(const uint16_t DigitNo, const GPIO_PinState dp)
{      
  ActivateDigit(DigitNo);
  HAL_GPIO_WritePin(SegLED[0].Port, SegLED[0].Pin, GPIO_PIN_SET);      // a
  HAL_GPIO_WritePin(SegLED[1].Port, SegLED[1].Pin, GPIO_PIN_SET);      // b
  HAL_GPIO_WritePin(SegLED[2].Port, SegLED[2].Pin, GPIO_PIN_SET);      // c
  HAL_GPIO_WritePin(SegLED[3].Port, SegLED[3].Pin, GPIO_PIN_SET);      // d
  HAL_GPIO_WritePin(SegLED[4].Port, SegLED[4].Pin, GPIO_PIN_SET);      // e
  HAL_GPIO_WritePin(SegLED[5].Port, SegLED[5].Pin, GPIO_PIN_SET);      // f
  HAL_GPIO_WritePin(SegLED[6].Port, SegLED[6].Pin, GPIO_PIN_SET);      // g
  HAL_GPIO_WritePin(SegLED[7].Port, SegLED[7].Pin, dp);                // dp
}

__INLINE static void Write_7Seg_9(const uint16_t DigitNo, const GPIO_PinState dp)
{      
  ActivateDigit(DigitNo);
  HAL_GPIO_WritePin(SegLED[0].Port, SegLED[0].Pin, GPIO_PIN_SET);      // a
  HAL_GPIO_WritePin(SegLED[1].Port, SegLED[1].Pin, GPIO_PIN_SET);      // b
  HAL_GPIO_WritePin(SegLED[2].Port, SegLED[2].Pin, GPIO_PIN_SET);      // c
  HAL_GPIO_WritePin(SegLED[3].Port, SegLED[3].Pin, GPIO_PIN_SET);      // d
  HAL_GPIO_WritePin(SegLED[4].Port, SegLED[4].Pin, GPIO_PIN_RESET);    // e
  HAL_GPIO_WritePin(SegLED[5].Port, SegLED[5].Pin, GPIO_PIN_SET);      // f
  HAL_GPIO_WritePin(SegLED[6].Port, SegLED[6].Pin, GPIO_PIN_SET);      // g
  HAL_GPIO_WritePin(SegLED[7].Port, SegLED[7].Pin, dp);                // dp
}

__INLINE static void Write_7Seg_A(const uint16_t DigitNo, const GPIO_PinState dp)
{
  ActivateDigit(DigitNo);
  HAL_GPIO_WritePin(SegLED[0].Port, SegLED[0].Pin, GPIO_PIN_SET);      // a
  HAL_GPIO_WritePin(SegLED[1].Port, SegLED[1].Pin, GPIO_PIN_SET);      // b
  HAL_GPIO_WritePin(SegLED[2].Port, SegLED[2].Pin, GPIO_PIN_SET);      // c
  HAL_GPIO_WritePin(SegLED[3].Port, SegLED[3].Pin, GPIO_PIN_RESET);    // d
  HAL_GPIO_WritePin(SegLED[4].Port, SegLED[4].Pin, GPIO_PIN_SET);      // e
  HAL_GPIO_WritePin(SegLED[5].Port, SegLED[5].Pin, GPIO_PIN_SET);      // f
  HAL_GPIO_WritePin(SegLED[6].Port, SegLED[6].Pin, GPIO_PIN_SET);      // g
  HAL_GPIO_WritePin(SegLED[7].Port, SegLED[7].Pin, dp);                // dp
}

__INLINE static void Write_7Seg_B(const uint16_t DigitNo, const GPIO_PinState dp)
{
  ActivateDigit(DigitNo);
  HAL_GPIO_WritePin(SegLED[0].Port, SegLED[0].Pin, GPIO_PIN_RESET);    // a
  HAL_GPIO_WritePin(SegLED[1].Port, SegLED[1].Pin, GPIO_PIN_RESET);    // b
  HAL_GPIO_WritePin(SegLED[2].Port, SegLED[2].Pin, GPIO_PIN_SET);      // c
  HAL_GPIO_WritePin(SegLED[3].Port, SegLED[3].Pin, GPIO_PIN_SET);      // d
  HAL_GPIO_WritePin(SegLED[4].Port, SegLED[4].Pin, GPIO_PIN_SET);      // e
  HAL_GPIO_WritePin(SegLED[5].Port, SegLED[5].Pin, GPIO_PIN_SET);      // f
  HAL_GPIO_WritePin(SegLED[6].Port, SegLED[6].Pin, GPIO_PIN_SET);      // g
  HAL_GPIO_WritePin(SegLED[7].Port, SegLED[7].Pin, dp);                // dp
}

__INLINE static void Write_7Seg_C(const uint16_t DigitNo, const GPIO_PinState dp)
{
  ActivateDigit(DigitNo);
  HAL_GPIO_WritePin(SegLED[0].Port, SegLED[0].Pin, GPIO_PIN_SET);      // a
  HAL_GPIO_WritePin(SegLED[1].Port, SegLED[1].Pin, GPIO_PIN_RESET);    // b
  HAL_GPIO_WritePin(SegLED[2].Port, SegLED[2].Pin, GPIO_PIN_RESET);    // c
  HAL_GPIO_WritePin(SegLED[3].Port, SegLED[3].Pin, GPIO_PIN_SET);      // d
  HAL_GPIO_WritePin(SegLED[4].Port, SegLED[4].Pin, GPIO_PIN_SET);      // e
  HAL_GPIO_WritePin(SegLED[5].Port, SegLED[5].Pin, GPIO_PIN_SET);      // f
  HAL_GPIO_WritePin(SegLED[6].Port, SegLED[6].Pin, GPIO_PIN_RESET);    // g
  HAL_GPIO_WritePin(SegLED[7].Port, SegLED[7].Pin, dp);                // dp
}

__INLINE static void Write_7Seg_D(const uint16_t DigitNo, const GPIO_PinState dp)
{
  ActivateDigit(DigitNo);
  HAL_GPIO_WritePin(SegLED[0].Port, SegLED[0].Pin, GPIO_PIN_RESET);    // a
  HAL_GPIO_WritePin(SegLED[1].Port, SegLED[1].Pin, GPIO_PIN_SET);      // b
  HAL_GPIO_WritePin(SegLED[2].Port, SegLED[2].Pin, GPIO_PIN_SET);      // c
  HAL_GPIO_WritePin(SegLED[3].Port, SegLED[3].Pin, GPIO_PIN_SET);      // d
  HAL_GPIO_WritePin(SegLED[4].Port, SegLED[4].Pin, GPIO_PIN_SET);      // e
  HAL_GPIO_WritePin(SegLED[5].Port, SegLED[5].Pin, GPIO_PIN_RESET);    // f
  HAL_GPIO_WritePin(SegLED[6].Port, SegLED[6].Pin, GPIO_PIN_SET);      // g
  HAL_GPIO_WritePin(SegLED[7].Port, SegLED[7].Pin, dp);                // dp
}

__INLINE static void Write_7Seg_E(const uint16_t DigitNo, const GPIO_PinState dp)
{
  ActivateDigit(DigitNo);
  HAL_GPIO_WritePin(SegLED[0].Port, SegLED[0].Pin, GPIO_PIN_SET);      // a
  HAL_GPIO_WritePin(SegLED[1].Port, SegLED[1].Pin, GPIO_PIN_RESET);    // b
  HAL_GPIO_WritePin(SegLED[2].Port, SegLED[2].Pin, GPIO_PIN_RESET);    // c
  HAL_GPIO_WritePin(SegLED[3].Port, SegLED[3].Pin, GPIO_PIN_SET);      // d
  HAL_GPIO_WritePin(SegLED[4].Port, SegLED[4].Pin, GPIO_PIN_SET);      // e
  HAL_GPIO_WritePin(SegLED[5].Port, SegLED[5].Pin, GPIO_PIN_SET);      // f
  HAL_GPIO_WritePin(SegLED[6].Port, SegLED[6].Pin, GPIO_PIN_SET);      // g
  HAL_GPIO_WritePin(SegLED[7].Port, SegLED[7].Pin, dp);                // dp
}

__INLINE static void Write_7Seg_F(const uint16_t DigitNo, const GPIO_PinState dp)
{
  ActivateDigit(DigitNo);
  HAL_GPIO_WritePin(SegLED[0].Port, SegLED[0].Pin, GPIO_PIN_SET);      // a
  HAL_GPIO_WritePin(SegLED[1].Port, SegLED[1].Pin, GPIO_PIN_RESET);    // b
  HAL_GPIO_WritePin(SegLED[2].Port, SegLED[2].Pin, GPIO_PIN_RESET);    // c
  HAL_GPIO_WritePin(SegLED[3].Port, SegLED[3].Pin, GPIO_PIN_RESET);    // d
  HAL_GPIO_WritePin(SegLED[4].Port, SegLED[4].Pin, GPIO_PIN_SET);      // e
  HAL_GPIO_WritePin(SegLED[5].Port, SegLED[5].Pin, GPIO_PIN_SET);      // f
  HAL_GPIO_WritePin(SegLED[6].Port, SegLED[6].Pin, GPIO_PIN_SET);      // g
  HAL_GPIO_WritePin(SegLED[7].Port, SegLED[7].Pin, dp);                // dp
}

/***** END OF FILE *****/
