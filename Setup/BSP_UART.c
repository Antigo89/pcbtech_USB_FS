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

-------------------------- END-OF-HEADER -----------------------------
Purpose : UART implementation

Additional information:

  Device : STM32F407
  Board  : Olimex STM32-P407

  Unit | UART   | Board connector
  ===============================
  0    | USART3 | RS232_2

*/

#include "BSP_UART.h"
#include "RTOS.h"        // For OS_INT_Enter()/OS_INT_Leave(). Remove this line and OS_INT_* functions if not using OS.
#include "stm32f4xx.h"   // Device specific header file, contains CMSIS defines.

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define BSP_UART_CLOCK        (SystemCoreClock)
#define BSP_UART_BAUDRATE     (38400)
#define BSP_UART_IRQn         (USART3_IRQn)
#define BSP_UART_IRQHandler   (USART3_IRQHandler)

#define USART_BASE_ADDR       (0x40004800u)
#define USART_SR              (*(volatile unsigned long*)(USART_BASE_ADDR + 0x00u))
#define USART_DR              (*(volatile unsigned long*)(USART_BASE_ADDR + 0x04u))
#define USART_BRR             (*(volatile unsigned long*)(USART_BASE_ADDR + 0x08u))
#define USART_CR1             (*(volatile unsigned long*)(USART_BASE_ADDR + 0x0Cu))
#define USART_CR2             (*(volatile unsigned long*)(USART_BASE_ADDR + 0x10u))

#define RCC_BASE_ADDR         (0x40023800u)
#define RCC_APB1ENR           (*(volatile unsigned long*)(RCC_BASE_ADDR + 0x40u))
#define RCC_AHB1ENR           (*(volatile unsigned long*)(RCC_BASE_ADDR + 0x30u))

#define GPIO_BASE_ADDR        (0x40020C00u)
#define GPIO_MODER            (*(volatile unsigned long*)(GPIO_BASE_ADDR + 0x00u))
#define GPIO_OTYPER           (*(volatile unsigned long*)(GPIO_BASE_ADDR + 0x04u))
#define GPIO_OSPEEDR          (*(volatile unsigned long*)(GPIO_BASE_ADDR + 0x08u))
#define GPIO_PUPDR            (*(volatile unsigned long*)(GPIO_BASE_ADDR + 0x0Cu))
#define GPIO_AF_HIGH          (*(volatile unsigned long*)(GPIO_BASE_ADDR + 0x24u))

#define US_RXRDY              (0x20u)   // RXNE
#define US_TXEMPTY            (0x80u)   // TXE
#define USART_RX_ERROR_FLAGS  (0x0Fu)   // ORE/NE/FE/PE

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static BSP_UART_TX_CB* _pfWriteCB;
static BSP_UART_RX_CB* _pfReadCB;

/*********************************************************************
*
*       Prototypes
*
*  Declare ISR handler here to avoid "no prototype" warning.
*  They are not declared in any CMSIS header.
*
**********************************************************************
*/

#if defined(__cplusplus)
  extern "C" {                // Make sure we have C-declarations in C++ programs.
#endif

void BSP_UART_IRQHandler(void);

#if defined(__cplusplus)
}                             // Make sure we have C-declarations in C++ programs.
#endif

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _SetBaudrate()
*
*  Function description
*    Configures the UART baud rate.
*
*  Parameters
*    Unit    : Unit number (typically zero-based).
*    Baudrate: Baud rate to configure [Hz].
*/
static void _SetBaudrate(unsigned int Unit, unsigned long Baudrate) {
  BSP_UART_USE_PARA(Unit);
  USART_BRR = BSP_UART_CLOCK / Baudrate / 4;
}

/*********************************************************************
*
*       Global functions, IRQ handler
*
**********************************************************************
*/

