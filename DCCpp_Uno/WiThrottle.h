
/**********************************************************************

WiThrottlePort.h
COPYRIGHT (c) 2015 Mark Underwood

Part of DCC++ BASE STATION for the Arduino Uno by Gregg E. Berman

**********************************************************************/

/**
 * WiThrottlePort: Server interface for WiThrottle compatible devices
 * Protocol:
 *
 * Commands from Handheld Device:
 * 'T' : Throttle Command
 * 'S' : Second Throttle Command
 * 'C' : (obsolete) Throttle Command
 * 'N' : Name of Device
 * 'H' : Hardware Info
 *       'U' : UDID Unique device ID
 * 'P' : Panel
 *       'P' : Track Power
 *       'T' : Turnouts
 *       'R' : Routes
 * 'R' : Roster
 * 'C' : Consists
 * 'Q' : Device Quit
 * '*' : Device Heartbeat
 * '*+' : Device Heartbeat start
 * '*+' : Device Heartbeat end
 * ---- Added in V2.0:
 * 'M' : Multi-Throttle
 * 'D' : Direct byte packet to rails
 *
 * Responses from Server
 * Track Power:  'PPA' + '0' (off), '1' (on), '2' (unknown), minimum 4 char length
 * Web Server Port: 'PW' + {port #}
 * Functions: 'RF##' (throttle 1) or 'RS##' (throttle 2) \[name]
 *
 * ... etc. Refer to jmri.jmrit.withrottle package for details.X
 */

#ifndef WiThrottle_h
#define WiThrottle_h

//#include <Dhcp.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetUdp.h>
#include "SerialCommand.h"
#include "DCCpp_Uno.h" // Do we really need this here?
#include "Accessories.h" // Do we really need this here?

#define NUM_WITHROTTLE_CLIENTS 10

struct Throttle {
  bool assigned;
  int reg;
  int address;
  int speed;
  int direction;
};

class WiThrottleClient {
public:
  WiThrottleClient(void);
  EthernetClient en_client;
  String name;
  String deviceUDID;
  Throttle throttles[2];
  boolean alreadyConnected;
  boolean usingHeartbeat;
  void sendReply(String& s);
};


class WiThrottle {
 private:  
  char commandString[MAX_COMMAND_LENGTH+1];
  EthernetServer *server;
  EthernetClient en_client;
  WiThrottleClient *reglist[MAX_MAIN_REGISTERS+1];
  int num_clients = 0;
  bool alreadyConnected = false;
  int dhcp_loop_count = 0;
RegisterList *_regs;
 protected:
  void handleTrackPowerMsg(String s);
  void handleTurnoutMsg(String s);
  void handleThrottleMsg(WiThrottleClient *c, int t, String s);
  void setAddress(WiThrottleClient *c, int t, int addr);
  void setSpeed(WiThrottleClient *c, int t, int speed);
  void setDirection(WiThrottleClient *c, int t, int dir);
  void sendSpeedAndDirCmd(WiThrottleClient *c, int t);
  WiThrottleClient *getClient(EthernetClient c);
  void freeClient(WiThrottleClient *c);
  bool parse(WiThrottleClient *c, String& s);
  void startEKG();
  void stopEKG();
 public:
  void initialize(RegisterList *l);
  void process();
  void sendReply(String buf);
  }; // WiThrottle

#endif // WiThrottle_h
