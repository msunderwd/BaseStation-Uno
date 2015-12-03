/**********************************************************************

WiThrottlePort.cpp
COPYRIGHT (c) 2015 Mark Underwood

Part of DCC++ BASE STATION for the Arduino Uno by Gregg E. Berman

**********************************************************************/

#include "WiThrottle.h"
#include "PacketRegister.h"
#include "Accessories.h"
#include <SPI.h>

// TODO: Use real MAC Address
// Note: mac[] defined in EthernetPort.cpp
extern byte mac[];
const int port_no = 1236; // Make this configurable?


const String SEND_START_FLAG = "<";
const String RECV_START_FLAG = ">";
const String newLine = "\n";
const String VersionNumber = "2.0";
const int pulseInterval = 16; // 16 seconds till disconnect on heartbeat fail
WiThrottleClient c;

#if (COMM_TYPE == 1)
  EthernetClient en_client  = INTERFACE.available();
#else
#define en_client Serial
#endif


Loco::Loco(String addr) {
  address = addr;
  reg = 0;
  speedsteps = 128;
  speedval = 0;
  dir = true;
  next = NULL;
}

byte Loco::adjustedSpeed() {
  int v = speedval;
  if (speedsteps == 14) { return(speedval * 9); }
  else if (speedsteps == 28) { return ((byte)(((int)speedval)*45)/10); }
  else { return(speedval); }
}

const String Loco::addressnum() { return(address.substring(1)); }

/*
Throttle::Throttle(void) {
  assigned = false;
  num_locos = 0;
  locos = NULL;
  next = NULL;
}
*/
Throttle::Throttle(char x) {
  assigned = true;
  num_locos = 0;
  id = x;
  locos = NULL;
  next = NULL;
}

Loco *Throttle::addLoco(String addr) {
  int i = 0;
  Loco *t = locos;
  if (locos == NULL) {
    // No locos yet. Create first one.
    t = new Loco(addr);
    locos = t;
    //Serial.println("Creating(1) " + addr);
    if (locos == NULL) { Serial.println("FAIL"); }
  } else {
    while (t->next != NULL) {
      t = t->next;
      i++;
    }
    //Serial.println("Creating(2) " + addr);
    t->next = new Loco(addr);
    t->next->reg = i+1; // use the array index (+1) as the register num
    t = t->next;
  }
  num_locos++;
  Serial.print("ctor ID: ");
  Serial.println(t->reg);
  Serial.println("ctor address: " + t->address);
  return(t);
}

Loco *Throttle::getLoco(String addr) {
  if (num_locos == 0) { return(addLoco(addr)); } // no locos
  Loco *t = locos;
  while ((t != NULL) && (t->address != addr)) {
    t = t->next;
  }
  if (t == NULL) { return(addLoco(addr)); } // not found
  else {
    return(t);
  }
}

void Throttle::releaseLoco(String addr) {
  if (num_locos == 0) { return; } // no throttles
  Loco *t = locos;
  Loco *pt = NULL;
  while ((t != NULL) && (t->address != addr)) {
    t = t->next;
    pt = t;
  }
  if (t == NULL) { return; } // not found
  else {
    pt->next = NULL;
    delete t;
    num_locos--;
  }
}

void Throttle::releaseAllLocos() {
  Loco *l = locos;
  while (l != NULL) {
    l = locos->next;
    delete locos;
  }
}