/*********************************************************************
*
*       BSP_UART_IRQHandler()
*
*  Function description
*    UART Rx & Tx interrupt handler.
*
*  Additional information
*    Needs to inform the OS that we are in interrupt context.
*/
void BSP_UART_IRQHandler(void) {
  unsigned int  Status;
  unsigned char Data;

  OS_EnterNestableInterrupt();
  Status = USART_SR;                        // Examine status register
  //
  // Handle Rx.
  //
  do {
    if (Status & US_RXRDY) {                // Data received?
      Data = USART_DR;
      if (Status & USART_RX_ERROR_FLAGS) {  // Any error ?
      } else if (_pfReadCB) {
        _pfReadCB(0, (unsigned char)Data);
      }
    }
    Status = USART_SR;                      // Examine current status
  } while (Status & US_RXRDY);
  //
  // Handle Tx.
  //
  if ((Status & US_TXEMPTY) && ((USART_CR1 & 0x40uL) != 0)) {
    if (_pfWriteCB) {
      if (_pfWriteCB(0)) {                  // No more characters to send ?
        USART_CR1 &= ~0x40uL;               // Disable further Tx interrupts
      }
    }
  }
  OS_LeaveNestableInterrupt();
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       BSP_UART_Init()
*
*  Function description
*    Initializes the UART peripheral.
*
*  Parameters
*    Unit       : Unit number (typically zero-based).
*    Baudrate   : Baud rate to configure [Hz].
*    NumDataBits: Number of data bits to use.
*    Parity     : One of the following values:
*                   * BSP_UART_PARITY_NONE
*                   * BSP_UART_PARITY_ODD
*                   * BSP_UART_PARITY_EVEN
*    NumStopBits: Number of stop bits to use.
*/
void BSP_UART_Init(unsigned int Unit, unsigned long Baudrate, unsigned char NumDataBits, unsigned char Parity, unsigned char NumStopBits) {
  //
  // Unused parameters. Partially hard coded USART settings.
  //
  BSP_UART_USE_PARA(NumDataBits);
  BSP_UART_USE_PARA(Parity);
  BSP_UART_USE_PARA(NumStopBits);
  //
  // Default baudrate
  //
  if (Baudrate == 0) {
    Baudrate = BSP_UART_BAUDRATE;
  }
  //
  // Setup clocks, GPIO ports and NVIC IRQs.
  //
  RCC_AHB1ENR  |= (1uL <<  3);    // GPIO CLK enable
  RCC_APB1ENR  |= (1uL << 18);    // Enable USART3 clock
                                  // GPIOC set alternate function for USART3
  GPIO_AF_HIGH  = (7uL <<  0)     // - Set pin_8 to AF7
                | (7uL <<  4);    // - Set pin_9 to AF7
                                  // GPIOC alternate function mode
  GPIO_MODER    = (2uL << 16)     // - Pin_8 AF
                | (2uL << 18);    // - Pin_9 AF
                                  // GPIOC speed setting
  GPIO_OSPEEDR  = (2uL << 16)     // - Pin_8 fast speed
                | (2uL << 18);    // - Pin_9 fast speed
  GPIO_OTYPER   = 0;              // Output type: push-pull for pin_10 and pin_11
                                  // Pull-up/pull-down register
  GPIO_PUPDR    = (1uL << 16)     // - Pin_8 pull-up
                | (1uL << 18);    // - Pin_9 pull-up
  //
  // Initialize IRQ.
  //
  NVIC_SetPriority(BSP_UART_IRQn, (1u << __NVIC_PRIO_BITS) - 2u);
  NVIC_EnableIRQ(BSP_UART_IRQn);
  //
  // Initialize USART.
  //
  _SetBaudrate(Unit, Baudrate);   // Set baudrate
  USART_CR1 = (1uL <<  3)         // Transmitter enable
            | (1uL <<  2)         // Receiver enable
            | (1uL <<  5)         // RX interrupt enable
            | (1uL << 13);        // Enable USART
}

/*********************************************************************
*
*       BSP_UART_DeInit()
*
*  Function description
*    De-initializes the UART peripheral.
*
*  Parameters
*    Unit: Unit number (typically zero-based).
*/
void BSP_UART_DeInit(unsigned int Unit) {
  BSP_UART_USE_PARA(Unit);
  NVIC_DisableIRQ(BSP_UART_IRQn);
  USART_CR1 = 0x00000000;
}

/*********************************************************************
*
*       BSP_UART_SetBaudrate()
*
*  Function description
*    Configures/changes the UART baud rate.
*
*  Parameters
*    Unit    : Unit number (typically zero-based).
*    Baudrate: Baud rate to configure [Hz].
*/
void BSP_UART_SetBaudrate(unsigned int Unit, unsigned long Baudrate) {
  _SetBaudrate(Unit, Baudrate);
}

/*********************************************************************
*
*       BSP_UART_SetReadCallback()
*
*  Function description
*    Sets the callback to execute upon an Rx interrupt.
*
*  Parameters
*    Unit: Unit number (typically zero-based).
*    pf  : Callback to execute.
*/
void BSP_UART_SetReadCallback(unsigned Unit, BSP_UART_RX_CB* pf) {
  BSP_UART_USE_PARA(Unit);
  _pfReadCB = pf;
}

/*********************************************************************
*
*       BSP_UART_SetWriteCallback()
*
*  Function description
*    Sets the callback to execute upon a Tx interrupt.
*
*  Parameters
*    Unit: Unit number (typically zero-based).
*    pf  : Callback to execute.
*/
void BSP_UART_SetWriteCallback(unsigned int Unit, BSP_UART_TX_CB* pf) {
  BSP_UART_USE_PARA(Unit);
  _pfWriteCB = pf;
}

/*********************************************************************
*
*       BSP_UART_Write1()
*
*  Function description
*    Sends one byte via UART.
*
*  Parameters
*    Unit: Unit number (typically zero-based).
*    Data: (First) data byte to send.
*
*  Additional information
*    The first byte of a transfer is typically sent from application
*    context. Further bytes of the transfer are then sent from the
*    Tx interrupt handler by also calling this function from interrupt
*    context.
*/
void BSP_UART_Write1(unsigned int Unit, unsigned char Data) {
  BSP_UART_USE_PARA(Unit);
  USART_DR   = Data;  // Send data.
  USART_CR1 |= 0x40;  // Enable Tx interrupt.
}

/*************************** End of file ****************************/
