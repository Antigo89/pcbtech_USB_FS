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
File    : embOSPlugin.js
Purpose : Script for thread aware debugging
*/

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

/**** Configurable **************************************************/
var StackCheckLimit = 8192;

/**** ARM Register indices, constant ********************************/
var arm_r0  =  0;
var arm_r1  =  1;
var arm_r2  =  2;
var arm_r3  =  3;
var arm_r4  =  4;
var arm_r5  =  5;
var arm_r6  =  6;
var arm_r7  =  7;
var arm_r8  =  8;
var arm_r9  =  9;
var arm_r10 = 10;
var arm_r11 = 11;
var arm_r12 = 12;
var arm_sp  = 13;
var arm_lr  = 14;
var arm_pc  = 15;
var arm_psr = 16;

//
// embOS preserves an additional [pseudo] register
// (exec location with ARM and a modified LR with Cortex-M)
//
var arm_exec = 15;

/**** RISC-V Register indices, constant ********************************/
var riscv_pc  =  0;
var riscv_ra  =  1;
var riscv_sp  =  2;
var riscv_gp  =  3;
var riscv_tp  =  4;
var riscv_t0  =  5;
var riscv_t1  =  6;
var riscv_t2  =  7;
var riscv_s0  =  8;
var riscv_s1  =  9;
var riscv_a0  = 10;
var riscv_a1  = 11;
var riscv_a2  = 12;
var riscv_a3  = 13;
var riscv_a4  = 14;
var riscv_a5  = 15;
var riscv_a6  = 16;
var riscv_a7  = 17;
var riscv_s2  = 18;
var riscv_s3  = 19;
var riscv_s4  = 20;
var riscv_s5  = 21;
var riscv_s6  = 22;
var riscv_s7  = 23;
var riscv_s8  = 24;
var riscv_s9  = 25;
var riscv_s10 = 26;
var riscv_s11 = 27;
var riscv_t3  = 28;
var riscv_t4  = 29;
var riscv_t5  = 30;
var riscv_t6  = 31;

/**** embOS task states, constant ***********************************/
var TS_READY            = (0x00 << 3);  // ready
var TS_WAIT_EVENT       = (0x01 << 3);  // waiting for task event
var TS_WAIT_MUTEX       = (0x02 << 3);  // waiting for mutexes
var TS_WAIT_ANY         = (0x03 << 3);  // Waiting for any reason
var TS_WAIT_SEMA        = (0x04 << 3);  // Waiting for semaphore
var TS_WAIT_MEMPOOL     = (0x05 << 3);  // Waiting for memory pool since V4.30
var TS_WAIT_MEMPOOL_old = (0x05 << 5);  // Waiting for memory pool pre V4.30
var TS_WAIT_QNE         = (0x06 << 3);  // Waiting for message in queue
var TS_WAIT_MBNF        = (0x07 << 3);  // Waiting for space in mailbox
var TS_WAIT_MBNE        = (0x08 << 3);  // Waiting for message in mailbox
var TS_WAIT_EVENTOBJ    = (0x09 << 3);  // Waiting for event object
var TS_WAIT_QNF         = (0x0A << 3);  // Waiting for space in queue

var TS_MASK_SUSPEND_CNT = (0x03 << 0);  // Mask for task suspension count
var TS_MASK_TIMEOUT     = (0x01 << 2);  // Mask for task timeout period
var TS_MASK_STATE       = (0xF8 << 0);  // Mask for task state

/**** embOS list names, constant ************************************/
var Timers              = "Timers";
var Mailboxes           = "Mailboxes";
var Queues              = "Queues";
var Mutexes             = "Mutexes";
var Semaphores          = "Semaphores";
var MemoryPools         = "Memory Pools";
var EventObjects        = "Event Objects";
var WatchDogs           = "Watchdogs";
var SystemInformation   = "System Information";
var ReadersWriterLock   = "Readers-Writer Lock";

/**** embOS object types  *******************************************/
var CastMEMF            = "(OS_MEMPOOL*)";
var CastMutex           = "(OS_MUTEX*)";
var CastQueue           = "(OS_QUEUE*)";
var CastSema            = "(OS_SEMAPHORE*)";

var OSStatus            = "OS_Global.Status";
var RootEvent           = "OS_Global.pEventRoot";
var RootMailbox         = "OS_Global.pMailboxRoot";
var RootMEMF            = "OS_Global.pMEMFRoot";
var RootMutex           = "OS_Global.pMutexRoot";
var RootObjName         = "OS_Global.pObjNameRoot";
var RootQueue           = "OS_Global.pQueueRoot";
var RootRWLock          = "OS_Global.pRWLockRoot";
var RootSema            = "OS_Global.pSemaRoot";
var RootWatchdog        = "OS_Global.pWDRoot";

var TaskStackBase       = "OS_TASK.pStackBase";

/*** Global vars ****************************************************/
var OS_Global;

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/
/*********************************************************************
*
*       GetObjectName()
*
*  Parameters
*    Address: Object address
*
*  Functions description:
*    Returns an object name
*/
function GetObjectName(Address) {
  var p;
  var pObjID;

  p = Debug.evaluate(RootObjName);
  if (p == undefined || p == 0) {
    return "";
  }
  pObjID = Debug.evaluate("(*(OS_OBJNAME*)" + p + ").pOSObjID");
  while (pObjID != Address) {
    p = Debug.evaluate("(OS_OBJNAME*)(*(OS_OBJNAME*)" + p + ").pNext");
    if (p != undefined && p != 0) {
      pObjID = Debug.evaluate("(*(OS_OBJNAME*)" + p + ").pOSObjID");
    } else {
      return "";
    }
  }
  return Debug.evaluate("(char*)(*(OS_OBJNAME*)" + p + ").sName");
}

/*********************************************************************
*
*       GetTaskName()
*
*  Parameters
*    Address: Task address
*
*  Functions description:
*    Returns an object name
*/
function GetTaskName(Address) {
  var Name;

  Name = Debug.evaluate("(char*)(*(OS_TASK*)" + Address + ").sName")  // Since V5.16.0: OS_TASK.sName
      || Debug.evaluate("(char*)(*(OS_TASK*)" + Address + ").Name");  // Pre V5.16.0: OS_TASK.Name
  if (Name == undefined) {
    Name = "n.a.";  // In OS_LIBMODE_XR, no names are available
  } else if (Name == 0) {
    Name = "";      // In case a NULL pointer or empty string was passed during task creation
  }
  return Name;
}

/*********************************************************************
*
*       GetWaitingTasks()
*
*  Functions description:
*    Used to retrieve a string with a list of tasks that are waiting
*    for the specified object.
*
*  Parameters
*    pObject: Address of the object which the tasks returned are waiting for.
*/
function GetWaitingTasks(pObject) {
  var WaitingTasks = "";
  var TaskIterator;
  var pWaitObject;

  for (TaskIterator = OS_Global.pTask; (TaskIterator != undefined) && (TaskIterator != 0); TaskIterator = Debug.evaluate("(OS_TASK*)(*(OS_TASK*)" + TaskIterator + ").pNext")) {
    pWaitObject = Debug.evaluate("(OS_WAIT_OBJ_STRUCT*)(*(OS_WAIT_LIST*)(*(OS_TASK*)" + TaskIterator + ").pWaitList).pWaitObj");
    if (pWaitObject == pObject) {
      WaitingTasks += "0x" + TaskIterator.toString(16).toUpperCase() + " (" + GetTaskName(TaskIterator) + "), ";
    }
  }
  WaitingTasks = WaitingTasks.substring(0, WaitingTasks.length - 2);
  return WaitingTasks;
}

