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

Purpose : Demonstrates usage of the HID component of the USB stack
          as a keyboard.
          Types a predefined string like from a regular keyboard.

Additional information:
  Preparations:
    It is advised to open a notepad application before
    connecting the USB cable.

  Expected behavior:
    The sample types a predefined string like from a regular keyboard.

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
#include <ctype.h>
#include "USB.h"
#include "USB_HID.h"
#include "BSP.h"
#include "stm32f4xx.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
//
// Specifies whether the return key should be sent in this sample.
//
#define SEND_RETURN 0
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
  void USBD_HID_Keyboard_Init(void);
  void USBD_HID_Keyboard_RunTask(void *);
#ifdef __cplusplus
}
#endif

/*********************************************************************
*
*       Local data definitions
*
**********************************************************************
*/
typedef struct {
  U16 KeyCode;
  char cCharacter;
} SCANCODE_TO_DESC;


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
  0x1115,         // ProductId
  "Vendor",       // VendorName
  "HID keyboard sample",  // ProductName
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
static const U8 _aHIDReport[] = {
  USB_HID_GLOBAL_USAGE_PAGE + 1, USB_HID_USAGE_PAGE_GENERIC_DESKTOP,
  USB_HID_LOCAL_USAGE + 1, USB_HID_USAGE_KEYBOARD,
  USB_HID_MAIN_COLLECTION + 1, USB_HID_COLLECTION_APPLICATION,
    USB_HID_GLOBAL_USAGE_PAGE + 1, 7,
    USB_HID_LOCAL_USAGE_MINIMUM + 1, 224,
    USB_HID_LOCAL_USAGE_MAXIMUM + 1, 231,
    USB_HID_GLOBAL_LOGICAL_MINIMUM + 1, 0,
    USB_HID_GLOBAL_LOGICAL_MAXIMUM + 1, 1,
    USB_HID_GLOBAL_REPORT_SIZE + 1, 1,
    USB_HID_GLOBAL_REPORT_COUNT + 1, 8,
    USB_HID_MAIN_INPUT + 1, USB_HID_VARIABLE,
    USB_HID_MAIN_INPUT + 1, 1,
    USB_HID_LOCAL_USAGE_MINIMUM + 1, 0,
    USB_HID_LOCAL_USAGE_MAXIMUM + 1, 101,
    USB_HID_GLOBAL_LOGICAL_MINIMUM + 1, 0,
    USB_HID_GLOBAL_LOGICAL_MAXIMUM + 1, 101,
    USB_HID_GLOBAL_REPORT_SIZE + 1, 8,
    USB_HID_GLOBAL_REPORT_COUNT + 1, 6,
    USB_HID_MAIN_INPUT + 1, 0,
    USB_HID_GLOBAL_USAGE_PAGE + 1, USB_HID_USAGE_PAGE_LEDS,
    USB_HID_LOCAL_USAGE_MINIMUM + 1, 1,
    USB_HID_LOCAL_USAGE_MAXIMUM + 1, 5,
    USB_HID_GLOBAL_LOGICAL_MINIMUM + 1, 0,
    USB_HID_GLOBAL_LOGICAL_MAXIMUM + 1, 1,
    USB_HID_GLOBAL_REPORT_SIZE + 1, 1,
    USB_HID_GLOBAL_REPORT_COUNT + 1, 5,
    USB_HID_MAIN_OUTPUT + 1, 2,
    USB_HID_GLOBAL_REPORT_COUNT + 1, 3,
    USB_HID_MAIN_OUTPUT + 1, 1,
  USB_HID_MAIN_ENDCOLLECTION
};
volatile uint32_t user_flags = 0x00;
static const  SCANCODE_TO_DESC _aScanCode2StringTable[] = {
  { 0x04, 'a'},
  { 0x05, 'b'},
  { 0x06, 'c'},
  { 0x07, 'd'},
  { 0x08, 'e'},
  { 0x09, 'f'},
  { 0x0A, 'g'},
  { 0x0B, 'h'},
  { 0x0C, 'i'},
  { 0x0D, 'j'},
  { 0x0E, 'k'},
  { 0x0F, 'l'},
  { 0x10, 'm'},
  { 0x11, 'n'},
  { 0x12, 'o'},
  { 0x13, 'p'},
  { 0x14, 'q'},
  { 0x15, 'r'},
  { 0x16, 's'},
  { 0x17, 't'},
  { 0x18, 'u'},
  { 0x19, 'v'},
  { 0x1A, 'w'},
  { 0x1B, 'x'},
  { 0x1C, 'y'},
  { 0x1D, 'z'},
  { 0x1E, '1'},
  { 0x1F, '2'},
  { 0x20, '3'},
  { 0x21, '4'},
  { 0x22, '5'},
  { 0x23, '6'},
  { 0x24, '7'},
  { 0x25, '8'},
  { 0x26, '9'},
  { 0x27, '0'},
  { 0x2C, ' '},
  { 0x37, '.'}
};


