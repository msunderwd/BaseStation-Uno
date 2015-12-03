
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
 *       TQ (quit)
 *    (if address set)
 *       TV<velocity> (speed)
 *       TX (emergency stop)
 *       TF .... (function)
 *       Tf .... (force function)
 *       TR0 or TR1 (direction -- really any nonzero is forward)
 *       Tr (release address)
 *       Td (dispatch address)
 *       TL <address> (set long address)
 *       TS <address> (set short address)
 *       TE <roster id> (set address from roster)
 *       TC ... (consist functions)
 *       Tc ... (roster consist functions)
 *       TI (idle)
 *       Ts <mode> (set speed step mode)
 *       Tm ... (momentary)
 *       Tq ... (request)
 *    (if address not set)
 *       TL <address>
 *       TS <address>
 *       TE <roster id> 
 *       TC ...
 *       Tc ...


 * 'S' : Second Throttle Command
 *    (same as T commands)
 * 'C' : (obsolete) Throttle Command
 *    (same as T commands)
 * 'N' : Name of Device
 *        N<name>  -- respond with "*"
 * 'H' : Hardware Info
 *        HU<deviceUDID> : UDID Unique device ID
 * 'P' : Panel
 *        PP... : Track Power
 *        PT... : Turnouts
 *        PR... : Routes
 * 'R' : Roster
 *        RC... : consist
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
 * MultiThrottle Response format: M(id)(char)(key)<;>(action)
 * Example:  Fn21, throttle T, Loco S123 is high:  MTAS123<;>F121
 *           Fn21, Throttle T, Loco S123 is low:   MTAS123<;>F021
 *           Report speed of LocoS123: MTAS123<;>V27
 *           Report direction of Loco S123: MTAS123<;>R1 (or R0)
 *           Report speed step mode: MTAS123<;>s14 (or s28 or s128)
 *           Fn21 Loco S123 Momentary State: MTAS123<;>m121 (or m021)
 *           Add or remove Loco S123 MT+S123<;>   MT-S123<;>
 *          
 * Turnout response format (send list, state
 *           Turnout system name DCCPP01 state: PTA<state>DCCPP01
 * ... etc. Refer to jmri.jmrit.withrottle package for details.
 */

#ifndef WiThrottle_h
#define WiThrottle_h

#include "DCCpp_Uno.h"
#include "Config.h"
#include "Comm.h"
//#include "EthernetPort.h"

#if (USE_W5200 > 0)
//#include <DhcpV2_0.h>
//#include <EthernetV2_0.h>
//#include <EthernetClientV2_0.h>
//#include <EthernetUdpV2_0.h>
#endif
#if (USE_W5100 > 0)
//#include <Dhcp.h>
//#include <Ethernet.h>
//#include <EthernetClient.h>
//#include <EthernetUdp.h>
#endif

#include "SerialCommand.h"
#include "DCCpp_Uno.h" // Do we really need this here?
#include "Accessories.h" // Do we really need this here?

#define NUM_WITHROTTLE_CLIENTS 8

struct Loco {
  String  address;
  byte reg;        // BaseStation Register #
  byte speedsteps; // 14, 28, or 128
  byte  speedval;   // 0 to 126 or -1, I suppose. 
  bool dir;        // true is forward, false is reverse
  Loco *next;
  Loco(String s);
  const String addressnum();
  byte adjustedSpeed();
};

class Throttle {
public:
  char id;
  bool assigned;
  byte num_locos;
  Loco *locos;
  Throttle *next;
  Throttle(char id);
  Loco *addLoco(String addr);
  Loco *getLoco(String addr);
  void releaseLoco(String addr);
  void releaseAllLocos();
  void takeAction(String k, String a);
  void takeAction(Loco *, String a);
  void sendReply(char p, String k, String a);
};



class WiThrottleClient : public EthernetClient {
public:
  WiThrottleClient(void);
  WiThrottleClient(EthernetClient e);
  String name;
  String deviceUDID;
  Throttle *throttles;
  byte num_throttles;
  boolean alreadyConnected;
  boolean usingHeartbeat;
  Throttle *addThrottle(char id);
  Throttle *getThrottle(char id);
  void releaseThrottle(char id);
  void releaseAllThrottles(void);
  void sendReply(String& s);
};


class WiThrottle {
 private:  
  char commandString[MAX_COMMAND_LENGTH+1];
  EthernetServer *server;
  WiThrottleClient en_client, c;
  WiThrottleClient *reglist[MAX_MAIN_REGISTERS+1];
  int addressMap[MAX_MAIN_REGISTERS];
  int num_clients = 0;
  bool alreadyConnected = false;
  int dhcp_loop_count = 0;
  volatile RegisterList *_regs;
 protected:
  void handleTrackPowerMsg(String s);
  void handleTurnoutMsg(String s);
  void handleThrottleMsg(WiThrottleClient &c, int t, String s);
  void setAddress(WiThrottleClient &c, int t, String addr);
  void setSpeed(WiThrottleClient &c, int t, int speed);
  void setDirection(WiThrottleClient &c, int t, int dir);
  void sendSpeedAndDirCmd(WiThrottleClient &c, int t);
  //WiThrottleClient *getClient(EthernetClient &c);
  //WiThrottleClient* getClient(Serial c);
  void freeClient(WiThrottleClient &c);
  void startEKG();
  void stopEKG();
  void addMultiThrottleLoco(WiThrottleClient &c, char i, String k, String a);
  void removeMultiThrottleLoco(WiThrottleClient &c, char i, String k, String a);
  void doMultiThrottleLocoAction(WiThrottleClient &c, char i, String k, String a);
 public:
  bool parse(WiThrottleClient *c, String& s);
  bool parse(String& s);
  void initialize(volatile RegisterList *l);
  void process();
  void sendReply(String buf);
  void sendIntroMessage(void);

  }; // WiThrottle

#endif // WiThrottle_h
