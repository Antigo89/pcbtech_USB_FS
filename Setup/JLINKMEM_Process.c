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
File    : JLINKMEM_Process.c
Purpose : Data handler for Cortex-M J-Link communication via memory
Literature: [1] ARMv7-M Architecture Reference Manual
                \\fileserver\Techinfo\Company\ARM\ArchitectureV7\DDI0403E_e_armv7m_arm.pdf

Additional information:

  Layout of communication area:

    +----------+  TOS - (TX_SIZE + RX_SIZE + 6)
    |          |
    | RX_BUF   |
    |          |
    +----------+  TOS - (TX_SIZE + 6)
    | RX_CNT   |
    +----------+  TOS - (TX_SIZE + 5)
    | HOST_ACT |
    +----------+  TOS - (TX_SIZE + 4)
    | TX_CNT   |
    +----------+  TOS - (TX_SIZE + 3)
    |          |
    | TX_BUF   |
    |          |
    +----------+  TOS - 3
    | TX_SIZE  |
    +----------+  TOS - 2
    | RX_SIZE  |
    +----------+  TOS - 1
    | PROT_ID  |
    +----------+  TOS

  TOS       Initial top of stack as defined by linker settings (top of CSTACK)
  PROT_ID   Magic number indicating the start of communication area
  RX_SIZE   Size of receiving buffer in bytes
  TX_SIZE   Size of sending buffer in bytes
  TX_BUF    Sending buffer
  TX_CNT    Number of bytes in sending buffer
  HOST_ACT  Set to one by embOSView to indicate it is still active
  RX_CNT    Number of bytes in the receiving buffer
  RX_BUF    Receiving buffer
*/

#include "RTOS.h"
#include "JLINKMEM.h"

//lint -save -e9078 -e9033 -e923, MISRA 2012 Rule 11.4, advisory, MISRA 2012 Rule 10.8, required, MISRA 2012 Rule 11.6, required

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

/*********************************************************************
*
*       Sizes of receiving and sending buffer.
*
*  Note:
*    Before you change any of these values make sure OS_Start()
*    reserves enough bytes for the communication area.
*    OS_Start() reserves per default 32 bytes:
*    RX_BUF_SIZE + TX_BUF_SIZE + 6 bytes (PROT_ID, RX_SIZE, TX_SIZE, TX_CNT,
*    HOST_ACT and RX_CNT).
*/
#define RX_BUF_SIZE  8u
#define TX_BUF_SIZE  18u

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/
#define MAGIC_NUM   0x52u // Identifies the start of our communication area.
#define PROT_ID     (*(volatile OS_U8*)(_BaseAddr - 1u))                  // Id of the protocol. Always set to MAGIC_NUM
#define RX_SIZE     (*(volatile OS_U8*)(_BaseAddr - 2u))                  // Size of receiving buffer in bytes
#define TX_SIZE     (*(volatile OS_U8*)(_BaseAddr - 3u))                  // Size of sending buffer in bytes
//
// Set by embOSView to a non-null value when it connects to target.
// The target sets this to null when it detects a communication timeout.
//
#define TX_CNT      (*(volatile OS_U8*)(_BaseAddr - (TX_BUF_SIZE + 4u)))  // Stores the number of bytes we send to embOSView
#define HOST_ACT    (*(volatile OS_U8*)(_BaseAddr - (TX_BUF_SIZE + 5u)))
#define RX_CNT      (*(volatile OS_U8*)(_BaseAddr - (TX_BUF_SIZE + 6u)))  // Stores the number of bytes the embOSView sent to us
#define TX_TIMEOUT  1000u                                                 // Time to wait for embOSView to fetch the data from sending buffer (in system ticks)
#define VTOR_ADDR   (*(volatile OS_U32*)(0xE000ED08u))                    // Vector table base register

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
//
// Pointer to user callbacks
//
static void   (*_pfOnRx)       (OS_U8 Data);
static OS_U8  (*_pfOnTx)       (void);
static OS_INT (*_pfGetNextChar)(void);