/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static USB_HID_HANDLE _hInst;
/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _Output
*
*  Function description
*    Outputs a string
*/
static void _Output(const char *sString) {
  U8   ac[8];
  char cTemp;
  unsigned int i;
  unsigned int j;

  memset(ac, 0, sizeof(ac));
  for (i = 0; sString[i] != 0; i++) {
    //
    // A character is uppercase if it's hex value is less than 0x61 ('a')
    // and greater or equal to 0x41 ('A'), therefore we set the
    // LeftShiftUp bit for those characters
    //
    if (sString[i] < 0x61 && sString[i] >= 0x41) {
      ac[0] = (1 << 1);
      cTemp = tolower((int)sString[i]);
    } else {
      cTemp = sString[i];
    }
    for (j = 0; j < sizeof(_aScanCode2StringTable)/sizeof(_aScanCode2StringTable[0]); j++) {
      if (_aScanCode2StringTable[j].cCharacter == cTemp) {
        ac[2] = _aScanCode2StringTable[j].KeyCode;
      }
    }
    USBD_HID_Write(_hInst, &ac[0], 8, 0);
    memset(ac, 0, sizeof(ac));
    //
    // Send a 0 field packet to tell the host that the key has been released
    //
    USBD_HID_Write(_hInst, &ac[0], 8, 0);
    USB_OS_Delay(50);
  }
}

#if (SEND_RETURN == 1)
/*********************************************************************
*
*       _SendReturnCharacter
*
*  Function description
*    Outputs a return character
*/
static void _SendReturnCharacter(void) {
  U8 ac[8];

  memset(ac, 0, sizeof(ac));
  ac[2] = 0x28;
  USBD_HID_Write(_hInst, &ac[0], 8, 0);
  memset(ac, 0, sizeof(ac));
  //
  // Send a 0 field packet to tell the host that the key has been released
  //
  USBD_HID_Write(_hInst, &ac[0], 8, 0);
  USB_OS_Delay(50);
}
#endif

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       USBD_HID_Keyboard_Init
*
*  Function description
*    Add HID keyboard to USB stack
*/
void USBD_HID_Keyboard_Init(void) {
  static U8           _abOutBuffer[USB_HS_INT_MAX_PACKET_SIZE];
  USB_HID_INIT_DATA   InitData;
  USB_ADD_EP_INFO     EPIntIn;
  USB_ADD_EP_INFO     EPIntOut;

  memset(&InitData, 0, sizeof(InitData));
  EPIntIn.Flags = 0;                             // Flags not used.
  EPIntIn.InDir = USB_DIR_IN;                    // IN direction (Device to Host)
  EPIntIn.Interval = 64;                            // Interval of 8 ms (125 us * 64)
  EPIntIn.MaxPacketSize = USB_HS_INT_MAX_PACKET_SIZE;    // Maximum packet size (64 for Interrupt).
  EPIntIn.TransferType = USB_TRANSFER_TYPE_INT;         // Endpoint type - Interrupt.
  InitData.EPIn = USBD_AddEPEx(&EPIntIn, NULL, 0);

  EPIntOut.Flags = 0;                             // Flags not used.
  EPIntOut.InDir = USB_DIR_OUT;                   // OUT direction (Host to Device)
  EPIntOut.Interval = 64;                            // Interval of 8 ms (125 us * 64)
  EPIntOut.MaxPacketSize = USB_HS_INT_MAX_PACKET_SIZE;    // Maximum packet size (64 for Interrupt).
  EPIntOut.TransferType = USB_TRANSFER_TYPE_INT;         // Endpoint type - Interrupt.
  InitData.EPOut = USBD_AddEPEx(&EPIntOut, _abOutBuffer, sizeof(_abOutBuffer));

  InitData.pReport = _aHIDReport;
  InitData.NumBytesReport = sizeof(_aHIDReport);
  _hInst = USBD_HID_Add(&InitData);
}

