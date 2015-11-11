/**********************************************************************

WiThrottlePort.cpp
COPYRIGHT (c) 2015 Mark Underwood

Part of DCC++ BASE STATION for the Arduino Uno by Gregg E. Berman

**********************************************************************/

#include "WiThrottle.h"
#include "PacketRegister.h"
#include <SPI.h>
#include <EthernetBonjour.h>

// TODO: Use real MAC Address
// Note: mac[] defined in EthernetPort.cpp
extern byte mac[];
const int port_no = 1236; // Make this configurable?


const String SEND_START_FLAG = "SEND";
const String RECV_START_FLAG = "RECEIVE";
const String newLine = "\n";
const String VersionNumber = "2.0";
const int pulseInterval = 16; // 16 seconds till disconnect on heartbeat fail

void WiThrottle::initialize(RegisterList *l) {
  IPAddress ip(192,168, 0, 27);
  IPAddress myDns(192,168,1,1);
  IPAddress gateway(192,168,1,1);
  IPAddress subnet(255,255,255,0);

  server = new EthernetServer(port_no);
#if (USE_DHCP > 0)
  Ethernet.begin(mac);
#else
  Ethernet.begin(mac, ip, dns, gateway, subnet);
#endif
#if (USE_BONJOUR > 0)
  EthernetBonjour.begin("WiThrottleServer");
  EthernetBonjour.addServiceRecord("DCC++ WiTHrottle Server", port_no, MDNSServiceTCP);
#endif
  dhcp_loop_count = 0;

  for (int i=0; i < MAX_MAIN_REGISTERS+1; i++)
    reglist[i] = NULL;

  _regs = l;
  // Fire up the server.
  server->begin();
}

void WiThrottle::process(void) {
#if (USE_BONJOUR)
  EthernetBonjour.run();
#endif
  int client_num = -1;
  en_client = server->available();

  String buffer = String("");
  
  if (en_client) {
    // Xmit welcome message on connect
    if (!alreadyConnected) {
      en_client.flush();
      en_client.println("VN" + VersionNumber); // Version #
      en_client.println("PW" + String(port_no)); // Web server Port #
      alreadyConnected = true;
    }

    // Receive
    while (en_client.available() > 0) {
      char thisChar = en_client.read();
      if (thisChar == '\n') {
	buffer += thisChar;
	if (this->parse(getClient(en_client), buffer)) {
	  // Valid message received and handled.
	  // woo hoo. Do what?
	}
	break;
      } else {
	buffer += thisChar;
      }
    }
  }
  dhcp_loop_count++;
  if (dhcp_loop_count == DHCP_POLL_INTERVAL) {
    // TODO: Handle DHCP failures more robustly than "not at all"
#if (USE_DHCP > 0)
   Ethernet.maintain();
#endif
  }
}

bool WiThrottle::parse(WiThrottleClient *c, String& s) {
  // This is more of a "handle" than just a "parse", but...
  switch(s[0]) {
  case 'C': // Treat 'C' just like 'T'
  case 'T':
    // Throttle command
    this->handleThrottleMsg(c, 1, s.substring(1));
    break;
  case 'S':
    // Second Throttle Command
    this->handleThrottleMsg(c, 2, s.substring(1));
    break;
  case 'M':
    // Multi Throttle Command
    break;
  case 'D':
    // Direct command
    break;
  case '*':
    // Heartbeat
    if (s.length() > 1) {
      switch(s[1]) {
      case '+':
	if (!c->usingHeartbeat)
	  startEKG();
	break;
      case '-':
	if (c->usingHeartbeat)
	  stopEKG();
	break;
      }
    }
    break;

  case 'N':
    // Device Name
    c->name = s.substring(1);
    c->sendReply("*" + String(pulseInterval));
    break;

  case 'H':
    // Hardware
    switch(s[1]) {
    case 'U':
      // UDID
      c->deviceUDID = s.substring(2);
      break;
    }
    break;

  case 'P':
    // Panel
    switch(s[1]) {
    case 'P':
      // Power
      this->handleTrackPowerMsg(s.substring(2));
      break;
    case 'T':
      // Turnout
      this->handleTurnoutMsg(s.substring(2));
      break;
    case 'R':
      // Route
      break;
    }
    break;
  case 'R':
    // Roster
    break;
  case 'Q':
    // Quit - remove this client from the list?
    break;
  default:
    return false;
    
  }
  return true;
}

void WiThrottle::startEKG(void) {
}

void WiThrottle::stopEKG(void) {
}