void Throttle::takeAction(String addr, String action) {
  if (addr.equals("*")) {
    Serial.println("Wildcard address... processing all locos...");
    for (int i = 0; i < num_locos; i++) {
      takeAction(&locos[i], action);
    }
  } else {
    Serial.println("Single address." + addr);
    takeAction(getLoco(addr), action);
  }
}
void Throttle::takeAction(Loco *l, String action) {
  char cmdbuf[50];
  String cmd = "";
  if (l != NULL) {
    switch(action.charAt(0)) {
    case 'V': // Velocity
      l->speedval = action.substring(1).toInt();
      cmd = "t "; 
      cmd += String(int(l->reg));
      cmd += " " + l->addressnum() + " " + String(int(l->adjustedSpeed())) + " ";
      cmd += String((int(l->dir ? 1 : 0)));
      Serial.println("cmd = " + cmd);
      cmd.toCharArray(cmdbuf, 50);
      SerialCommand::parse(cmdbuf);
      sendReply('A', l->address, String("V") + String(l->speedval));
      break;

    case 'X': // Emergency Stop
      Serial.println("cmd = " + cmd);
      cmd.toCharArray(cmdbuf, 50);
      SerialCommand::parse(cmdbuf);
      sendReply('A', l->address, String("V") + String(l->speedval));
      break;

    case 'F': // function
      Serial.println("Do function." + action.substring(1));
      break;

    case 'f': // force function
      Serial.println("Do force function." + action.substring(1));
      break;

    case 'R': // direction
      l->dir = (action.substring(1).endsWith("0") ? false : true);
      cmd = "t ";
      cmd += l->reg;
      cmd += " " + l->addressnum() + " ";
      cmd += String(int(l->adjustedSpeed()));
      cmd += " " + String(int((l->dir ? 1 : 0)));
      Serial.println("cmd = " + cmd);
      cmd.toCharArray(cmdbuf, 50);
      SerialCommand::parse(cmdbuf);
      sendReply('A', l->address, String("R") + String(l->dir ? 1 : 0));
      break;

    case 'r': // release
    case 'd': // dispatch
      releaseLoco(l->address);
      sendReply('-', l->address, "");
      break;

    case 'L': // Set a long address
    case 'S': // Set a short address
      l->address = action.substring(1);
      sendReply('+', l->address, "");
      break;

    case 'E': // v >=1.7
    case 'c': // v >=1.7 Consist functions from roster
      // Roster actions not supported.
      break;

    case 'C': // Consist functions
      break;

    case 'I': // idle
      l->speedval = 0;
      cmd = "t ";
      cmd += l->reg;
      cmd += " " + l->addressnum() + " 0 ";
      cmd += (l->dir ? "1" : "0");
      Serial.println("cmd = " + cmd);
      cmd.toCharArray(cmdbuf, 50);
      SerialCommand::parse(cmdbuf);
      sendReply('A', l->address, String("V") + String(l->speedval));
      break;

    case 's': // v >=2.0 Set speed steps
      l->speedsteps = action.substring(1).toInt();
      sendReply('A', l->address, String("s") + String(l->speedsteps));
      break;

    case 'm': // v >=2.0 Set function to momentary or on/off
      break;

    case 'q': // v >=2.0 Request value
      // NOTE: As of JMRI 4.1.4 these aren't actually supported
      switch(action.charAt(1)) {
      case 'V':
	sendReply('A', l->address, String("V") + String(l->speedval));
	break;
      case 'R':
	sendReply('A', l->address, String("R") + l->dir ? "1" : "0");
      case 's':
	sendReply('A', l->address, String("s") + String(l->speedsteps));
      case 'm':
	// Not supported in JMRI 4.1.x??
      default:
	break;
      }
      break;
    default:
      Serial.println("ERROR: Unrecognized command! >> " + action);
      break;
    }
  }
}

void Throttle::sendReply(char prefix, String key, String action) {
  String reply = "M";
  reply += String(id);
  reply += String(prefix);
  reply += key;
  reply += "<;>";
  reply += action;
  en_client.println(reply);
  // Do something.
}

/*********************************************************************/

WiThrottleClient::WiThrottleClient(EthernetClient e) {
  name = "";
  deviceUDID = "";
  throttles = NULL;
  num_throttles = 0;
  alreadyConnected = false;
  usingHeartbeat = false;
}

Throttle *WiThrottleClient::addThrottle(char id) {
  Serial.print("Adding Throttle ID: ");
  Serial.println(id);
  Throttle *t = throttles;
  if (throttles == NULL) {
    // No throttles yet. Create first one.
    Serial.println("Create (1)");
    throttles = t = new Throttle(id);
    if (throttles == NULL) { Serial.println("FAIL"); }
  } else {
    while (t->next != NULL) {
      t = t->next;
    }
    Serial.println("Create (2)");
    t->next = new Throttle(id);
    t = t->next;
  }
  num_throttles++;
  Serial.println(num_throttles);
  return(t);
}