/*********************************************************************
*
*       USBD_HID_Keyboard_RunTask
*
*  Function description
*    Performs the HID echo1 operation
*/
void USBD_HID_Keyboard_RunTask(void * pPara) {
  //const char * sInfo0 = "This sample is based on the SEGGER emUSB-Device software with an HID component. ";
  //const char * sInfo1 = "For further information please visit: www.segger.com ";
  const char * char_s = "S";
  const char * char_t = "T";
  const char * char_m = "M";

  USB_USE_PARA(pPara);
  while (1) {

    //
    // Wait for configuration
    //
    while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
      BSP_ToggleLED(0);
      USB_OS_Delay(50);
    }
    USB_OS_Delay(100);
    BSP_SetLED(0);
    //
    // The "_SendReturnCharacter()" line can be added if desired. Please set
    // the SEND_RETURN define to 1 in the "Defines, configurable" section of this file.
    // This function will send a Return/Enter key to the host.
    // In some cases this is not wanted as a return key may have undesired behavior.
    //
    
    if(user_flags & (1<<0)){
      _Output(char_s);
      #if (SEND_RETURN == 1)
         _SendReturnCharacter();
      #endif
      user_flags &= ~(1<<0);
    }
    if(user_flags & (1<<1)){
      _Output(char_t);
      #if (SEND_RETURN == 1)
         _SendReturnCharacter();
      #endif
      user_flags &= ~(1<<1);
    }
    if(user_flags & (1<<2)){
      _Output(char_m);
      #if (SEND_RETURN == 1)
         _SendReturnCharacter();
      #endif
      user_flags &= ~(1<<2);
    }
/*
    _Output(sInfo0);
#if (SEND_RETURN == 1)
    _SendReturnCharacter();
#endif
    _Output(sInfo1);
#if (SEND_RETURN == 1)
    _SendReturnCharacter();
#endif
*/
  }

}

/*********************************************************************
*
*       MainTask
*
* Function description
*   USB handling task.
*/
#if USBD_SAMPLE_NO_MAINTASK == 0
void MainTask(void) {
  USBD_Init();
  USBD_SetDeviceInfo(&_DeviceInfo);
  USBD_HID_Keyboard_Init();
  USBD_Start();
  USBD_HID_Keyboard_RunTask(NULL);
}
void EXTI15_10_IRQHandler(void){
  switch(EXTI->PR & (EXTI_PR_PR10|EXTI_PR_PR11|EXTI_PR_PR12)){
      case EXTI_PR_PR10:
        user_flags |= (1<<0);
        break;
      case EXTI_PR_PR11:
        user_flags |= (1<<1);
        break;
      case EXTI_PR_PR12:
        user_flags |= (1<<2);
        break;
  } 
  EXTI->PR |= EXTI_PR_PR10|EXTI_PR_PR11|EXTI_PR_PR12;
}
#endif
/**************************** end of file ***************************/
