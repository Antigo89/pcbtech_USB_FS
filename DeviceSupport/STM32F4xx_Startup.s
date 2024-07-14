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

File      : STM32F4xx_Startup.s
Purpose   : Startup and exception handlers for STM32F4xx devices.

Additional information:
  Preprocessor Definitions
    __NO_SYSTEM_INIT
      If defined, 
        SystemInit is not called.
      If not defined,
        SystemInit is called.
        SystemInit is usually supplied by the CMSIS files.
        This file declares a weak implementation as fallback.

    __MEMORY_INIT
      If defined,
        MemoryInit is called after SystemInit.
        void MemoryInit(void) can be implemented to enable external
        memory controllers.

    __VECTORS_IN_RAM
      If defined,
        the vector table will be copied from Flash to RAM,
        and the vector table offset register is adjusted.

    __VTOR_CONFIG
      If defined,
        the vector table offset register is set to point to the
        application's vector table.

    __NO_FPU_ENABLE
      If defined, the FPU is explicitly not enabled,
      even if the compiler could use floating point operations.

    __SOFTFP__
      Defined by the build system.
      If not defined, the FPU is enabled for floating point operations.

*/

        .syntax unified  

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/
/*********************************************************************
*
*       Reset_Handler
*
*  Function description
*    Exception handler for reset.
*    Generic bringup of a Cortex-M system.
*
*  Additional information
*    The stack pointer is expected to be initialized by hardware,
*    i.e. read from vectortable[0].
*    For manual initialization add
*      ldr R0, =__stack_end__
*      mov SP, R0
*/
        .global reset_handler
        .global Reset_Handler
        .equ reset_handler, Reset_Handler
        .section .init.Reset_Handler, "ax"
        .balign 2
        .thumb_func
Reset_Handler:
#ifndef __NO_SYSTEM_INIT
        //
        // Call SystemInit
        //
        bl      SystemInit
#endif
#ifdef __MEMORY_INIT
        //
        // Call MemoryInit
        //
        bl      MemoryInit
#endif
#ifdef __VECTORS_IN_RAM
        //
        // Copy vector table (from Flash) to RAM
        //
        ldr     R0, =__vectors_start__
        ldr     R1, =__vectors_end__
        ldr     R2, =__vectors_ram_start__
1:
        cmp     R0, R1
        beq     2f
        ldr     R3, [R0]
        str     R3, [R2]
        adds    R0, R0, #4
        adds    R2, R2, #4
        b       1b
2:
#endif

#if defined(__VTOR_CONFIG) || defined(__VECTORS_IN_RAM)
        //
        // Configure vector table offset register
        //
        movw    R0, 0xED08       // VTOR_REG
        movt    R0, 0xE000
#ifdef __VECTORS_IN_RAM
        ldr     R1, =_vectors_ram
#else
        ldr     R1, =_vectors
#endif
        str     R1, [R0]
#endif
#if !defined(__SOFTFP__) && !defined(__NO_FPU_ENABLE)
        //
        // Enable CP11 and CP10 with CPACR |= (0xf<<20)
        //
        movw    R0, 0xED88       // CPACR
        movt    R0, 0xE000
        ldr     R1, [R0]
        orrs    R1, R1, #(0xf << 20)
        str     R1, [R0]
#endif
        //
        // Call runtime initialization, which calls main().
        //
        bl      _start
        
        //
        // Weak only declaration of SystemInit enables Linker to replace bl SystemInit with a NOP,
        // when there is no strong definition of SystemInit.
        //
        .weak SystemInit
        //
        // Place SystemCoreClockUpdate in .init_array
        // to be called after runtime initialization
        //
#ifndef __NO_SYSTEM_INIT
        .section .init_array, "aw"
        .balign 4
        .word   SystemCoreClockUpdate
#endif

/*********************************************************************
*
*       HardFault_Handler
*
*  Function description
*    Simple exception handler for HardFault.
*    In case of a HardFault caused by BKPT instruction without 
*    debugger attached, return execution, otherwise stay in loop.
*
*  Additional information
*    The stack pointer is expected to be initialized by hardware,
*    i.e. read from vectortable[0].
*    For manual initialization add
*      ldr R0, =__stack_end__
*      mov SP, R0
*/
        .weak HardFault_Handler
        .section .init.HardFault_Handler, "ax"
        .balign 2
        .thumb_func
HardFault_Handler:
        //
        // Check if HardFault is caused by BKPT instruction
        //
        ldr     R1, =0xE000ED2C        // Load NVIC_HFSR
        ldr     R2, [R1]
        cmp     R2, #0                  // Check NVIC_HFSR[31]

hfLoop:
        bmi     hfLoop                  // Not set? Stay in HardFault Handler.
        //
        // Continue execution after BKPT instruction
        //
#if defined(__thumb__) && !defined(__thumb2__)
        movs    R0, #4
        mov     R1, LR
        tst     R0, R1                  // Check EXC_RETURN in Link register bit 2.
        bne     Uses_PSP
        mrs     R0, MSP                 // Stacking was using MSP.
        b       Pass_StackPtr
Uses_PSP:
        mrs     R0, PSP                 // Stacking was using PSP.
Pass_StackPtr:
#else
        tst     LR, #4                  // Check EXC_RETURN[2] in link register to get the return stack
        ite     eq
        mrseq   R0, MSP                 // Frame stored on MSP
        mrsne   R0, PSP                 // Frame stored on PSP
#endif
        //
        // Reset HardFault Status
        //
#if defined(__thumb__) && !defined(__thumb2__)
        movs    R3, #1
        lsls    R3, R3, #31
        orrs    R2, R3
        str     R2, [R1]
#else
        orr R2, R2, #0x80000000
        str R2, [R1]
#endif
        //
        // Adjust return address
        //
        ldr     R1, [R0, #24]           // Get stored PC from stack
        adds    R1, #2                  // Adjust PC by 2 to skip current BKPT
        str     R1, [R0, #24]           // Write back adjusted PC to stack
        //
        bx      LR                      // Return

/*************************** End of file ****************************/