/*********************************************************************
*
*       UpdateTimers()
*
*  Functions description:
*    Called from the update() function in order to retrieve timer
*    information and update the timers list.
*
*  Parameters
*    Window: Object used to access the plug in window.
*/
function UpdateTimers(Window) {
  var Hook;
  var RemainingTime;
  var Timeout;
  var Period;
  var pTimer;
  var Timer;
  var CallbackName;

  for (pTimer = OS_Global.pTimer; (pTimer != undefined) && (pTimer != 0); pTimer = Debug.evaluate("(*(OS_TIMER*)" + pTimer + ").pNext")) {
    //
    // Read information from timer structure. Only running timers can be listed.
    //
    Timer = Debug.evaluate("*(OS_TIMER_EX*)" + pTimer);
    if (Timer == undefined) {
      //
      // If OS_TIMER_EX can't be evaluated (because there's no debug information for it)
      //
      Timer = Debug.evaluate("*(OS_TIMER*)" + pTimer);
      if (Timer == undefined) {
        //
        // Update information in the plug in window for ERRORS
        //
        Window.add2(Timers,
                    "0x" + pTimer.toString(16).toUpperCase(),
                    "<error>",
                    "<error>",
                    "<error>",
                    "<error>");
        break;  // Leave for loop on error
      } else {
        if (Debug.evaluate("OS_Status") != undefined) {
          Hook        = Timer.Hook;
        } else {
          Hook        = Timer.pfTimerRoutine;
        }
        Timeout       = Timer.Time;
        RemainingTime = Timeout - OS_Global.Time;
        Period        = Timer.Period;
      }
    } else {
      //
      // OS_TIMER_EX is used to retrieve information for extended and normal timers
      //
      if (Debug.evaluate("OS_Status") != undefined) {
        Hook        = Timer.Hook;
      } else {
        Hook        = Timer.pfTimerRoutine;
      }
      Timeout       = Timer.Timer.Time;
      RemainingTime = Timeout - OS_Global.Time;
      Period        = Timer.Timer.Period;
    }
    //
    // If the hook function is _OS_cbTimerEx, read the user callback
    // address from the extended timer structure and get its name.
    //
    CallbackName = Debug.getfunction(Hook);
    if (CallbackName == "_OS_cbTimerEx") {
      //
      // Timer is expected to be an object of OS_TIMER_EX, as it has
      // the extended callback function as hook.
      //
      if (Debug.evaluate("OS_Status") != undefined) {
        Hook       = Timer.pfUser;
      } else {
        Hook       = Timer.pfTimerRoutine;
      }
      CallbackName = Debug.getfunction(Hook);
    }
    //
    // Update information in the plug in window
    //
    Window.add2(Timers,
                "0x" + pTimer.toString(16).toUpperCase(),
                GetObjectName(pTimer),
                "0x" + Hook.toString(16).toUpperCase() + " (" + CallbackName + ")",
                RemainingTime + " (" + Timeout + ")",
                Period);
  }
}

/*********************************************************************
*
*       UpdateMailboxes()
*
*  Functions description:
*    Called from the update() function in order to retrieve mailbox
*    information and update the mailbox list.
*
*  Parameters
*    Window: Object used to access the plug in window.
*/
function UpdateMailboxes(Window) {
  var InUse;
  var Messages;
  var MaxMessages;
  var MessageSize;
  var pBuffer;
  var WaitingTasks;
  var pMailbox;
  var MailBox;

  for (pMailbox = Debug.evaluate(RootMailbox); (pMailbox != undefined) && (pMailbox != 0); pMailbox = Debug.evaluate("(OS_MAILBOX*)(*(OS_MAILBOX*)" + pMailbox + ").pNext")) {
    MailBox = Debug.evaluate("*(OS_MAILBOX*)" + pMailbox);
    if (MailBox == undefined) {
      //
      // Update information in the plug in window for ERRORS
      //
      Window.add2(Mailboxes,
                  "0x" + pMailbox.toString(16).toUpperCase(),
                  "<error>",
                  "<error>",
                  "<error>",
                  "<error>",
                  "<error>",
                  "<error>");
      break;  // Leave for loop on error
    } else {
      //
      // Read information from mailbox structure
      //
      InUse       = MailBox.InUse;
      Messages    = MailBox.nofMsg;
      MaxMessages = MailBox.maxMsg;
      MessageSize = MailBox.sizeofMsg;
      pBuffer     = Debug.evaluate("(unsigned int*)(*(OS_MAILBOX*)" + pMailbox + ").pData");
      //
      // Retrieve waiting tasks
      //
      WaitingTasks = GetWaitingTasks(pMailbox);
      //
      // Update information in the plug in window
      //
      Window.add2(Mailboxes,
                  "0x" + pMailbox.toString(16).toUpperCase(),
                  GetObjectName(pMailbox),
                  Messages + "/" + MaxMessages,
                  MessageSize,
                  "0x" + pBuffer.toString(16).toUpperCase(),
                  WaitingTasks,
                  InUse ? "True" : "False");
    }
  }
}

/*********************************************************************
*
*       UpdateQueues()
*
*  Functions description:
*    Called from the update() function in order to retrieve queue
*    information and update the queues list.
*
*  Parameters
*    Window: Object used to access the plug in window.
*/
function UpdateQueues(Window) {
  var Messages;
  var pBuffer;
  var BufferSize;
  var WaitingTasks;
  var pQueue;
  var Queue;

  for (pQueue = Debug.evaluate(RootQueue); (pQueue != undefined) && (pQueue != 0); pQueue = Debug.evaluate("(*" + CastQueue + pQueue + ").pNext")) {
    Queue = Debug.evaluate("*" + CastQueue + pQueue);
    if (Queue == undefined) {
      //
      // Update information in the plug in window for ERRORS
      //
      Window.add2(Queues,
                  "0x" + pQueue.toString(16).toUpperCase(),
                  "<error>",
                  "<error>",
                  "<error>",
                  "<error>",
                  "<error>");
      break;  // Leave for loop on error
    } else {
      //
      // Read information from queue structure
      //
      Messages   = Queue.MsgCnt;
      pBuffer    = Queue.pData;
      BufferSize = Queue.Size;
      //
      // Retrieve waiting tasks
      //
      WaitingTasks = GetWaitingTasks(pQueue);
      //
      // Update information in the plug in window
      //
      Window.add2(Queues,
                  "0x" + pQueue.toString(16).toUpperCase(),
                  GetObjectName(pQueue),
                  Messages,
                  "0x" + pBuffer.toString(16).toUpperCase(),
                  BufferSize,
                  WaitingTasks);
    }
  }
}

