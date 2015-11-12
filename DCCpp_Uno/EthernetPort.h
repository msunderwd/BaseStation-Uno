
/**********************************************************************

EthernetPort.h
COPYRIGHT (c) 2015 Mark Underwood

Part of DCC++ BASE STATION for the Arduino Uno by Gregg E. Berman

**********************************************************************/

#ifndef EthernetPort_h
#define EthernetPort_h

#include <Arduino.h>

struct EthernetPort {

  void initialize();
  void process();
  void sendReply(String buf);
private:  
  bool parse_dccpptcp(String& s);
  }; // EthernetPort

#endif // EthernetPort_h