void WiThrottle::handleTrackPowerMsg(String s) {
  if (s[0] == 'A') {
    if (s[1] == '1') {
      commandString[0] = '1';
      SerialCommand::parse(commandString);
    } else if (s[1] == '0') {
      commandString[0] = '0';
      SerialCommand::parse(commandString);
    }
  }
}

void WiThrottle::handleThrottleMsg(WiThrottleClient *c, int t, String s) {
  switch(s[0]) {
  case 'V': // Velocity
    setSpeed(c, t, s.substring(1).toInt());
    sendSpeedAndDirCmd(c, t);
    break;
  case 'X': // Emergency Stop
    setSpeed(c, t, 0);
    sendSpeedAndDirCmd(c, t);
    break;
  case 'F': // Function
    break;
  case 'f': // (v2.0+) force function
    break;
  case 'R': // Set Direction
    setDirection(c, t, s.substring(1).toInt());
    sendSpeedAndDirCmd(c, t);
    break;
  case 'r': // Release
  case 'd': // Dispatch
    freeClient(c);
    break;
  case 'L': // Set long address
  case 'S': // Set short address
    setAddress(c, t, s.substring(1).toInt());
    break;
  case 'E': // (v1.7+) Release Address from RosterEntry
    break;
  case 'C': // set loco for consist functions
    break;
  case 'c': // (v1.7+) set roster loco for consist functions
    break;
  case 'I': // Idle
    setSpeed(c, t, 0);
    sendSpeedAndDirCmd(c, t);
    break;
  case 's': // Set speed step mode
    // We're always 128 steps here.  Figure out how to scale.
    break;
  case 'm': // Handle momemtary function
    break;
  case 'q': // Handle request
    break;
  }
}

void WiThrottle::setAddress(WiThrottleClient *c, int t, int addr) {
  // Assign the address to the wi_client
  c->throttles[t].address = addr;

  // Try to get a register
  c->throttles[t].reg = _regs->getRegisterByAddr(addr);
  if (c->throttles[t].reg == -1) {
    // No register with this address.  Allocate a new one
    // by sending an "Idle" command to the address.
    c->throttles[t].reg = _regs->getEmptyRegister();
    String cmd = String("");
    cmd += c->throttles[t].reg;
    cmd += " ";
    cmd += addr;
    cmd += " ";
    cmd += "0 1";
    cmd.toCharArray(commandString, MAX_COMMAND_LENGTH+1);
    _regs->setThrottle(commandString);
    c->throttles[t].speed = 0;
    c->throttles[t].direction = 1;
  }
}

void WiThrottle::setSpeed(WiThrottleClient *c, int t, int speed) {
  c->throttles[t].speed = speed;
}

void WiThrottle::setDirection(WiThrottleClient *c, int t, int dir) {
  c->throttles[t].direction = dir;
}
void WiThrottle::sendSpeedAndDirCmd(WiThrottleClient *c, int t) {
  String cmd = String("<T");
  cmd += c->throttles[t].reg; // TODO
  cmd += " ";
  cmd += c->throttles[t].address;
  cmd += " ";
  cmd += c->throttles[t].speed;
  cmd += " ";
  cmd += c->throttles[t].direction;
  cmd += ">";
  cmd.toCharArray(commandString, MAX_COMMAND_LENGTH+1);
  _regs->setThrottle(commandString);
}

WiThrottleClient *WiThrottle::getClient(EthernetClient ec) {
  for (int i = 0; i < MAX_MAIN_REGISTERS+1; i++) {
    if ((reglist[i] != NULL) && (reglist[i]->en_client == ec)) {
      return(reglist[i]);
    }
  }
  // Not found. Look for an empty slot and create a new one.
  for (int i = 0; i < MAX_MAIN_REGISTERS+1; i++) {
    if (reglist[i] == NULL) {
      WiThrottleClient *w = new WiThrottleClient();
      reglist[i] = w;
      reglist[i]->en_client = ec;
      num_clients++;
      return(reglist[i]);
    }
  }
  // No available slot. Return Null
  return(NULL);
}

void WiThrottle::freeClient(WiThrottleClient *c) {
  for (int i = 0; i < MAX_MAIN_REGISTERS+1; i++) {
    if (reglist[i] == c) { // yes, equality...looking for the same object
      // TODO: Do something to free up the Register here.
      delete reglist[i]; // (sigh) no auto garbage collection in C++
      reglist[i] = NULL;
    }
  }
}

WiThrottleClient::WiThrottleClient() {
    alreadyConnected = false;
    usingHeartbeat = false;
}

void WiThrottleClient::sendReply(String& buf) {
  if (en_client && alreadyConnected) {
    en_client.println(buf + newLine);
  }
}

