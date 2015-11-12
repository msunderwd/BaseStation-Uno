/**********************************************************************

EthernetPort.cpp
COPYRIGHT (c) 2015 Mark Underwood

Part of DCC++ BASE STATION for the Arduino Uno by Gregg E. Berman

**********************************************************************/

//#include <DhcpV2_0.h>
#include <EthernetV2_0.h>
#include <EthernetClientV2_0.h>
#include <EthernetUdpV2_0.h>

#include "DCCpp_Uno.h"
#include "SerialCommand.h"
#include "Accessories.h"
#include "EthernetPort.h"
#include <SPI.h>
#if (USE_BONJOUR > 0)
#include <EthernetBonjourV2_0.h>
#endif

char commandString[MAX_COMMAND_LENGTH+1];

// TODO: Use real MAC Address
// This will have to be edited by the user to match their
// Ethernet shield's MAC Address.
// Candidate for a new serial interface command.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
// TODO: Replace this with DHCP setup if possible
// This would also have to be edited by the user.
// Or configured at runtime somehow.
IPAddress ip(192,168,2,27);
IPAddress myDns(192,168,2,1);
IPAddress gateway(192,168,2,1);
IPAddress subnet(255,255,255,0);
const int port_no = 1235; // Make this configurable?
//const int port_no = 23;

// Global variables
EthernetServer server(port_no);
EthernetClient client;
bool alreadyConnected = false;
int dhcp_loop_count = 0;

// TCP Message start flags.  These must come before
// the opening bracket of the DCC++ message.
const String SEND_START_FLAG = "SEND";
const String RECV_START_FLAG = "RECEIVE";

void EthernetPort::initialize() {
  // Disable SD
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  // Create the TCP port.
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

  // Start the server.
  dhcp_loop_count = 0;
  server.begin();
  Serial.print("My Address = ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print("."); 
  }
  Serial.println();

  // If Bonjour is enabled, register the service.
#if (USE_BONJOUR > 0)
  EthernetBonjour.begin("withrottle_server");
  
  int retv = EthernetBonjour.addServiceRecord("my application._telnet._tcp.", port_no, MDNSServiceTCP);
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
  // If Bonjouris enabled, we need to run it every loop.
  EthernetBonjour.run();
#endif  

  // Check to see if there is a client with data for us.
  client = server.available();
  String buffer = String("");
  
  // If so, process the message.
  if (client) {
    // Pull in all of the bytes and create
    // a DCC++ command from them.
    while (client.available() > 0) {
      char thisChar = client.read();
      Serial.print(thisChar);
      if (thisChar == '>') {

	// We found the end bracket. Process the message.
	buffer += thisChar;
	if (this->parse_dccpptcp(buffer)) {
	  // Valid message received. Forward it to serial parser
	  buffer.toCharArray(commandString, MAX_COMMAND_LENGTH+1);
	  Serial.println("Received:" + buffer);

	  // Use the SerialCommand parser to handle the actual message.
	  SerialCommand::parse(commandString);
	} 
	break;  // Get out of the while() loop.  The message is complete.
      } else {
	// No end bracket yet. Keep pulling in bytes.
	buffer += thisChar;
      }
    }
  }

  // If DHCP is enabled, we need to periodically poll the server.
  // This can be low bandwidth, thus the interval count before calling it.
#if (USE_DHCP > 0)
  dhcp_loop_count++;
  if (dhcp_loop_count == DHCP_POLL_INTERVAL) {
    // TODO: Handle DHCP failures more robustly than "not at all"
   Ethernet.maintain();
   dhcp_loop_count = 0;

  }
#endif
}

// Wrap a reply for TCP and forward it to the client.
void EthernetPort::sendReply(String buf) {
#if (USE_SERIAL_DEBUG > 0)
  Serial.println("TCP Reply Buf: " + buf);
#endif
  if (client != NULL) {
    client.println("RECEIVE" + buf);
  }
}

// Check for a valid TCP message.
bool EthernetPort::parse_dccpptcp(String&s) {
  char *retv;
  if (s.startsWith(SEND_START_FLAG) || s.startsWith(RECV_START_FLAG)) {
    s = s.substring(s.indexOf('<')+1, s.lastIndexOf('>'));
    return(true);
  } else {
    return(false);
  }
}