/*********************************************************************
*
*       UpdateMutexes()
*
*  Functions description:
*    Called from the update() function in order to retrieve mutexes
*    information and update the mutexes list.
*
*  Parameters
*    Window: Object used to access the plug in window.
*/
function UpdateMutexes(Window) {
  var Owner;
  var UseCounter;
  var WaitingTasks;
  var pMutex;
  var Mutex;

  for (pMutex = Debug.evaluate(RootMutex); (pMutex != undefined) && (pMutex != 0); pMutex = Debug.evaluate("(*" + CastMutex + pMutex + ").pNext")) {
    Mutex = Debug.evaluate("*" + CastMutex + pMutex);
    if (Mutex == undefined) {
      //
      // Update information in the plug in window for ERRORS
      //
      Window.add2(Mutexes,
                  "0x" + pMutex.toString(16).toUpperCase(),
                  "<error>",
                  "<error>",
                  "<error>",
                  "<error>");
      break;  // Leave for loop on error
    } else {
      //
      // Read information from queue structure
      //
      UseCounter = Mutex.UseCnt;
      if (UseCounter != 0) {
        Owner = Mutex.pTask;
      } else {
        Owner = 0;
      }
      //
      // Retrieve waiting tasks
      //
      WaitingTasks = GetWaitingTasks(pMutex);
      //
      // Update information in the plug in window
      //
      Window.add2(Mutexes,
                  "0x" + pMutex.toString(16).toUpperCase(),
                  GetObjectName(pMutex),
                  Owner ? "0x" + Owner.toString(16).toUpperCase() + " (" + GetTaskName(Owner) + ")" : "",
                  UseCounter,
                  WaitingTasks);
    }
  }
}

/*********************************************************************
*
*       UpdateSemaphores()
*
*  Functions description:
*    Called from the update() function in order to retrieve semaphores
*    information and update the semaphores list.
*
*  Parameters
*    Window: Object used to access the plug in window.
*/
function UpdateSemaphores(Window) {
  var Count;
  var WaitingTasks;
  var pSema;
  var Sema;

  for (pSema = Debug.evaluate(RootSema); (pSema != undefined) && (pSema != 0); pSema = Debug.evaluate("(*" + CastSema + pSema + ").pNext")) {
    Sema = Debug.evaluate("*" + CastSema + pSema);
    if (Sema == undefined) {
      //
      // Update information in the plug in window for ERRORS
      //
      Window.add2(Semaphores,
                  "0x" + pSema.toString(16).toUpperCase(),
                  "<error>",
                  "<error>",
                  "<error>");
      break;  // Leave for loop on error
    } else {
      //
      // Read information from queue structure
      //
      Count = Sema.Cnt;
      //
      // Retrieve waiting tasks
      //
      WaitingTasks = GetWaitingTasks(pSema);
      //
      // Update information in the plug in window
      //
      Window.add2(Semaphores,
                  "0x" + pSema.toString(16).toUpperCase(),
                  GetObjectName(pSema),
                  Count,
                  WaitingTasks);
    }
  }
}

/*********************************************************************
*
*       UpdateMemoryPools()
*
*  Functions description:
*    Called from the update() function in order to retrieve memory
*    pool information and update the memory pools list.
*
*  Parameters
*    Window: Object used to access the plug in window.
*/
function UpdateMemoryPools(Window) {
  var NumFreeBlocks;
  var NumBlocks;
  var BlockSize;
  var MaxUsage;
  var pBuffer;
  var WaitingTasks;
  var pMemPool;
  var MemPool;

  for (pMemPool = Debug.evaluate(RootMEMF); (pMemPool != undefined) && (pMemPool != 0); pMemPool = Debug.evaluate("(*" + CastMEMF + pMemPool + ").pNext")) {
    MemPool = Debug.evaluate("*" + CastMEMF + pMemPool);
    if (MemPool == undefined) {
      //
      // Update information in the plug in window for ERRORS
      //
      Window.add2(MemoryPools,
                  "0x" + pMemPool.toString(16).toUpperCase(),
                  "<error>",
                  "<error>",
                  "<error>",
                  "<error>",
                  "<error>",
                  "<error>");
      break;  // Leave for loop on error
    } else {
      //
      // Read information from queue structure
      //
      NumFreeBlocks = MemPool.NumFreeBlocks;
      NumBlocks     = MemPool.NumBlocks;
      BlockSize     = MemPool.BlockSize;
      MaxUsage      = MemPool.MaxUsed;
      pBuffer       = MemPool.pPool;
      //
      // Retrieve waiting tasks
      //
      WaitingTasks = GetWaitingTasks(pMemPool);
      //
      // Update information in the plug in window
      //
      Window.add2(MemoryPools,
                  "0x" + pMemPool.toString(16).toUpperCase(),
                  GetObjectName(pMemPool),
                  NumFreeBlocks + "/" + NumBlocks,
                  BlockSize,
                  MaxUsage,
                  "0x" + pBuffer.toString(16).toUpperCase(),
                  WaitingTasks);
    }
  }
}

/*********************************************************************
*
*       UpdateEventObjects()
*
*  Functions description:
*    Called from the update() function in order to retrieve event
*    object information and update the event objects list.
*
*  Parameters
*    Window: Object used to access the plug in window.
*/
function UpdateEventObjects(Window) {
  var Signaled;
  var ResetMode;
  var MaskMode;
  var WaitingTasks;
  var pEvent;

  for (pEvent = Debug.evaluate(RootEvent); (pEvent != undefined) && (pEvent != 0); pEvent = Debug.evaluate("(*(OS_EVENT*)" + pEvent + ").pNext")) {
    //
    // Read information from queue structure
    //
    Signaled  = Debug.evaluate("               (*(OS_EVENT*)" + pEvent + ").Signaled");
    ResetMode = Debug.evaluate("(unsigned char)(*(OS_EVENT*)" + pEvent + ").ResetMode");
    MaskMode  = Debug.evaluate("(unsigned char)(*(OS_EVENT*)" + pEvent + ").MaskMode");
    if (Signaled == undefined || ResetMode == undefined || MaskMode == undefined) {
      //
      // Update information in the plug in window for ERRORS
      //
      Window.add2(EventObjects,
                  "0x" + pEvent.toString(16).toUpperCase(),
                  "<error>",
                  "<error>",
                  "<error>",
                  "<error>",
                  "<error>");
      break;  // Leave for loop on error
    } else {
      //
      // set sMaskMode
      //
      switch (MaskMode) {
      case 0:
        sMaskMode = "OR Logic";
        break;
      case 4:
        sMaskMode = "AND Logic";
        break;
      default:
        sMaskMode = "0x" + MaskMode.toString(16).toUpperCase();
        break;
      }
      //
      // set sResetMode
      //
      switch (ResetMode) {
      case 0:
        sResetMode = "Semiauto";
        break;
      case 1:
        sResetMode = "Manual";
        break;
      case 2:
        sResetMode = "Auto";
        break;
      default:
        sResetMode = "0x" + ResetMode.toString(16).toUpperCase();
        break;
      }
      //
      // Retrieve waiting tasks
      //
      WaitingTasks = GetWaitingTasks(pEvent);
      //
      // Update information in the plug in window
      //
      Window.add2(EventObjects,
                  "0x" + pEvent.toString(16).toUpperCase(),
                  GetObjectName(pEvent),
                  "0x" + Signaled.toString(16).toUpperCase(),
                  sResetMode,
                  sMaskMode,
                  WaitingTasks);
    }
  }
}

