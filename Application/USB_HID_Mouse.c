/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2003 - 2022     SEGGER Microcontroller GmbH              *
*                                                                    *
*       www.segger.com     Support: www.segger.com/ticket            *
*                                                                    *
**********************************************************************
*                                                                    *
*       emUSB-Device * USB Device stack for embedded applications    *
*                                                                    *
*       Please note: Knowledge of this file may under no             *
*       circumstances be used to write a similar product.            *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       emUSB-Device version: V3.50.0                                *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

Purpose : Demonstrates usage of the HID component as a mouse.

Additional information:
  Preparations:
    None.

  Expected behavior:
    The mouse cursor constantly jumps from left to right and back.

  Sample output:
    The target side does not produce terminal output.
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include <string.h>
#include "USB.h"
#include "USB_HID.h"
#include "BSP.h"

/*********************************************************************
*
*       Defines configurable
*
**********************************************************************
*/
#ifndef USBD_SAMPLE_NO_MAINTASK
#define USBD_SAMPLE_NO_MAINTASK  0
#endif

/*********************************************************************
*
*       Forward declarations
*
**********************************************************************
*/
#ifdef __cplusplus
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif
  void MainTask(void);
  void USBD_HID_Mouse_Init(void);
  void USBD_HID_Mouse_RunTask(void *);
#ifdef __cplusplus
}
#endif

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/
/*********************************************************************
*
*       Information that are used during enumeration
*/
#if USBD_SAMPLE_NO_MAINTASK == 0
static const USB_DEVICE_INFO _DeviceInfo = {
  0x8765,         // VendorId
  0x1112,         // ProductId
  "Vendor",       // VendorName
  "HID mouse sample",  // ProductName
  "12345678"      // SerialNumber
};
#endif

/*********************************************************************
*
*       _aHIDReport
*
*  This report is generated according to HID spec and
*  HID Usage Tables specifications.
*/
const U8 _aHIDReport[] = {
  USB_HID_GLOBAL_USAGE_PAGE + 1, USB_HID_USAGE_PAGE_GENERIC_DESKTOP,
  USB_HID_LOCAL_USAGE + 1, USB_HID_USAGE_MOUSE,
  USB_HID_MAIN_COLLECTION + 1, USB_HID_COLLECTION_APPLICATION,
    USB_HID_LOCAL_USAGE + 1, USB_HID_USAGE_POINTER,
    USB_HID_MAIN_COLLECTION + 1, USB_HID_COLLECTION_PHYSICAL,
      USB_HID_GLOBAL_USAGE_PAGE + 1, USB_HID_USAGE_PAGE_BUTTON,
      USB_HID_LOCAL_USAGE_MINIMUM + 1, 1,
      USB_HID_LOCAL_USAGE_MAXIMUM + 1, 3,
      USB_HID_GLOBAL_LOGICAL_MINIMUM + 1, 0,
      USB_HID_GLOBAL_LOGICAL_MAXIMUM + 1, 1,
      USB_HID_GLOBAL_REPORT_COUNT + 1, 3,
      USB_HID_GLOBAL_REPORT_SIZE + 1, 1,
      USB_HID_MAIN_INPUT + 1, USB_HID_VARIABLE,  // 3 button bits
      USB_HID_GLOBAL_REPORT_COUNT + 1, 1,
      USB_HID_GLOBAL_REPORT_SIZE + 1, 5,
      USB_HID_MAIN_INPUT + 1, USB_HID_CONSTANT,  // 5 bit padding
      USB_HID_GLOBAL_USAGE_PAGE + 1, USB_HID_USAGE_PAGE_GENERIC_DESKTOP,
      USB_HID_LOCAL_USAGE + 1, USB_HID_USAGE_X,
      USB_HID_LOCAL_USAGE + 1, USB_HID_USAGE_Y,
      USB_HID_GLOBAL_LOGICAL_MINIMUM + 1, (unsigned char) -127,
      USB_HID_GLOBAL_LOGICAL_MAXIMUM + 1, 127,
      USB_HID_GLOBAL_REPORT_SIZE + 1, 8,
      USB_HID_GLOBAL_REPORT_COUNT + 1, 2,
      USB_HID_MAIN_INPUT + 1, USB_HID_VARIABLE | USB_HID_RELATIVE,
    USB_HID_MAIN_ENDCOLLECTION,
  USB_HID_MAIN_ENDCOLLECTION
};


/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
static USB_HID_HANDLE _hInst;

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       USBD_HID_Mouse_Init
*
*  Function description
*    Add HID mouse class to USB stack
*/
void USBD_HID_Mouse_Init(void) {
  USB_HID_INIT_DATA InitData;
  USB_ADD_EP_INFO   EPIntIn;

  memset(&InitData, 0, sizeof(InitData));
  EPIntIn.Flags           = 0;                             // Flags not used.
  EPIntIn.InDir           = USB_DIR_IN;                    // IN direction (Device to Host)
  EPIntIn.Interval        = 64;                            // Interval of 8 ms (125 us * 64)
  EPIntIn.MaxPacketSize   = USB_HS_INT_MAX_PACKET_SIZE;    // Maximum packet size (64 for Interrupt).
  EPIntIn.TransferType    = USB_TRANSFER_TYPE_INT;         // Endpoint type - Interrupt.
  InitData.EPIn = USBD_AddEPEx(&EPIntIn, NULL, 0);

  InitData.pReport = _aHIDReport;
  InitData.NumBytesReport = sizeof(_aHIDReport);
  _hInst = USBD_HID_Add(&InitData);
}

/*********************************************************************
*
*       USBD_HID_Mouse_RunTask
*
*  Function description
*    Performs the HID mouse operation
*/
void USBD_HID_Mouse_RunTask(void * pPara) {
  U8 ac[3];

  USB_USE_PARA(pPara);
  while (1) {

    //
    // Wait for configuration
    //
    while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
      BSP_ToggleLED(0);
      USB_OS_Delay(50);
    }
    BSP_SetLED(0);
    memset(ac, 0, sizeof(ac));
    ac[1] = 20;   // To the left !
    USBD_HID_Write(_hInst, &ac[0], 3, 0);      // Make sure we send the number of bytes defined in REPORT
    USB_OS_Delay(500);
    ac[1] = (U8)-20;  // To the right !
    USBD_HID_Write(_hInst, &ac[0], 3, 0);      // Make sure we send the number of bytes defined in REPORT
    USB_OS_Delay(100);

  }
}

/*********************************************************************
*
*       MainTask
*
* USB handling task.
*/
#if USBD_SAMPLE_NO_MAINTASK == 0
void MainTask(void) {
  USBD_Init();
  USBD_SetDeviceInfo(&_DeviceInfo);
  USBD_HID_Mouse_Init();
  USBD_Start();
  USBD_HID_Mouse_RunTask(NULL);
}

#endif
/**************************** end of file ***************************/
