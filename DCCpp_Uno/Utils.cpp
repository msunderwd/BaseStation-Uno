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

// Send a reply message to ALL of the open ports.
// 
// Supports: Serial, Ethernet
void sendReply(String buf) {
  Serial.print(buf);
#if (USE_ETHERNET > 0)
  ethernetPort.sendReply(buf);
#endif
}