/*********************************************************************
*
*       UpdateWatchDogs()
*
*  Functions description:
*    Called from the update() function in order to retrieve watchdog
*    information and update the watchdogs list.
*
*  Parameters
*    Window: Object used to access the plug in window.
*/
function UpdateWatchDogs(Window) {
  var TimeDex;
  var Period;
  var GlobalTime;
  var Watchdog;
  var pWatchdog;

  GlobalTime = OS_Global.Time;
  for (pWatchdog = Debug.evaluate(RootWatchdog); (pWatchdog != undefined) && (pWatchdog != 0); pWatchdog = Debug.evaluate("(*(OS_WD*)" + pWatchdog + ").pNext")) {
    Watchdog = Debug.evaluate("*(OS_WD*)" + pWatchdog);
    if (Watchdog == undefined) {
      //
      // Update information in the plug in window for ERRORS
      //
      Window.add2(WatchDogs,
                  "0x" + pWatchdog.toString(16).toUpperCase(),
                  "<error>",
                  "<error>",
                  "<error>");
      break;  // Leave for loop on error
    } else {
      //
      // Read information from queue structure
      //
      TimeDex = Watchdog.TimeDex;
      Period  = Watchdog.Period;
      //
      // Retrieve waiting tasks
      //
      WaitingTasks = GetWaitingTasks(pWatchdog);
      //
      // Update information in the plug in window
      //
      Window.add2(WatchDogs,
                  "0x" + pWatchdog.toString(16).toUpperCase(),
                  GetObjectName(pWatchdog),
                  (TimeDex - GlobalTime) + " (" + TimeDex + ")",
                  Period,
                  WaitingTasks);
    }
  }
}

/*********************************************************************
*
*       UpdateSystemInformation()
*
*  Functions description:
*    Called from the update() function in order to retrieve system
*    information and update the system information window.
*
*  Parameters
*    Window: Object used to access the plug in window.
*/
function UpdateSystemInformation(Window) {
  var Status;
  var SystemTime;
  var pCurrentTask;
  var ActiveTask;
  var Libmode;
  var VersionNum;
  var Version;
  var Major;
  var Minor;
  var Patch;
  var Revision;

  var aFunctions = ["OS_CreateTask_XR", "OS_CreateTask_R", "OS_CreateTask_S",
                    "OS_CreateTask_SP", "OS_CreateTask_D", "OS_CreateTask_DP",
                    "OS_CreateTask_DT", "OS_CreateTask_SAFE",
                    "OS_TASK_Create_XR", "OS_TASK_Create_R", "OS_TASK_Create_S",
                    "OS_TASK_Create_SP", "OS_TASK_Create_D", "OS_TASK_Create_DP",
                    "OS_TASK_Create_DT", "OS_TASK_Create_SAFE",
                    "OS_CreateTaskEx_XR", "OS_CreateTaskEx_R", "OS_CreateTaskEx_S",
                    "OS_CreateTaskEx_SP", "OS_CreateTaskEx_D", "OS_CreateTaskEx_DP",
                    "OS_CreateTaskEx_DT", "OS_CreateTaskEx_SAFE",
                    "OS_TASK_CreateEx_XR", "OS_TASK_CreateEx_R", "OS_TASK_CreateEx_S",
                    "OS_TASK_CreateEx_SP", "OS_TASK_CreateEx_D", "OS_TASK_CreateEx_DP",
                    "OS_TASK_CreateEx_DT", "OS_TASK_CreateEx_SAFE"];

  var aLibmodes = ["Extreme Release (XR)",
                   "Release (R)",
                   "Stackcheck (S)",
                   "Stackcheck + Profiling (SP)",
                   "Debug (D)",
                   "Debug + Profiling (DP)",
                   "Debug + Trace + Profiling (DT)",
                   "Safe (SAFE)"];
  //
  // Read information
  //
  Status       = Debug.evaluate(OSStatus);
  SystemTime   = OS_Global.Time >>> 0;  // Right shift without sign extension. Used to cast from signed to unsigned.
  pCurrentTask = OS_Global.pCurrentTask;
  pActiveTask  = OS_Global.pActiveTask;
  VersionNum   = Debug.evaluate("OS_Version");
  //
  // Check if OS_Status could be evaluated
  //
  if (Status == undefined) {
    Status = "n.a.";
  }
  //
  // Check Libmode
  //
  for (var i = 0; i < aFunctions.length; ++i) {
    Libmode = Debug.evaluate(aFunctions[i]);
    if (Libmode != undefined) {
      Libmode = aLibmodes[i % 8];
      break;
    } else {
      Libmode = "n.a.";
    }
  }
  //
  // Compute information
  //
  if        (pCurrentTask == undefined) {
    pCurrentTask = "n.a."
  } else if (pCurrentTask == "0") {
    pCurrentTask = "None"
  } else {
    pCurrentTask = "0x" + pCurrentTask.toString(16).toUpperCase() + " (" + GetTaskName(pCurrentTask) + ")";
  }
  if        (pActiveTask == undefined) {
    pActiveTask = "n.a."
  } else if (pActiveTask == "0") {
    pActiveTask = "None"
  } else {
    pActiveTask = "0x" + pActiveTask.toString(16).toUpperCase() + " (" + GetTaskName(pActiveTask) + ")";
  }
  if (VersionNum == undefined) {
    Version = "n.a.";
  } else {
    Major    = Math.floor( VersionNum / 10000);
    Minor    = Math.floor((VersionNum / 100) % 100);
    Patch    = Math.floor((VersionNum % 100) % 25);
    Revision = Math.floor((VersionNum % 100) / 25);

    if (VersionNum < 50800) {
      if (Minor < 10) {
        Version = Major + ".0" + Minor;                       // Add leading zero
      } else {
        Version = Major + "." + Minor;
      }
      if (Patch != 0) {
        Version = Version + String.fromCharCode(96 + Patch);  // Add patch value as character
      }
      if (Patch == 0 && Revision != 0) {                      // If there is no patch value but a revision value add a dot
        Version = Version + ".";
      }
      if (Revision != 0) {
        Version = Version + Revision;
      }
    } else {
      Version = Major + "." + Minor + "." + Patch + "." + Revision;
    }
  }
  //
  // Update information in the plug in window
  //
  Window.add2(SystemInformation, "System Status", (Status == "OS_OK") ? "O.K." : Status);
  Window.add2(SystemInformation, "System Time",   SystemTime);
  Window.add2(SystemInformation, "Current Task",  pCurrentTask);
  Window.add2(SystemInformation, "Active Task",   pActiveTask);
  Window.add2(SystemInformation, "embOS Build",   Libmode);
  Window.add2(SystemInformation, "embOS Version", Version);
}

