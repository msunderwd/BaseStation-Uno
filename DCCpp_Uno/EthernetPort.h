
/**********************************************************************

EthernetPort.h
COPYRIGHT (c) 2015 Mark Underwood

Part of DCC++ BASE STATION for the Arduino Uno by Gregg E. Berman

**********************************************************************/

#ifndef EthernetPort_h
#define EthernetPort_h

//#include <DhcpV2_0.h>
#include <EthernetV2_0.h>
#include <EthernetClientV2_0.h>
#include <EthernetUdpV2_0.h>
#include "SerialCommand.h"
#include "DCCpp_Uno.h" // Do we really need this here?
#include "Accessories.h" // Do we really need this here?

struct EthernetPort {

  void initialize();
  void process();
  void sendReply(String buf);
private:  
  bool parse_dccpptcp(String& s);
  }; // EthernetPort

#endif // EthernetPort_h
