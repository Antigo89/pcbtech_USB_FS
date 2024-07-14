/*********************************************************************
*               SEGGER MICROCONTROLLER SYSTEME GmbH                  *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*       (C) 2003 - 2007   SEGGER Microcontroller Systeme GmbH        *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       USB device stack for embedded applications                   *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : USB_Config_ST_STM32F429I_DISCO_HS_InFS.c
Purpose : Config file for ST MB1045.
--------  END-OF-HEADER  ---------------------------------------------
*/

#include "USB.h"
#include "BSP_USB.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#define USB_ISR_ID    (67)
#define USB_ISR_PRIO  254

/*********************************************************************
*
*       Defines, sfrs
*
**********************************************************************
*/

//
// RCC
//
#define RCC_BASE_ADDR             ((unsigned int)(0x40023800))
#define RCC_AHB1RSTR              (*(volatile U32 *)(RCC_BASE_ADDR + 0x10))
#define RCC_AHB2RSTR              (*(volatile U32 *)(RCC_BASE_ADDR + 0x14))
#define RCC_AHB1ENR               (*(volatile U32 *)(RCC_BASE_ADDR + 0x30))
#define RCC_AHB2ENR               (*(volatile U32 *)(RCC_BASE_ADDR + 0x34))
#define RCC_AHB1LPENR             (*(volatile U32 *)(RCC_BASE_ADDR + 0x50))
//
// GPIO
//
#define GPIOA_BASE_ADDR           ((unsigned int)0x40020000)
#define GPIOA_MODER               (*(volatile U32 *)(GPIOA_BASE_ADDR + 0x00))
#define GPIOA_OTYPER              (*(volatile U32 *)(GPIOA_BASE_ADDR + 0x04))
#define GPIOA_OSPEEDR             (*(volatile U32 *)(GPIOA_BASE_ADDR + 0x08))
#define GPIOA_PUPDR               (*(volatile U32 *)(GPIOA_BASE_ADDR + 0x0C))
#define GPIOA_IDR                 (*(volatile U32 *)(GPIOA_BASE_ADDR + 0x10))
#define GPIOA_ODR                 (*(volatile U32 *)(GPIOA_BASE_ADDR + 0x14))
#define GPIOA_BSRRL               (*(volatile U16 *)(GPIOA_BASE_ADDR + 0x18))
#define GPIOA_BSRRH               (*(volatile U16 *)(GPIOA_BASE_ADDR + 0x16))
#define GPIOA_LCKR                (*(volatile U32 *)(GPIOA_BASE_ADDR + 0x1C))
#define GPIOA_AFRL                (*(volatile U32 *)(GPIOA_BASE_ADDR + 0x20))
#define GPIOA_AFRH                (*(volatile U32 *)(GPIOA_BASE_ADDR + 0x24))

#define GPIOB_BASE_ADDR           ((unsigned int)0x40020400)
#define GPIOB_MODER               (*(volatile U32 *)(GPIOB_BASE_ADDR + 0x00))
#define GPIOB_OTYPER              (*(volatile U32 *)(GPIOB_BASE_ADDR + 0x04))
#define GPIOB_OSPEEDR             (*(volatile U32 *)(GPIOB_BASE_ADDR + 0x08))
#define GPIOB_PUPDR               (*(volatile U32 *)(GPIOB_BASE_ADDR + 0x0C))
#define GPIOB_IDR                 (*(volatile U32 *)(GPIOB_BASE_ADDR + 0x10))
#define GPIOB_ODR                 (*(volatile U32 *)(GPIOB_BASE_ADDR + 0x14))
#define GPIOB_BSRRL               (*(volatile U16 *)(GPIOB_BASE_ADDR + 0x18))
#define GPIOB_BSRRH               (*(volatile U16 *)(GPIOB_BASE_ADDR + 0x16))
#define GPIOB_LCKR                (*(volatile U32 *)(GPIOB_BASE_ADDR + 0x1C))
#define GPIOB_AFRL                (*(volatile U32 *)(GPIOB_BASE_ADDR + 0x20))
#define GPIOB_AFRH                (*(volatile U32 *)(GPIOB_BASE_ADDR + 0x24))

#define GPIOC_BASE_ADDR           ((unsigned int)0x40020800)
#define GPIOC_MODER               (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x00))
#define GPIOC_OTYPER              (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x04))
#define GPIOC_OSPEEDR             (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x08))
#define GPIOC_PUPDR               (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x0C))
#define GPIOC_IDR                 (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x10))
#define GPIOC_ODR                 (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x14))
#define GPIOC_BSRRL               (*(volatile U16 *)(GPIOC_BASE_ADDR + 0x18))
#define GPIOC_BSRRH               (*(volatile U16 *)(GPIOC_BASE_ADDR + 0x16))
#define GPIOC_LCKR                (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x1C))
#define GPIOC_AFRL                (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x20))
#define GPIOC_AFRH                (*(volatile U32 *)(GPIOC_BASE_ADDR + 0x24))