/*********************************************************************
*
*       UpdateRWLock()
*
*  Functions description:
*    Called from the update() function in order to retrieve RW lock
*    information and update the RW lock list.
*
*  Parameters
*    Window: Object used to access the plug in window.
*/
function UpdateRWLock(Window) {
  var RWLock;
  var pRWLock;

  for (pRWLock = Debug.evaluate(RootRWLock); (pRWLock != undefined) && (pRWLock != 0); pRWLock = Debug.evaluate("(*(OS_RWLOCK*)" + pRWLock + ").pNext")) {
    RWLock = Debug.evaluate("*(OS_RWLOCK*)" + pRWLock);
    if (RWLock == undefined) {
      //
      // Update information in the plug in window for ERRORS
      //
      Window.add2(ReadersWriterLock,
                  "0x" + pRWLock.toString(16).toUpperCase(),
                  "<error>",
                  "<error>",
                  "<error>");
      break;  // Leave for loop on error
    } else {
      //
      // Update information in the plug in window
      //
      Window.add2(ReadersWriterLock,
                  "0x" + pRWLock.toString(16).toUpperCase(),
                  GetObjectName(pRWLock),
                  (RWLock.Semaphore.Cnt == 0) ? "Locked" : "Unlocked",
                  RWLock.NumReaders,
                  RWLock.Semaphore.Cnt);
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
*       getregs()
*
*  Functions description:
*    If a thread is selected in the threads window, this function returns
*    an array containing the saved registers o the selected thread.
*
*    arm registers: r0-r12, sp, lr, pc, and psr
*    risc-v registers: x1-x31, pc
*
*  Parameters
*    x: Pointer to task control block
*/
function getregs(x) {
  if (Debug.evaluate("SkipSaveMainContextRISCV")) {  // This is for embOS for RISCV
    var aRegs       = new Array(32);
    var Interrupted = 0;
    var PreviousPC;
    var VectoredMode;

    //
    // Load permanent registers from stack.
    //
    aRegs[riscv_sp]   = Debug.evaluate("((OS_TASK*)" + x + ")->pStack");  // Evaluate stack pointer of the task
    //                  TargetInterface.peekWord(aRegs[riscv_sp] +  0);   // We don't need Counters
    aRegs[riscv_s0]   = TargetInterface.peekWord(aRegs[riscv_sp] +  4);   // Load s0
    aRegs[riscv_s1]   = TargetInterface.peekWord(aRegs[riscv_sp] +  8);   // Load s1
    aRegs[riscv_s2]   = TargetInterface.peekWord(aRegs[riscv_sp] + 12);   // Load s2
    aRegs[riscv_s3]   = TargetInterface.peekWord(aRegs[riscv_sp] + 16);   // Load s3
    aRegs[riscv_s4]   = TargetInterface.peekWord(aRegs[riscv_sp] + 20);   // Load s4
    aRegs[riscv_s5]   = TargetInterface.peekWord(aRegs[riscv_sp] + 24);   // Load s5
    aRegs[riscv_s6]   = TargetInterface.peekWord(aRegs[riscv_sp] + 28);   // Load s6
    aRegs[riscv_s7]   = TargetInterface.peekWord(aRegs[riscv_sp] + 32);   // Load s7
    aRegs[riscv_s8]   = TargetInterface.peekWord(aRegs[riscv_sp] + 36);   // Load s8
    aRegs[riscv_s9]   = TargetInterface.peekWord(aRegs[riscv_sp] + 40);   // Load s9
    aRegs[riscv_s10]  = TargetInterface.peekWord(aRegs[riscv_sp] + 44);   // Load s10
    aRegs[riscv_s11]  = TargetInterface.peekWord(aRegs[riscv_sp] + 48);   // Load s11
    aRegs[riscv_ra]   = TargetInterface.peekWord(aRegs[riscv_sp] + 52);   // Load ra
    aRegs[riscv_pc]   = TargetInterface.peekWord(aRegs[riscv_sp] + 52);   // Load ra (used as pc)
    //
    // Adjust stack pointer. 64 bytes were reserved for context preservation.
    // 12 bytes of them were just reserved in order to keep the stack aligned.
    //
    aRegs[riscv_sp]  += 64;
    //
    // We can't restore caller save registers and mepc, because we
    // don't know how big the interrupt stack frame is.
    //
    aRegs[riscv_t0]  = 0;
    aRegs[riscv_t1]  = 0;
    aRegs[riscv_t2]  = 0;
    aRegs[riscv_a0]  = 0;
    aRegs[riscv_a1]  = 0;
    aRegs[riscv_a2]  = 0;
    aRegs[riscv_a3]  = 0;
    aRegs[riscv_a4]  = 0;
    aRegs[riscv_a5]  = 0;
    aRegs[riscv_a6]  = 0;
    aRegs[riscv_a7]  = 0;
    aRegs[riscv_t3]  = 0;
    aRegs[riscv_t4]  = 0;
    aRegs[riscv_t5]  = 0;
    aRegs[riscv_t6]  = 0;
    //
    // Evaluate gp and tp
    //
    aRegs[riscv_gp] = Debug.evaluate("__global_pointer$") || 0;
    aRegs[riscv_tp] = Debug.evaluate("((OS_TASK*)" + x + ")->pTLS") || Debug.evaluate("__thread_pointer$");

    return aRegs;
  } else {
    var aRegs       = new Array(17);
    var Interrupted = 0;
    var FPU_Used    = 0;
    var OSSwitch;
    var MPU_Used;

    MPU_Used = Debug.evaluate("(unsigned int*)(*(OS_TASK*)" + x + ").MPU_Config");
    MPU_Used = (MPU_Used != NULL) ? 1 : 0;

    //
    // Retrieve current top-of-stack
    //
    aRegs[arm_sp] = Debug.evaluate("((OS_TASK*)" + x + ")->pStack");
    //
    // Handle well known registers
    //
    aRegs[arm_sp]   += 4;  // "pop" Counters
    aRegs[arm_r4]    = TargetInterface.peekWord(aRegs[arm_sp]);
    aRegs[arm_sp]   += 4;  // "pop" R4
    aRegs[arm_r5]    = TargetInterface.peekWord(aRegs[arm_sp]);
    aRegs[arm_sp]   += 4;  // "pop" R5
    aRegs[arm_r6]    = TargetInterface.peekWord(aRegs[arm_sp]);
    aRegs[arm_sp]   += 4;  // "pop" R6
    aRegs[arm_r7]    = TargetInterface.peekWord(aRegs[arm_sp]);
    aRegs[arm_sp]   += 4;  // "pop" R7
    aRegs[arm_r8]    = TargetInterface.peekWord(aRegs[arm_sp]);
    aRegs[arm_sp]   += 4;  // "pop" R8
    aRegs[arm_r9]    = TargetInterface.peekWord(aRegs[arm_sp]);
    aRegs[arm_sp]   += 4;  // "pop" R9
    aRegs[arm_r10]   = TargetInterface.peekWord(aRegs[arm_sp]);
    aRegs[arm_sp]   += 4;  // "pop" R10
    aRegs[arm_r11]   = TargetInterface.peekWord(aRegs[arm_sp]);
    aRegs[arm_sp]   += 4;  // "pop" R11
    if (MPU_Used != 0) {
      aRegs[arm_sp] += 8;  // "pop" CONTROL & Dummy
    }
    aRegs[arm_exec]  = TargetInterface.peekWord(aRegs[arm_sp]);
    aRegs[arm_sp]   += 4;  // "pop" exec location
    //
    // Handle remaining registers through special treatment
    //
    if (Debug.evaluate("OS_SwitchAfterISR_ARM")) {  // This is for embOS for ARM
      //
      // Check if this task has been interrupted (i.e., exec location is addr. of OS_Switch() + 12)
      //
      OSSwitch = Debug.evaluate("OS_SwitchAfterISR_ARM");
      if (((aRegs[arm_exec] & ~1) == (OSSwitch + 12)) ||       // RTOS.s compiled in arm mode
          ((aRegs[arm_exec] & ~1) == (OSSwitch + 10))) {       // RTOS.s compiled in thumb mode
        Interrupted = 1;
      }
      //
      // Restore appropriate register contents
      //
      if (Interrupted == 0) {  // Remaining register contents have NOT been preserved.
        aRegs[arm_r0]  = "0x00000000";                         // unknown after cooperative task switch
        aRegs[arm_r1]  = "0x00000000";                         // unknown after cooperative task switch
        aRegs[arm_r2]  = "0x00000000";                         // unknown after cooperative task switch
        aRegs[arm_r3]  = "0x00000000";                         // unknown after cooperative task switch
        aRegs[arm_r12] = "0x00000000";                         // unknown after cooperative task switch
        aRegs[arm_lr]  = aRegs[arm_exec];                      // Set LR to exec location
        aRegs[arm_psr] = (aRegs[arm_exec] & 1) ? 0x3F : 0x1F;  // Thumb vs. ARM mode?
      } else {                 // Remaining register contents have been preserved.
        aRegs[arm_r0]  = TargetInterface.peekWord(aRegs[arm_sp]);
        aRegs[arm_sp] += 4;  // "pop" R0
        aRegs[arm_r1]  = TargetInterface.peekWord(aRegs[arm_sp]);
        aRegs[arm_sp] += 4;  // "pop" R1
        aRegs[arm_r2]  = TargetInterface.peekWord(aRegs[arm_sp]);
        aRegs[arm_sp] += 4;  // "pop" R2
        aRegs[arm_r3]  = TargetInterface.peekWord(aRegs[arm_sp]);
        aRegs[arm_sp] += 4;  // "pop" R3
        aRegs[arm_r12] = TargetInterface.peekWord(aRegs[arm_sp]);
        aRegs[arm_sp] += 4;  // "pop" R12
        aRegs[arm_lr]  = TargetInterface.peekWord(aRegs[arm_sp]);
        aRegs[arm_sp] += 4;  // "pop" LR
        aRegs[arm_pc]  = TargetInterface.peekWord(aRegs[arm_sp]);
        aRegs[arm_sp] += 4;  // "pop" PC
        aRegs[arm_psr] = TargetInterface.peekWord(aRegs[arm_sp]);
        aRegs[arm_sp] += 4;  // "pop" PSR
      }
    } else {                                    // This is for embOS for Cortex-M
      var OSSwitchEnd;
      var v;
      //
      // Check if this task has used the FPU
      //
      v = TargetInterface.peekWord(0xE000ED88);
      if ((v & 0x00F00000) != 0) {                    // Is the FPU enabled (CPACR b23..b20)?
        v = TargetInterface.peekWord(0xE000EF34);
        if ((v & 0xC0000000) != 0) {                  // Is the (lazy) hardware state preservation enabled (FPCCR b31..b30)?
          if ((aRegs[arm_exec] & 0x00000010) == 0) {  // Has this task used the FPU (LR b4)?
            FPU_Used = 1;
          }
        }
      }
      //
      // Check if this task has been interrupted (i.e., PC is located in OS_Switch() function)
      //
      if (FPU_Used == 0) {  // FPU registers have not been preserved, PC is located on stack 6 bytes after current SP
        aRegs[arm_pc] = TargetInterface.peekWord(aRegs[arm_sp] + (4 *  6));
      } else {              // FPU registers have been preserved, PC is located on stack 22 bytes after current SP
        aRegs[arm_pc] = TargetInterface.peekWord(aRegs[arm_sp] + (4 * 22));
      }
      OSSwitch    = Debug.evaluate("OS_Switch");
      OSSwitchEnd = Debug.evaluate("OS_Switch_End");
      if ((aRegs[arm_pc] <= OSSwitch) || (aRegs[arm_pc] >= OSSwitchEnd)) {
        Interrupted = 1;
      }
      //
      // Restore appropriate register contents
      //
      if (FPU_Used == 0) {       // FPU registers have not been preserved, use regular stack layout
        if (Interrupted == 0) {  // Task called OS_Switch(), unwind stack to show previous state.
          aRegs[arm_r0]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;  // "pop" R0
          aRegs[arm_r1]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;  // "pop" R1
          aRegs[arm_r2]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;  // "pop" R2
          aRegs[arm_r3]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;  // "pop" R3
          aRegs[arm_r12]  = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;  // "pop" R12
          aRegs[arm_lr]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;  // "pop" LR
          // aRegs[arm_pc] has already been read above
          aRegs[arm_sp]  += 4;  // "pop" PC
          aRegs[arm_psr]  = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;  // "pop" PSR
          //
          // unwind OS_Switch()
          //
          aRegs[arm_r2]  = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp] += 4;  // "pop" R2
          aRegs[arm_r3]  = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp] += 4;  // "pop" R3
          aRegs[arm_pc]  = aRegs[arm_lr];
        } else {                 // Task was preempted, no additional unwinding required.
          aRegs[arm_r0]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;  // "pop" R0
          aRegs[arm_r1]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;  // "pop" R1
          aRegs[arm_r2]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;  // "pop" R2
          aRegs[arm_r3]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;  // "pop" R3
          aRegs[arm_r12]  = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;  // "pop" R12
          aRegs[arm_lr]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;  // "pop" LR
          // aRegs[arm_pc] has already been read above
          aRegs[arm_sp]  += 4;  // "pop" PC
          aRegs[arm_psr]  = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;  // "pop" APSR
        }
      } else {                   // FPU registers have been preserved, use extended stack layout
        if (Interrupted == 0) {  // Task called OS_Switch(), unwind stack to show previous state.
          aRegs[arm_sp]  += (4 * 16);  // "pop" S16..S31
          aRegs[arm_r0]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;         // "pop" R0
          aRegs[arm_r1]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;         // "pop" R1
          aRegs[arm_r2]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;         // "pop" R2
          aRegs[arm_r3]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;         // "pop" R3
          aRegs[arm_r12]  = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;         // "pop" R12
          aRegs[arm_lr]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;         // "pop" LR
          // aRegs[arm_pc] has already been read above
          aRegs[arm_sp]  += 4;         // "pop" PC
          aRegs[arm_psr]  = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;         // "pop" PSR
          aRegs[arm_sp]  += (4 * 18);  // "pop" S0..S15, FPSCR, and Res.
          //
          // unwind OS_Switch()
          //
          aRegs[arm_r2]  = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp] += 4;         // "pop" R2
          aRegs[arm_r3]  = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp] += 4;         // "pop" R3
          aRegs[arm_pc]  = aRegs[arm_lr];
        } else {                 // Task was preempted, no additional unwinding required.
          aRegs[arm_sp]  += (4 * 16);  // "pop" S16..S31
          aRegs[arm_r0]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;         // "pop" R0
          aRegs[arm_r1]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;         // "pop" R1
          aRegs[arm_r2]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;         // "pop" R2
          aRegs[arm_r3]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;         // "pop" R3
          aRegs[arm_r12]  = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;         // "pop" R12
          aRegs[arm_lr]   = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;         // "pop" LR
          // aRegs[arm_pc] has already been read above
          aRegs[arm_sp]  += 4;         // "pop" PC
          aRegs[arm_psr]  = TargetInterface.peekWord(aRegs[arm_sp]);
          aRegs[arm_sp]  += 4;         // "pop" APSR
          aRegs[arm_sp]  += (4 * 18);  // "pop" S0..S15, FPSCR, and Res.
        }
      }
    }
    return aRegs;
  }
}

