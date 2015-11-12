/**********************************************************************

EthernetPort.cpp
COPYRIGHT (c) 2015 Mark Underwood

Part of DCC++ BASE STATION for the Arduino Uno by Gregg E. Berman

**********************************************************************/

#include "EthernetPort.h"
#include <SPI.h>
#if (USE_BONJOUR > 0)
#include <EthernetBonjourV2_0.h>
#endif

char commandString[MAX_COMMAND_LENGTH+1];

// TODO: Use real MAC Address
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
// TODO: Replace this with DHCP setup if possible
IPAddress ip(192,168,2,27);
IPAddress myDns(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
//const int port_no = 1235; // Make this configurable?
const int port_no = 80;

EthernetServer server(port_no);
EthernetClient client;
bool alreadyConnected = false;
int dhcp_loop_count = 0;

const String SEND_START_FLAG = "SEND";
const String RECV_START_FLAG = "RECEIVE";

void EthernetPort::initialize() {
  // Disable SD
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

#if (USE_DHCP > 0)
  Ethernet.begin(mac);
  #if (USE_SERIAL_DEBUG)
  Serial.println("Connected via DHCP."); 
  #endif
#else
  Ethernet.begin(mac, ip, dns, gateway, subnet);
  #if (USE_SERIAL_DEBUG)
  Serial.println("Connected via Static IP");
  #endif
#endif

  dhcp_loop_count = 0;
  server.begin();
  Serial.print("My Address = ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print("."); 
  }
  Serial.println();

#if (USE_BONJOUR > 0)
  EthernetBonjour.begin("withrottle_server");
  
  int retv = EthernetBonjour.addServiceRecord("DCCpp Base Station Server._withrottle._tcp.", port_no, MDNSServiceTCP);
  if (retv == 0) {
#if (USE_SERIAL_DEBUG)
    Serial.println("Bonjour Service Failed " + String(retv));
#endif
  } else {
  #if (USE_SERIAL_DEBUG)
  Serial.println("Bonjour Service Alive " + String(retv));
  #endif
  }
#endif
  
}

void EthernetPort::process(void) {

#if (USE_BONJOUR)
  EthernetBonjour.run();
#endif

  client = server.available();
  String buffer = String("");
  
  if (client) {
    //Serial.println("DCC++ Arduino DCCppOverTCP Server");
    //Serial.print("Local IP Address:");
    //Serial.println("Data Received");
    // Xmit welcome message on connect
    /*
    if (!alreadyConnected) {
      client.flush();
      Serial.println("DCC++ Arduino DCCppOverTCP Server");
      Serial.print("Local IP Address:");
      Serial.println(Ethernet.localIP());
      alreadyConnected = true;
    }
    */
    // Receive
    while (client.available() > 0) {
      char thisChar = client.read();
      Serial.print(thisChar);
      if (thisChar == '>') {
	buffer += thisChar;
	if (this->parse_dccpptcp(buffer)) {
	  // Valid message received. Forward it to serial parser
	  buffer.toCharArray(commandString, MAX_COMMAND_LENGTH+1);
	  Serial.println("Received:" + buffer);
	  SerialCommand::parse(commandString);
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

void EthernetPort::sendReply(String buf) {
  Serial.println("TCP Reply Buf: " + buf);
  if (client != NULL) {
    client.println("RECEIVE" + buf);
  }
}

bool EthernetPort::parse_dccpptcp(String&s) {
  char *retv;
  Serial.println("Parser: " + s);
  if (s.startsWith(SEND_START_FLAG) || s.startsWith(RECV_START_FLAG)) {
    s = s.substring(s.indexOf('<')+1, s.lastIndexOf('>'));
    return(true);
  } else {
    return(false);
  }
}

