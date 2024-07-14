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
File    : OS_ThreadSafe.c
Purpose : Thread safe library functions

Additional information:
  This module enables thread and/or interrupt safety for e.g. malloc().
  Per default it ensures thread and interrupt safety by disabling/restoring
  embOS interrupts. Zero latency interrupts are not affected and protected.
  If you need to call e.g. malloc() also from within a zero latency interrupt
  additional handling needs to be added.
  If you don't call such functions from within embOS interrupts you can use
  thread safety instead. This reduces the interrupt latency because a mutex
  is used instead of disabling embOS interrupts.
*/

#include "RTOS.h"

/*********************************************************************
*
*       Function prototypes
*
**********************************************************************
*/
#ifdef __cplusplus
  extern "C" {
#endif

void __heap_lock      (void) OS_TEXT_SECTION_ATTRIBUTE(__heap_lock);
void __heap_unlock    (void) OS_TEXT_SECTION_ATTRIBUTE(__heap_unlock);
void __printf_lock    (void) OS_TEXT_SECTION_ATTRIBUTE(__printf_lock);
void __printf_unlock  (void) OS_TEXT_SECTION_ATTRIBUTE(__printf_unlock);
void __scanf_lock     (void) OS_TEXT_SECTION_ATTRIBUTE(__scanf_lock);
void __scanf_unlock   (void) OS_TEXT_SECTION_ATTRIBUTE(__scanf_unlock);
void __debug_io_lock  (void) OS_TEXT_SECTION_ATTRIBUTE(__debug_io_lock);
void __debug_io_unlock(void) OS_TEXT_SECTION_ATTRIBUTE(__debug_io_unlock);

#ifdef __cplusplus
  }
#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
//
// When set to 1 thread and interrupt safety is guaranteed.
//
// When set to 0 only thread safety is guaranteed. In this case you
// must not call e.g. heap functions from ISRs, software timers or OS_Idle().
//
#ifndef   OS_INTERRUPT_SAFE
  #define OS_INTERRUPT_SAFE  1
#endif

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/**********************************************************************
*
*       __heap_lock()
*/
void __heap_lock(void) {
#if (OS_INTERRUPT_SAFE == 1)
  OS_InterruptSafe_Lock();
#else
  OS_ThreadSafe_Lock();
#endif
}

/**********************************************************************
*
*       __heap_unlock()
*/
void __heap_unlock(void) {
#if (OS_INTERRUPT_SAFE == 1)
  OS_InterruptSafe_Unlock();
#else
  OS_ThreadSafe_Unlock();
#endif
}

/**********************************************************************
*
*       __printf_lock()
*/
void __printf_lock(void) {
#if (OS_INTERRUPT_SAFE == 1)
  OS_InterruptSafe_Lock();
#else
  OS_ThreadSafe_Lock();
#endif
}

/**********************************************************************
*
*       __printf_unlock()
*/
void __printf_unlock(void) {
#if (OS_INTERRUPT_SAFE == 1)
  OS_InterruptSafe_Unlock();
#else
  OS_ThreadSafe_Unlock();
#endif
}

/**********************************************************************
*
*       __scanf_lock()
*/
void __scanf_lock(void) {
#if (OS_INTERRUPT_SAFE == 1)
  OS_InterruptSafe_Lock();
#else
  OS_ThreadSafe_Lock();
#endif
}

/**********************************************************************
*
*       __scanf_unlock()
*/
void __scanf_unlock(void) {
#if (OS_INTERRUPT_SAFE == 1)
  OS_InterruptSafe_Unlock();
#else
  OS_ThreadSafe_Unlock();
#endif
}

/**********************************************************************
*
*       __debug_io_lock()
*/
void __debug_io_lock(void) {
#if (OS_INTERRUPT_SAFE == 1)
  OS_InterruptSafe_Lock();
#else
  OS_ThreadSafe_Lock();
#endif
}

/**********************************************************************
*
*       __debug_io_unlock()
*/
void __debug_io_unlock(void) {
#if (OS_INTERRUPT_SAFE == 1)
  OS_InterruptSafe_Unlock();
#else
  OS_ThreadSafe_Unlock();
#endif
}

/*************************** End of file ****************************/