/*********************************************************************
*
*       init()
*
*  Functions description:
*    This function describes the look of the threads window.
*/
function init() {
  Threads.clear();
  Threads.setColumns("Name", "Run Count", "Priority", "Status", "Timeout", "Stack Info", "Id", "Time Slice", "Task Events");
  Threads.setSortByNumber("Priority");
  if (Threads.setColor) {
    Threads.setColor("Status", "Ready", "Executing", "Waiting");
  }
  if (Threads.setColumns2) {
    Threads.setColumns2(Timers,             "Timers",       "Name", "Hook",         "Timeout",        "Period"                                           );
    Threads.setColumns2(Mailboxes,          "Mailboxes",    "Name", "Messages",     "Message Size",   "Buffer Address", "Waiting Tasks",  "In Use"       );
    Threads.setColumns2(Queues,             "Queues",       "Name", "Messages",     "Buffer Address", "Buffer Size",    "Waiting Tasks"                  );
    Threads.setColumns2(Mutexes,            "Mutexes",      "Name", "Owner",        "Use Counter",    "Waiting Tasks"                                    );
    Threads.setColumns2(Semaphores,         "Semaphores",   "Name", "Count",        "Waiting Tasks"                                                      );
    Threads.setColumns2(MemoryPools,        "MemoryPools",  "Name", "Total Blocks", "Block Size",     "Max. Usage",     "Buffer Address", "Waiting Tasks");
    Threads.setColumns2(EventObjects,       "EventObjects", "Name", "Signaled",     "Reset Mode",     "Mask Mode",      "Waiting Tasks"                  );
    Threads.setColumns2(WatchDogs,          "WatchDogs",    "Name", "Timeout",      "Period"                                                             );
    Threads.setColumns2(ReadersWriterLock,  "RW Lock",      "Name", "Status",       "Max. number of tokens",       "Tokens left"                         );
    Threads.setColumns2(SystemInformation,  SystemInformation, "Value"                                                                                   );
  }
}