#define GPIOH_BASE_ADDR           ((unsigned int)0x40021C00)
#define GPIOH_MODER               (*(volatile U32 *)(GPIOH_BASE_ADDR + 0x00))
#define GPIOH_OTYPER              (*(volatile U32 *)(GPIOH_BASE_ADDR + 0x04))
#define GPIOH_OSPEEDR             (*(volatile U32 *)(GPIOH_BASE_ADDR + 0x08))
#define GPIOH_PUPDR               (*(volatile U32 *)(GPIOH_BASE_ADDR + 0x0C))
#define GPIOH_IDR                 (*(volatile U32 *)(GPIOH_BASE_ADDR + 0x10))
#define GPIOH_ODR                 (*(volatile U32 *)(GPIOH_BASE_ADDR + 0x14))
#define GPIOH_BSRRL               (*(volatile U16 *)(GPIOH_BASE_ADDR + 0x18))
#define GPIOH_BSRRH               (*(volatile U16 *)(GPIOH_BASE_ADDR + 0x16))
#define GPIOH_LCKR                (*(volatile U32 *)(GPIOH_BASE_ADDR + 0x1C))
#define GPIOH_AFRL                (*(volatile U32 *)(GPIOH_BASE_ADDR + 0x20))
#define GPIOH_AFRH                (*(volatile U32 *)(GPIOH_BASE_ADDR + 0x24))

#define GPIOI_BASE_ADDR           ((unsigned int)0x40022000)
#define GPIOI_MODER               (*(volatile U32 *)(GPIOI_BASE_ADDR + 0x00))
#define GPIOI_OTYPER              (*(volatile U32 *)(GPIOI_BASE_ADDR + 0x04))
#define GPIOI_OSPEEDR             (*(volatile U32 *)(GPIOI_BASE_ADDR + 0x08))
#define GPIOI_PUPDR               (*(volatile U32 *)(GPIOI_BASE_ADDR + 0x0C))
#define GPIOI_IDR                 (*(volatile U32 *)(GPIOI_BASE_ADDR + 0x10))
#define GPIOI_ODR                 (*(volatile U32 *)(GPIOI_BASE_ADDR + 0x14))
#define GPIOI_BSRRL               (*(volatile U16 *)(GPIOI_BASE_ADDR + 0x18))
#define GPIOI_BSRRH               (*(volatile U16 *)(GPIOI_BASE_ADDR + 0x16))
#define GPIOI_LCKR                (*(volatile U32 *)(GPIOI_BASE_ADDR + 0x1C))
#define GPIOI_AFRL                (*(volatile U32 *)(GPIOI_BASE_ADDR + 0x20))
#define GPIOI_AFRH                (*(volatile U32 *)(GPIOI_BASE_ADDR + 0x24))

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
/*********************************************************************
*
*       _EnableISR
*/
static void _EnableISR(USB_ISR_HANDLER * pfISRHandler) {
  BSP_USB_InstallISR_Ex(USB_ISR_ID, pfISRHandler, USB_ISR_PRIO);
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       Setup which target USB driver shall be used
*/

/*********************************************************************
*
*       USBD_X_Config
*/
void USBD_X_Config(void) {
  volatile unsigned int v;

  RCC_AHB1ENR |= 0
              | (1 <<  0)  // GPIOAEN: IO port B clock enable
              ;

  // PA12,PA11
  GPIOA_MODER    =   (GPIOA_MODER  & ~(0x0FUL << 22)) | (0x0AUL << 22);
  GPIOA_OTYPER  &=  ~(0x03UL << 11);
  GPIOA_OSPEEDR |=   (0x0FUL << 22);
  GPIOA_PUPDR   &=  ~(0x0FUL << 22);
  GPIOA_AFRH     =   (GPIOA_AFRH  & ~(0xFFUL << 12)) | (0xAAUL << 12);

  // Enable clock for OTG_HS.
  RCC_AHB2ENR    |=  (1UL << 7);
  for (v = 0; v < 1000000; v++);

  // Reset OTGFS clock.

  RCC_AHB2RSTR   |=  (1UL << 7);
  for (v = 0; v < 5000000; v++);
  RCC_AHB2RSTR   &= ~(1UL << 7);
  for (v = 0; v < 4000000; v++);

  // Add driver.

  USBD_AddDriver(&USB_Driver_ST_STM32F4xxFS);
  USBD_SetISRMgmFuncs(_EnableISR, USB_OS_IncDI, USB_OS_DecRI);
}

/*************************** End of file ****************************/