Throttle *WiThrottleClient::getThrottle(char id) {
  Serial.println("Looking for Throttle " + String(id));
  if (num_throttles == 0) { return(addThrottle(id)); } // no throttles
  Throttle *t = throttles;
  while ((t != NULL)) {
    Serial.println("Check ID: " + String(t->id));
    if (t->id == id) {
      Serial.println("Found ID: " + String(id));
      return(t);
    } else {
      t = t->next;
    }
  }
  if (t == NULL) { return(addThrottle(id)); } // not found
  else {
    return(t);
  }
}

void WiThrottleClient::releaseThrottle(char id) {
  if (num_throttles == 0) { return; } // no throttles
  Throttle *t = throttles;
  Throttle *pt = NULL;
  while ((t != NULL) && (t->id != id)) {
    t = t->next;
    pt = t;
  }
  if (t == NULL) { return; } // not found
  else {
    pt->next = NULL;
    t->releaseAllLocos();
    delete t;
    num_throttles--;
  }
}

void WiThrottleClient::releaseAllThrottles(void) {
  Throttle *t = throttles;
  while (t != NULL) {
    t = t->next;
    throttles->releaseAllLocos();
    delete throttles;
  }
}

void WiThrottle::initialize(volatile RegisterList *l) {
  Serial.println("Starting WiThrottle Init");

  Serial.println("New Server set.");

  for (int i=0; i < MAX_MAIN_REGISTERS+1; i++) {
    reglist[i] = NULL;
  }
  for (int i = 0; i < MAX_MAIN_REGISTERS; i++) {
    addressMap[i] = 0;
  }

  _regs = l;
  // Fire up the server.
  Serial.println("Server launched.");
}