/*********************************************************************
*
*       getstate()
*
*  Parameters
*    task: Task Id
*
*  Functions description:
*    This function returns the state of a task.
*/
function getState(task) {
  var WaitObj;
  var sName;
  var sText;

  //
  // Retrieve waiting object name if available
  //
  WaitObj = Debug.evaluate("(((OS_TASK*)" + task.pWaitList + ")->pWaitList)");
  sName   = GetObjectName(WaitObj);
  sText   = (WaitObj) ? (" 0x" + WaitObj.toString(16).toUpperCase()) : "";
  if (sName != "") {
    sText += " (" + sName + ")";
  }
  //
  // Return corresponding state + waiting object name if available
  //
  if ((task.Stat & TS_MASK_SUSPEND_CNT) != 0) {
    return "Suspended";
  } else {
    switch (task.Stat & (TS_MASK_TIMEOUT | TS_MASK_STATE)) {
    case TS_READY:
      return "Ready";
    case TS_READY | TS_MASK_TIMEOUT:
      return "Delayed";
    case TS_WAIT_ANY:
    case TS_WAIT_ANY | TS_MASK_TIMEOUT:
      return "Blocked";
    case TS_WAIT_EVENT:
    case TS_WAIT_EVENT | TS_MASK_TIMEOUT:
      return "Waiting for Task Event" + sText;
    case TS_WAIT_EVENTOBJ:
    case TS_WAIT_EVENTOBJ | TS_MASK_TIMEOUT:
      return "Waiting for Event Object" + sText;
    case TS_WAIT_MBNE:
    case TS_WAIT_MBNE | TS_MASK_TIMEOUT:
      return "Waiting for message in Mailbox" + sText;
    case TS_WAIT_MBNF:
    case TS_WAIT_MBNF | TS_MASK_TIMEOUT:
      return "Waiting for space in Mailbox" + sText;
    case TS_WAIT_MEMPOOL:
    case TS_WAIT_MEMPOOL | TS_MASK_TIMEOUT:
    case TS_WAIT_MEMPOOL_old:
    case TS_WAIT_MEMPOOL_old | TS_MASK_TIMEOUT:
      return "Waiting for Memory Pool" + sText;
    case TS_WAIT_QNE:
    case TS_WAIT_QNE | TS_MASK_TIMEOUT:
      return "Waiting for message in Queue" + sText;
    case TS_WAIT_QNF:
    case TS_WAIT_QNF | TS_MASK_TIMEOUT:
      return "Waiting for space in Queue" + sText;
    case TS_WAIT_MUTEX:
    case TS_WAIT_MUTEX | TS_MASK_TIMEOUT:
      return "Waiting for Mutex" + sText;
    case TS_WAIT_SEMA:
    case TS_WAIT_SEMA | TS_MASK_TIMEOUT:
      return "Waiting for Semaphore" + sText;
    default:
      return "Invalid";
    }
  }
}

