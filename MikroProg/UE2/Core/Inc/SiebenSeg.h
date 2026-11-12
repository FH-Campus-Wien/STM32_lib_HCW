/**
  ********************************************************************************
  * File Name          : SiebenSeg.h
  * Description        : Library for 7-Seg Display on FH training board system
  ********************************************************************************
  *
  * COPYRIGHT(c) 2017, 2023 FH Campus Wien University of Applied Sciences,
  *              FH-Prof. Dipl.-Ing. Herbert Paulis
  *              Extension to Hex Values by Pirmin Karger, modified by Herbert Paulis
  *
  * Usage: 1) Add SiebenSeg.h and SiebenSeg.c to your project (Core -> Inc and
  *           Core -> Src respectively)
  * 	   2) Include this file 
  *        3) a) If *no* HW-Timer is used for SysTick: 
  *             Modify SysTick IRQ handler as following:
  *             In file stm32f4xx_it.c add:
  *               - 'void HAL_SYSTICK_Hook(void);' in Private function prototypes 
  *                 section after /+ USER CODE BEGIN PFP +/ (around line 50)
  *               - in function SysTick_Handler add 'HAL_SYSTICK_Hook();'
  *                 after /+ USER CODE BEGIN SysTick_IRQn 1 +/ (around line 190)
  *           b) Alternatively, if a HW-Timer is used for SysTick:
  *             STM32 Cube MX wil copy HAL_TIM_PeriodElapsedCallback function to
  *             main.c during code generation. In this function, add to 
  *             USER CODE BEGIN Callback 1 section the following code, where TIMx
  *             has to be replaced by the timer actually used for SysTick:
  *               if (htim->Instance == TIMx)
  *               {
  *         	     HAL_SYSTICK_Hook();
  *               }
  *           These modifications will be kept even after updating your project
  *           with STM32 CubeMX again.  
  *        4) Call 'Reset_7Seg()' in your init code 
  *        5) Call 'Write_7Seg(Value, DecimalPointPosition)' to display Value
  *           in decimal form where Value is between 0 and 9999 or use
  *           'Write_7Seg_Hex(Value, DecimalPointPosition)' to display Value
  *           in hexadecimal form where Value is between 0x0000 and 0xFFFF.
  *           DecimalPointPosition is between 0 and 4 for one decimal point
  *           (0 is leftmost digit, etc., 4 means no decimal point).
  *           Alternatively, multiple decimal points can be set using the
  *           pre-defined constants DP_M_0 to DP_M_3 for DecimalPointPosition:
  *           DP_M_0 | DP_M_1 means decimal points will be set at pos 0 and 1 etc.
  *
  * The following GPIO pins are reserved and must must be configured manually or 
  * with STM32 CubeMX by user to 'GPIO output':
  * PA6, PA7, PA8, PC5, PC9, PC10, PC11, PC12, PD0, PD6, PD7, PE3
  * 
  * Attention: Requires working SysTick 1ms interrupt.
  * =========
  *
  * Important Note: For RTOS projects use SiebenSeg_RTOS.h/.c files!
  * ===============
  * 
  ********************************************************************************
  */

#define DP_M_0  0x8001
#define DP_M_1  0x8002
#define DP_M_2  0x8004
#define DP_M_3  0x8008

void Reset_7Seg(void);
void Write_7Seg(const uint16_t Value, const uint16_t DP_Pos);
void Write_7Seg_Hex(const uint16_t Value, const uint16_t DP_Pos);

void HAL_SYSTICK_Hook(void);

/***** END OF FILE *****/
