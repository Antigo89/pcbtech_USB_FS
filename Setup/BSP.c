/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2022 SEGGER Microcontroller GmbH                  *
*                                                                    *
*       Internet: segger.com  Support: support_embos@segger.com      *
*                                                                    *
**********************************************************************
*                                                                    *
*       embOS * Real time operating system                           *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product or a real-time            *
*       operating system for in-house use.                           *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       OS version: V5.18.0.0                                        *
*                                                                    *
**********************************************************************

----------------------------------------------------------------------
File    : BSP.c
Purpose : BSP for the Olimex STM32P407 eval board
--------  END-OF-HEADER  ---------------------------------------------
*/

#include "BSP.h"
#include "stm32f4xx.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

/****** SFRs used for LED-Port **************************************/

#define GPIOE_BASE_ADDR           ((unsigned int)0x40021000)

#define GPIOE_MODER               (*(volatile unsigned int*)(GPIOE_BASE_ADDR + 0x00))
#define GPIOE_ODR                 (*(volatile unsigned int*)(GPIOE_BASE_ADDR + 0x14))
#define GPIOE_BSRR                (*(volatile unsigned int*)(GPIOE_BASE_ADDR + 0x18))

#define RCC_BASE_ADDR             ((unsigned int)(0x40023800))
#define RCC_AHB1RSTR              (*(volatile unsigned int*)(RCC_BASE_ADDR + 0x10))
#define RCC_AHBENR                (*(volatile unsigned int*)(RCC_BASE_ADDR + 0x30))

#define RCC_LEDPORT_RSTR          RCC_AHB1RSTR
#define RCC_LEDPORT_ENR           RCC_AHBENR
#define RCC_LEDPORT_BIT           (4)


/****** Assign LEDs to Ports ****************************************/

#define LED_PORT_MODER            GPIOE_MODER
#define LED_PORT_ODR              GPIOE_ODR
#define LED_PORT_BSRR             GPIOE_BSRR

#define LED0_BIT                  (13)
#define LED1_BIT                  (14)
#define KEYS_BIT                  (10)
#define KEYT_BIT                  (11)
#define KEYM_BIT                  (12)

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       BSP_Init()
*/
void BSP_Init(void) {
  
  // Initialize port for LEDs and KEYs (sample application)
  
  RCC_LEDPORT_ENR  &= ~(1uL << RCC_LEDPORT_BIT);
  RCC_LEDPORT_RSTR &= ~(1uL << RCC_LEDPORT_BIT);
  RCC_LEDPORT_ENR  |=  (1uL << RCC_LEDPORT_BIT);
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
  SYSCFG->EXTICR[2] |= SYSCFG_EXTICR3_EXTI10_PE|SYSCFG_EXTICR3_EXTI11_PE;
  SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI12_PE;

  LED_PORT_MODER &= ~(3uL << (LED0_BIT * 2)) | (3uL << (LED1_BIT * 2))| (3uL << (KEYS_BIT * 2))| (3uL << (KEYT_BIT * 2))| (3uL << (KEYM_BIT * 2));   // Reset mode; sets port to input
  LED_PORT_MODER |=  (1uL << (LED0_BIT * 2)) | (1uL << (LED1_BIT * 2));   // Set to output mode
  LED_PORT_BSRR   =  (0x10000uL << LED0_BIT) | (0x10000uL << LED1_BIT);   // Initially clear LEDs

  //Initialize port for Keys
  EXTI->PR |= EXTI_PR_PR10|EXTI_PR_PR11|EXTI_PR_PR12;
  EXTI->FTSR |= EXTI_FTSR_TR10|EXTI_FTSR_TR11|EXTI_FTSR_TR12;
  EXTI->IMR |= EXTI_IMR_MR10|EXTI_IMR_MR11|EXTI_IMR_MR12;
  //Interrupt NVIC Enable
  NVIC_EnableIRQ(EXTI15_10_IRQn);
;
}

/*********************************************************************
*
*       BSP_SetLED()
*/
void BSP_SetLED(int Index) {
  if (Index == 0) {
    LED_PORT_BSRR = (1uL << LED0_BIT);       // Switch on LED0
  } else if (Index == 1) {
    LED_PORT_BSRR = (1uL << LED1_BIT);       // Switch on LED1
  }
}

/*********************************************************************
*
*       BSP_ClrLED()
*/
void BSP_ClrLED(int Index) {
  if (Index == 0) {
    LED_PORT_BSRR = (0x10000uL << LED0_BIT); // Switch off LED0
  } else if (Index == 1) {
    LED_PORT_BSRR = (0x10000uL << LED1_BIT); // Switch off LED1
  }
}

/*********************************************************************
*
*       BSP_ToggleLED()
*/
void BSP_ToggleLED(int Index) {
  if (Index == 0) {
    if ((LED_PORT_ODR & (1uL << LED0_BIT)) == 0) {  // LED is switched off
      LED_PORT_BSRR = (1uL << LED0_BIT);            // Switch on LED0
    } else {
      LED_PORT_BSRR = (0x10000uL << LED0_BIT);      // Switch off LED0
    }
  } else if (Index == 1) {
    if ((LED_PORT_ODR & (1uL << LED1_BIT)) == 0) {  // LED is switched off
      LED_PORT_BSRR = (1uL << LED1_BIT);            // Switch on LED1
    } else {
      LED_PORT_BSRR = (0x10000uL << LED1_BIT);      // Switch off LED1
    }
  }
}

/****** End Of File *************************************************/
