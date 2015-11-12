/**********************************************************************

Utils.cpp
COPYRIGHT (c) 2015 Mark Underwood

Part of DCC++ BASE STATION for the Arduino Uno by Gregg E. Berman

**********************************************************************/

#include "DCCpp_Uno.h"
#include "EthernetPort.h"



String reply_buffer;

extern bool use_ethernet;
#ifdef USE_ETHERNET
extern EthernetPort ethernetPort;
#endif

void sendReply(String buf) {
  Serial.print(buf);
#ifdef USE_ETHERNET
  if (use_ethernet) {
    ethernetPort.sendReply(buf);
  }
#endif
}