static OS_U32       _BaseAddr;       // Initial stack pointer value from the vector table
static unsigned int _TxIsPending;    // Set when there is a character waiting to be sent
static OS_U8        _TxPendingData;  // Holds tha character waiting to be sent
static unsigned int _IsInited;       // Set when the communication is initialized
static OS_U8*       _pRxBuf;         // Start of receiving buffer
static OS_U8*       _pTxBuf;         // Start of sending buffer
//
// Supervises the connection to embOSView
//
static unsigned int _TxTimeoutTimer;  // Timer for TX timeout
static unsigned int _TxBufLocked;     // Serializes the access to our sending buffer

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _Init()
*
*  Function description
*    Performs initialization of memory locations used for communication.
*/
static void _Init(void) {
  OS_U32 VectorTableBaseAddr;

  //
  // On Cortex-M initial top of stack is stored at address defined by VTOR
  //
  VectorTableBaseAddr  = VTOR_ADDR;                /*lint !e9078 MISRA C:2012 Rule 11.4, advisory */ /*lint !e923 MISRA C:2012 Rule 11.6, required */
  _BaseAddr    = (*(OS_U32*)VectorTableBaseAddr);  /*lint !e9078 MISRA C:2012 Rule 11.4, advisory */ /*lint !e923 MISRA C:2012 Rule 11.6, required */
  HOST_ACT     = 0u;
  RX_SIZE      = RX_BUF_SIZE;
  TX_SIZE      = TX_BUF_SIZE;
  RX_CNT       = 0u;
  TX_CNT       = 0u;
  PROT_ID      = MAGIC_NUM;
  _pTxBuf      = (OS_U8*)(_BaseAddr - (TX_BUF_SIZE + 3u));
  _pRxBuf      = (OS_U8*)(_BaseAddr - (TX_BUF_SIZE + RX_BUF_SIZE + 6u));
  _TxIsPending = 0u;
}

/*********************************************************************
*
*       _LockTxBuf()
*
*  Function description
*    Gains exclusive access to sending buffer.
*
*  Return value
*    1: Sending buffer locked.
*    0: Sending buffer couldn't be locked as already in use.
*/
static unsigned int _LockTxBuf(void) {
  unsigned int Locked;

  Locked = 0u;
  OS_INT_Disable();
  if (_TxBufLocked == 0u) {
    _TxBufLocked = 1u;
    Locked = 1u;
  }
  OS_INT_EnableConditional();
  return Locked;
}

/*********************************************************************
*
*       _UnlockTxBuf()
*
*  Function description
*    Releases the exclusive access to sending buffer.
*/
static void _UnlockTxBuf(void) {
  _TxBufLocked = 0u;
}

/*********************************************************************
*
*       _Receive()
*
*  Function description
*    Performs Command / data read from embOSView
*/
static void _Receive(void) {
  OS_U8        i;
  const OS_U8* pBuf;

  if (RX_CNT > 0u) {  // Data received?
    if (_pfOnRx != NULL) {
      pBuf = _pRxBuf + (RX_BUF_SIZE - RX_CNT);  //lint !e9016 MISRA C:2012 Rule 18.4, advisory
      for (i = 0u; i < RX_CNT; i++) {
        _pfOnRx(*pBuf);
        pBuf++;
      }
    }
    RX_CNT = 0u;
  }
}

/*********************************************************************
*
*       _FillTxBuf()
*
*  Function description
*    Stores bytes in the sending buffer.
*
*  Parameter
*    Data: Data byte to be sent.
*/
static void _FillTxBuf(OS_U8 Data) {
  unsigned int Cnt;
  OS_INT       Byte;
  OS_U8*       pBuf;

  Cnt   = 1u;
  pBuf  = _pTxBuf;
  *pBuf = Data;
  pBuf++;
  if (_pfGetNextChar != NULL) {
    //
    // Get more bytes from the communication state machine
    // until the sending buffer is full.
    //
    for (;;) {
      if (Cnt >= TX_BUF_SIZE) {
        break;
      }
      Byte = _pfGetNextChar();
      if (Byte < 0) {
        break;  //lint !e9011 MISRA C:2012 Rule 15.4, advisory
      }
      *pBuf = (OS_U8)Byte;
      pBuf++;
      Cnt++;
    }
  }
  OS_INT_Disable();
  _TxTimeoutTimer = TX_TIMEOUT;
  TX_CNT = (OS_U8)Cnt;
  OS_INT_EnableConditional();
}

/*********************************************************************
*
*       _DropTxData
*
*  Function description
*    Empties the sending buffer of embOS.
*/
static void _DropTxData(void) {
  if (_pfGetNextChar != NULL) {
    while (_pfGetNextChar() >= 0) {
      ;
    }
  }
}