void WiThrottle::process(void) {
  int client_num = -1;

  String buffer = String("");
  
  if (en_client) {
    // Xmit welcome message on connect
    if (!alreadyConnected) {
      en_client.flush();
      alreadyConnected = true;
    }

    // Receive
#if (COMM_TYPE == 1)
    while (en_client.connected() && en_client.available()) {
#else
      while (Serial.available()) {
#endif
      char thisChar = en_client.read();
      if (thisChar == '\n') {
	buffer += thisChar;
	//if (this->parse(getClient(en_client), buffer)) {
	if (this->parse(buffer)) {
	  // Valid message received and handled.
	  // woo hoo. Do what?
	} else {
	  // Failed to parse? How do we handle that??
	}
	break;
      } else {
	buffer += thisChar;
      }
    }
  }
}

  bool WiThrottle::parse(WiThrottleClient *c, String& s) {
    return this->parse(s);
  }

 bool WiThrottle::parse(String& s) {
   Throttle *t;
   Serial.println("WTS parsing: " + s);
   // This is more of a "handle" than just a "parse", but...
   s.trim(); // Drop leading and trailing whitespace.
   switch(s[0]) {
   case 'C': // Treat 'C' just like 'T'
   case 'T':
     // Throttle command (effectively obsolete)
     if ((c.num_throttles > 0) && (c.throttles != NULL)) {
       c.throttles->takeAction(s.substring(3, s.indexOf('<')), s.substring(s.indexOf('>')+1));
     }
     break;

   case 'S':
     // Second Throttle Command (effectively obsolete)
     if ((c.num_throttles > 1) && (c.throttles+1 != NULL)) {
       (c.throttles+1)->takeAction(s.substring(3, s.indexOf('<')), s.substring(s.indexOf('>')+1));
     }
     break;

   case 'M':
     // Multi Throttle Command
     switch(s.charAt(2)) {  // Second char is an OpCode, sort of. More of a Selector
     case '+':
       // Add a loco to throttle # id
       addMultiThrottleLoco(c, s.charAt(1), s.substring(3, s.indexOf('<')), s.substring(s.indexOf('>')+1));
       break;

     case '-':
       // Remove a loco from throttle # id
       removeMultiThrottleLoco(c, s.charAt(1), s.substring(3, s.indexOf('<')), s.substring(s.indexOf('>')+1));
       break;

     case 'A':
       // Loco Action
       doMultiThrottleLocoAction(c, s.charAt(1),s.substring(3, s.indexOf('<')), s.substring(s.indexOf('>')+1));
       break;

     default:
       // Invalid char. Do nothing.
       return false;
     } // switch
     break;

   case 'D':
     // Direct command
     break;

   case '*':
     // Heartbeat
     if (s.length() > 1) {
       switch(s[1]) {
       case '+':
	 if (!c.usingHeartbeat)
	   startEKG();
	 break;
       case '-':
	 if (c.usingHeartbeat)
	   stopEKG();
	 break;
       }
     }
     break;
     
   case 'N':
     // Device Name
     c.name = s.substring(1);
     sendIntroMessage();
     c.sendReply("*" + String(pulseInterval));
     break;
     
   case 'H':
     // Hardware
     switch(s[1]) {
     case 'U':
       // UDID
       c.deviceUDID = s.substring(2);
       Serial.print("Device UDID: ");
       Serial.println(c.deviceUDID);
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
       // Routes not supported.
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

 void WiThrottle::sendIntroMessage(void) {
   String msg;
   // Send version
   en_client.println("VN" + VersionNumber); // Version #
   // Send port #
   en_client.println("PW" + String(port_no)); // Web server Port #
   // Send power state
   if (digitalRead(SIGNAL_ENABLE_PIN_MAIN)) {
     en_client.println("PPA1");
   } else {
     en_client.println("PPA0");
   }
   // Send Turnout titles and state
   msg = "PTT]\\[Turnouts}|{Turnout]\\[Closed]|{2]\\[Thrown}|{4";
   en_client.println(msg);
   Turnout *tt = Turnout::firstTurnout;
   en_client.print("PTL");
   while(tt != NULL) {
     msg = "]\\[";
     msg += tt->data.id;
     msg += "}|{";
     if (tt->data.tStatus == 1) {
       msg += "1";
     } else {
       msg += "0";
     }
     en_client.print(msg);
     tt = tt->nextTurnout;
   }
   en_client.println("");
   // Send Route titles and state (LOL J/K)
 }

void WiThrottle::startEKG(void) {
  // This needs to start a timer somehow.
}

void WiThrottle::stopEKG(void) {
  // This needs to stop the timer somehow.
}

/** 
 * handleTrackPowerMsg()
 *
 * Turns power to the track on and off.
 */
 void WiThrottle::handleTrackPowerMsg(String s) {
  if (s[0] == 'A') {
      Serial.print("Power...");
    if (s[1] == '1') {
      commandString[0] = '1';
      SerialCommand::parse(commandString);
      Serial.println("ON");
    } else if (s[1] == '0') {
      commandString[0] = '0';
      SerialCommand::parse(commandString);
      Serial.println("OFF");
    }
  }
  // No response to client.
}

 void WiThrottle::handleTurnoutMsg(String s) {
   int id;
   Turnout *t;
   String msg;
   if (s.charAt(0) == 'A') {
     id = s.substring(2).toInt();
     t = Turnout::get(s.substring(2).toInt());
     if (t != NULL) {
       switch(s.charAt(1)) {
       case '2': // Toggle
	 if (t->data.tStatus == 0) {
	   t->activate(1);
	 } else {
	   t->activate(0);
	 }
	 break;
       case 'C': // Close
	 t->activate(0);
	 break;
       case 'T': // Throw
	 t->activate(1);
	 break;
       default:
	 Serial.println("Invalid Turnout Message: " + s);
	 break;
       } // switch
       msg = "PTA";
       msg += (t->data.tStatus == 0 ? '2' : '4');
       msg += t->data.id;
       en_client.println(msg);
     } // if !NULL
   } // if 'A'
 }

 void WiThrottle::addMultiThrottleLoco(WiThrottleClient &c, char id, String key, String action) {
   // Do something.
   String debug = "add MultiThrottleLoco id: ";
   Throttle *t;
   debug.concat(id);
   debug += " key: " + key + " action: " + action;
   Serial.println(debug);
   t = c.getThrottle(id);
   if (t != NULL) {
     // getLoco() will auto-call addLoco(), but will return existing loco instead of make duplicate 
     t->getLoco(key);  
     debug = "id = ";
     debug += id;
     debug += " num locos = ";
     debug += int(t->num_locos);
     t->sendReply('+', key, "");
   } else {
     debug = "no throttle!";
   }
   Serial.println(debug);
 }

 void WiThrottle::removeMultiThrottleLoco(WiThrottleClient &c, char id, String key, String action) {
   // Do something.
   String debug = "remove MultiThrottleLoco id: ";
   Throttle *t;
   debug.concat(id);
   debug += " key: " + key + " action: " + action;
   Serial.println(debug);
   t = c.getThrottle(id);
   if (t != NULL) { 
     t->releaseLoco(key);
     debug = "id = ";
     debug += id;
     debug += " num locos = ";
     debug += int(t->num_locos);
     t->sendReply('-', key, "");
   } else {
     debug = "no throttle!";
   }
   Serial.println(debug);
   // If there are no more locos on this Throttle, free up the
   // memory.  If a new loco gets added, we'll create a new throttle.
   if (t->num_locos == 0) {
     Serial.println("Throttle empty. Removing...");
     c.releaseThrottle(id);
   }
 }

 void WiThrottle::doMultiThrottleLocoAction(WiThrottleClient &c, char id, String key, String action) {
   // Do something.
   String debug = "do MultiThrottleLocoAction id: ";
   Throttle *t;
   debug.concat(id);
   debug += " key: " + key + " action: " + action;
   Serial.println(debug);
   t = c.getThrottle(id);
   if (t != NULL) {
     debug = "Taking action on Throttle ID " + key;
     t->takeAction(key, action);
   } else {
     debug = "no throttle!";
   }
   Serial.println(debug);
   
 }


void WiThrottle::setAddress(WiThrottleClient &c, int t, String addr) {
  // Assign the address to the wi_client
  Loco *l = c.throttles[t].getLoco(addr);
  if (l == NULL) { return; }

  l->address = addr;

  // Try to get a register
  //c.throttles[t].reg = _regs->getRegisterByAddr(addr);
  if (l->reg == -1) {
    // No register with this address.  Allocate a new one
    // by sending an "Idle" command to the address.
    //c.throttles[t].reg = _regs->getEmptyRegister();
    String cmd = String("");
    cmd += l->reg;
    cmd += " ";
    cmd += addr;
    cmd += " ";
    cmd += "0 1";
    cmd.toCharArray(commandString, MAX_COMMAND_LENGTH+1);
    //_regs->setThrottle(commandString);
    l->speedval = 0;
    l->dir = true;
  }
}

void WiThrottle::setSpeed(WiThrottleClient &c, int t, int speed) {
  Loco *l = &(c.throttles[t].locos[0]);
  if (l != NULL) {l->speedval = speed; }
}

void WiThrottle::setDirection(WiThrottleClient &c, int t, int dir) {
  Loco *l = &(c.throttles[t].locos[0]);
  if (l != NULL) { l->dir = (dir == 1); }
}
void WiThrottle::sendSpeedAndDirCmd(WiThrottleClient &c, int t) {
  Loco *l = &(c.throttles[t].locos[0]);
  if (l == NULL) { return; }

  
  String cmd = String("<T");
  cmd += l->reg; // TODO
  cmd += " ";
  cmd += l->address;
  cmd += " ";
  cmd += l->adjustedSpeed();
  cmd += " ";
  cmd += l->dir ? 1 : 0;
  cmd += ">";
  cmd.toCharArray(commandString, MAX_COMMAND_LENGTH+1);
  _regs->setThrottle(commandString);
}

/*
//WiThrottleClient *WiThrottle::getClient(EthernetClient &ec) {
WiThrottleClient *WiThrottle::getClient(Serial ec) {
  for (int i = 0; i < MAX_MAIN_REGISTERS+1; i++) {
    if ((reglist[i] != NULL) ) { //&& (reglist[i]->en_client == ec)) {
      return(reglist[i]);
    }
  }
  // Not found. Look for an empty slot and create a new one.
  for (int i = 0; i < MAX_MAIN_REGISTERS+1; i++) {
    if (reglist[i] == NULL) {
      WiThrottleClient *w = new WiThrottleClient();
      reglist[i] = w;
      //reglist[i]->en_client = ec;
      num_clients++;
      return(reglist[i]);
    }
  }
  // No available slot. Return Null
  return(NULL);
}
*/

void WiThrottle::freeClient(WiThrottleClient &c) {
  for (int i = 0; i < MAX_MAIN_REGISTERS+1; i++) {
    if (*reglist[i] == c) { // yes, equality...looking for the same object
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
  //if (en_client && alreadyConnected) {
  //if ( alreadyConnected) {
    en_client.println(buf + newLine);
    //}
}

