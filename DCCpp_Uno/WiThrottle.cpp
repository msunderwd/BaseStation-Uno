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

// WiThrottle Heartbeat Timer info
#define SECONDS_TO_DISCONNECT 16
#define MILLIS 1000
unsigned long last_heartbeat;
boolean heartbeat = false;
boolean heartbeatEnabled = false;

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

/** Throttle::takeAction()
 *
 * Handles velocity, function, etc.
 *
 * Generates an equivalent <t...> or <f...> serial command
 * and runs it through SerialCommand::parse().
 */
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
      cmd = "t "; 
      cmd += String(int(l->reg));
      cmd += " " + l->addressnum() + " -1 1";
      Serial.println("cmd = " + cmd);
      cmd.toCharArray(cmdbuf, 50);
      SerialCommand::parse(cmdbuf);
      sendReply('A', l->address, String("V0"));
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
      // Consist functions not supported (for now)
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

  handleHeartbeat();
  /*
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
  */
}

  void WiThrottle::handleHeartbeat(void) {
    unsigned long this_heartbeat = millis();
    unsigned long delta;
    if (c.usingHeartbeat) {
      // All is well. We had some kind of contact.
      c.last_heartbeat = this_heartbeat;
    } else {
      // Have we had a timeout?
      if (this_heartbeat < last_heartbeat) {
	// We've had a wrap event.  Do the math.
	// This will happen once every 50 days or so.
	delta = (0xFFFFFFFF - this_heartbeat) + c.last_heartbeat;
      } else {
	// Normal timeout, no wraparound
	delta = this_heartbeat - c.last_heartbeat;
      }
      if (delta > (SECONDS_TO_DISCONNECT * MILLIS)) {
	// Boom!  We've had a timeout. E-Stop everything.
	Serial.println("Heartbeat Failed!");
	for (int i = 0; i < c.num_throttles; i++) {
	  for (int j = 0; j < c.throttles[i].num_locos; j++) {
	    c.throttles[i].takeAction(c.throttles[i].locos[j].address, "X");
	  } // for locos
	} // for throttles
      } // if timeout
    } // if heartbeat else
    c.usingHeartbeat = false;
  }


  bool WiThrottle::parse(WiThrottleClient *c, String& s) {
    return this->parse(s);
  }

 bool WiThrottle::parse(String& s) {
   Throttle *t;
   Serial.println("WTS parsing: " + s);
   // This is more of a "handle" than just a "parse", but...
   s.trim(); // Drop leading and trailing whitespace.
   heartbeat = true;
   switch(s.charAt(0)) {
   case 'T':
     switch(s.charAt(1)) {
     case 'L':
     case 'S':
       t = c.getThrottle('T');
       if (t != NULL) {
	 t->releaseLoco(s.substring(2));
	 t->getLoco(s.substring(2));
       }
       break;
     default:
       t = c.getThrottle('T');
       if (t != NULL) {
	 t->takeAction(c.throttles->locos->address, s.substring(1));
       }
       break;
     }
     break;

   case 'S':
     // Second Throttle Command (effectively obsolete)
     // NOTE: Does not auto-create throttle (which it should!)
     switch(s.charAt(1)) {
     case 'L':
     case 'S':
       t = c.getThrottle('S');
       if (t != NULL) {
	 t->releaseLoco(s.substring(2));
	 t->getLoco(s.substring(2));
       }
       break;
     default:
       t = c.getThrottle('T');
       if (t != NULL) {
	 t->takeAction(c.throttles->locos->address, s.substring(1));
       }
       break;
     }
     break;

   case 'C': // Treat 'C' just like 'T'
     if (s.charAt(1) == 'T') {
       switch(s.charAt(2)) {
       case 'L':
       case 'S':
	t = c.getThrottle('S');
	 if (t != NULL) {
	   t->releaseLoco(s.substring(3));
	   t->getLoco(s.substring(3));
	 }
	 break;
       default:
	 t = c.getThrottle('T');
	 if (t != NULL) {
	   t->takeAction(c.throttles->locos->address, s.substring(2));
	 }
	 break;
       }
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
     // Roster - Not supported.
     break;
   case 'Q':
     // Quit - remove this client from the list?
     c.releaseAllThrottles();
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
  c.usingHeartbeat = true;
}

void WiThrottle::stopEKG(void) {
  // This needs to stop the timer somehow.
  c.usingHeartbeat = false;
}

/** 
 * WiThrottle::handleTrackPowerMsg()
 *
 * Turns power to the track on and off.
 *
 * Use SerialCommand::parse() to make sure we do
 * the same thing as the serial port code does.
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

 /**
  * WiThrottle::handleTurnoutMsg(String s)
  *
  * Triggers turnouts as needed.
  *
  * This directly calls the Turnout::activate() function for
  * "same as serial" functionality
  *
  * Note that the Turnout::activate() actually generates
  * an <a ...> serial message and calls SerialCommand::parse() itself.
  */
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