/*********************************************************************
*
*       _Send()
*
*  Function description
*    Sends data back to embOSView if it is ready to receive data.
*/
static void _Send(void) {
  if (TX_CNT == 0u) {  // Can we send data?
    _TxTimeoutTimer = 0u;
    if (_TxIsPending != 0u) {
      _FillTxBuf(_TxPendingData);
      _TxIsPending = 0u;
    } else {
      if (_pfOnTx != NULL) {
        if (_LockTxBuf() != 0u) {
          (void)_pfOnTx();
          _UnlockTxBuf();
        }
      }
    }
  }
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       JLINKMEM_Process()
*
*  Function description
*    This function should be called more or less regularly to allow
*    memory reads while the application program is running.
*    The more often it is called, the higher the transfer speed.
*/
void JLINKMEM_Process(void) {
  static unsigned int _TxTimeout;  // Set when the embOSView fails to fetch our data
  if (OS_IsRunning() != 0u) {      // No communication until the embOS starts
    if (_IsInited == 0u) {
      _Init();
      _IsInited = 1u;
    }
    if (HOST_ACT != 0u) {          // Do nothing until the embOSView connects to us
      //
      // Handle Timeout timer
      //
      if (_TxTimeoutTimer > 0u) {
        _TxTimeoutTimer--;
        if (_TxTimeoutTimer == 0u) {
          _TxTimeout = 1u;
        }
      }

      if (_TxTimeout != 0u) {
        HOST_ACT     = 0u;
        _TxTimeout   = 0u;
        _TxIsPending = 0u;
        _DropTxData();
        RX_CNT = 0u;               // Drop all bytes form receiving buffer.
      } else {
        _Receive();
        _Send();
      }
    }
  }
}

/*********************************************************************
*
*       JLINKMEM_SendChar()
*
*  Function description
*    Send data to embOSView. This function is non-blocking.
*    If data can not be send it is stored in a buffer
*    and sent later, when the handler is called.
*
*  Parameter
*    Data: Data byte to be sent.
*/
void JLINKMEM_SendChar(OS_U8 Data) {
  if (OS_IsRunning() != 0u) {  // No communication until the embOS starts
    if (_IsInited == 0u) {
      _Init();
      _IsInited = 1u;
    }
    if (HOST_ACT != 0u) {      // Do nothing until embOSView connects to us
      if (TX_CNT == 0u) {
        if (_LockTxBuf() != 0u) {
          _FillTxBuf(Data);
          _UnlockTxBuf();
        } else {
          _TxIsPending   = 1u;
          _TxPendingData = Data;
        }
      } else {
        _TxIsPending   = 1u;
        _TxPendingData = Data;
      }
    } else {
      //
      // embOSView not connected, drop characters
      //
      OS_TASK_EnterRegion();
      OS_COM_ClearTxActive();
      OS_TASK_LeaveRegion();
    }
  } else {
    //
    // embOS not started, drop characters
    //
    OS_TASK_EnterRegion();
    OS_COM_ClearTxActive();
    OS_TASK_LeaveRegion();
  }
}

/*********************************************************************
*
*       JLINKMEM_SetpfOnRx()
*
*  Function description
*    Sets the Rx callback routine
*
*  Parameter
*    pfOnRx: Pointer to Rx callback routine
*/
void JLINKMEM_SetpfOnRx(void (*pfOnRx)(OS_U8 Data)) {
  _pfOnRx = pfOnRx;
}

/*********************************************************************
*
*       JLINKMEM_SetpfOnTx()
*
*  Function description
*    Sets the Tx callback routine
*
*  Parameter
*    pfOnTx: Pointer to Tx callback routine
*/
void JLINKMEM_SetpfOnTx(OS_U8 (*pfOnTx)(void)) {
  _pfOnTx = pfOnTx;
}

/*********************************************************************
*
*       JLINKMEM_SetpfGetNextChar()
*
*  Function description
*    Sets the get next character callback routine
*
*  Parameter
*    pfGetNextChar: Pointer to  get next character callback routine
*/
void JLINKMEM_SetpfGetNextChar(OS_INT (*pfGetNextChar)(void)) {
  _pfGetNextChar = pfGetNextChar;
}

//lint -restore

/*************************** End of file ****************************/