/*********************************************************************
*
*       update()
*
*  Functions description:
*    This function is called to update the threads window and its
*    entries upon debug stop.
*/
function update() {
  var pTask;
  var pCurrentTask;
  var OS_TASK;
  var Id;
  var TaskName;
  var NumActivations;
  var StackUsage;
  var TimeSliceReload;
  var TimeSliceRem;
  var sName;

  //
  // Retrieve OS_Global object
  //
  OS_Global = Debug.evaluate("OS_Global");
  //
  // Check if embOS is existent
  //
  if (OS_Global == undefined) {
    return;
  }
  //
  // Check if embOS V5.14 or earlier is used and, if so, set appropriate defines
  //
  if (Debug.evaluate("OS_Status") != undefined) {
    OSStatus       = "OS_Status";

    RootEvent      = "OS_pEventRoot";
    RootMailbox    = "OS_pMailboxRoot";
    RootMEMF       = "OS_pMEMFRoot";
    RootObjName    = "OS_pObjNameRoot";
    RootQueue      = "OS_pQRoot";
    RootRWLock     = "OS_pRWLockRoot";
    RootWatchdog   = "OS_pWDRoot";

    TaskStackBase  = "OS_TASK.pStackBot";
    //
    // Check if embOS V4.xx is used and, if so, set appropriate defines
    //
    if (Debug.evaluate("OS_pRSemaRoot") != undefined) {
      CastMEMF   = "(OS_MEMF*)";
      CastMutex  = "(OS_RSEMA*)";
      CastQueue  = "(OS_Q*)";
      CastSema   = "(OS_CSEMA*)";

      RootMutex  = "OS_pRSemaRoot";
      RootSema   = "OS_pCSemaRoot";
    } else {
      RootMutex  = "OS_pMutexRoot";
      RootSema   = "OS_pSemaRoot";
    }
  }
  //
  // Clear entire threads window
  //
  Threads.clear();
  //
  // Update plug in lists
  //
  if (Threads.newqueue2 != undefined) {
    if (Threads.shown(Timers))             UpdateTimers(Threads);
    if (Threads.shown(Mailboxes))          UpdateMailboxes(Threads);
    if (Threads.shown(Queues))             UpdateQueues(Threads);
    if (Threads.shown(Mutexes))            UpdateMutexes(Threads);
    if (Threads.shown(Semaphores))         UpdateSemaphores(Threads);
    if (Threads.shown(MemoryPools))        UpdateMemoryPools(Threads);
    if (Threads.shown(EventObjects))       UpdateEventObjects(Threads);
    if (Threads.shown(WatchDogs))          UpdateWatchDogs(Threads);
    if (Threads.shown(ReadersWriterLock))  UpdateRWLock(Threads);
    if (Threads.shown(SystemInformation))  UpdateSystemInformation(Threads);
  }
  //
  // Retrieve start of linked task list from target
  //
  pTask = OS_Global.pTask;
  //
  // Create new queue if at least one task is present
  //
  if (pTask != 0) {
    if (Threads.newqueue2 == undefined) {
      Threads.newqueue("Task List");
    }
    //
    // Retrieve currently executing task, if any
    //
    pCurrentTask = OS_Global.pCurrentTask;
  }
  //
  // Iterate through linked list of tasks and create an entry to the queue for each
  //
  while (pTask != 0) {
    //
    // Retrieve Task ID
    //
    Id = pTask;
    //
    // Get a pointer to this task control block
    //
    OS_TASK = Debug.evaluate("*(OS_TASK*)" + pTask);
    if (OS_TASK == undefined) {
      return;
    }
    //
    // Retrieve TaskName (unavailable in some libmodes)
    //
    TaskName = GetTaskName(pTask);
    //
    // Retrieve NumActivations (unavailable in some libmodes)
    //
    NumActivations = (OS_TASK.NumActivations != undefined) ? OS_TASK.NumActivations : "n.a.";
    //
    // Retrieve Stackinfo (unavailable in some libmodes)
    //
    if (OS_TASK.StackSize != undefined) {
      var MaxStackUsed;
      var index;

      if (OS_TASK.StackSize > StackCheckLimit) {
        //
        // Skip stack check if stack is too big
        //
        MaxStackUsed = "??";
      } else {
        //
        // Check if TargetInterface.findByte is implemented
        //
        if (TargetInterface.findByte) {
          if (Debug.evaluate("OS_Status") != undefined) {
            index = TargetInterface.findNotByte(OS_TASK.pStackBot, OS_TASK.StackSize, 0xCD);
          } else {
            index = TargetInterface.findNotByte(OS_TASK.pStackBase, OS_TASK.StackSize, 0xCD) ;
          }
        } else {
          var abStack;
          if (Debug.evaluate("OS_Status") != undefined) {
            abStack = TargetInterface.peekBytes(OS_TASK.pStackBot, OS_TASK.StackSize);
          } else {
            abStack = TargetInterface.peekBytes(OS_TASK.pStackBase, OS_TASK.StackSize);
          }
          for (index = 0; index < OS_TASK.StackSize; index++) {
            if (abStack[index].toString(16) != "cd") {
              break;
            }
          }
        }
        MaxStackUsed = OS_TASK.StackSize - index;
      }
      if (Debug.evaluate("OS_Status") != undefined) {
        StackUsage = MaxStackUsed + " / " + OS_TASK.StackSize + " @ 0x" + OS_TASK.pStackBot.toString(16).toUpperCase();
      } else {
        StackUsage = MaxStackUsed + " / " + OS_TASK.StackSize + " @ 0x" + OS_TASK.pStackBase.toString(16).toUpperCase();
      }
    } else {
      StackUsage = "n.a.";
    }
    //
    // Retrieve TimeSliceReload and TimeSliceRem (unavailable in some libmodes)
    //
    TimeSliceReload = (OS_TASK.TimeSliceReload != undefined) ? OS_TASK.TimeSliceReload : "n.a.";
    TimeSliceRem    = (OS_TASK.TimeSliceRem    != undefined) ? OS_TASK.TimeSliceRem :    "n.a.";
    //
    // Add task to queue
    //
    Threads.add(TaskName,                                                                                                          // Name
                NumActivations,                                                                                                    // Run count
                OS_TASK.Priority,                                                                                                  // Priority
                (pTask == pCurrentTask) ? "Executing" : getState(OS_TASK),                                                         // Status
                ((OS_TASK.Stat & TS_MASK_TIMEOUT) != 0) ? (OS_TASK.Timeout - OS_Global.Time) + " (" + (OS_TASK.Timeout >>> 0) + ")" : "",  // Timeout
                StackUsage,                                                                                                        // Stack info
                "0x" + Id.toString(16).toUpperCase(),                                                                              // ID
                TimeSliceRem + " / " + TimeSliceReload,                                                                            // Time slice
                "0x" + OS_TASK.Events.toString(16).toUpperCase(),                                                                  // Events
                //
                // If task is executing, getregs() must not be called. We therefore pass a null pointer.
                // If task is inactive, getregs() must be called. We therefore pass a pointer to this task.
                //
                (pTask == pCurrentTask) ? undefined : pTask
               );
    pTask = OS_TASK.pNext;
  }
  //
  // Add idle to queue
  //
  Threads.add("Idle", "", "", (pCurrentTask == 0) ? "Executing" : "", "", "", "", "", "", undefined);
}

/*********************************************************************
*
*       gettls()
*
*  Functions description:
*    This function is called to retrieve the TLS address of the
*    specified task.
*/
function gettls(pTask) {
  var pTLS;

  //
  // Retrieve OS_Global object
  //
  OS_Global = Debug.evaluate("OS_Global");
  //
  // If null is passed to gettls(pTask), get the current task.
  //
  if (pTask == null) {
    pTask = OS_Global.pCurrentTask;
  }
  //
  // Load the pTLS address of the selected task.
  //
  pTLS = Debug.evaluate("(*(OS_TASK*)" + pTask + ").pTLS");
  //
  // If the selected task does not use TLS, load tbss section address instead.
  //
  if (pTLS == 0) {
    pTLS = Debug.evaluate("__tbss_start__");
  }
  return pTLS;
}

/*********************************************************************
*
*       getContextSwitchAddrs()
*
*  Functions description:
*    Returns an unsigned integer array containing the base addresses
*    of all functions that complete a task switch when executed.
*/
function getContextSwitchAddrs() {

  var aAddrs;
  var Addr;

  Addr = Debug.evaluate("&OS_ChangeTask_VFP");

  if (Addr == undefined) {
    Addr = Debug.evaluate("&OS_ChangeTask_STD");
  }
  if (Addr == undefined) {
    Addr = Debug.evaluate("&OS_ChangeTask");
  }
  if (Addr == undefined) {
    return [];
  }
  aAddrs = new Array(1);
  aAddrs[0] = Addr;
  return aAddrs;
}

/*********************************************************************
*
*       getOSName()
*
*  Functions description:
*    Returns the name of the RTOS this script supplies support for
*/
function getOSName() {
  return "embOS";
}

/****** End Of File *************************************************/
