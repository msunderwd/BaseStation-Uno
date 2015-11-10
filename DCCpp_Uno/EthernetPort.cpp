/**********************************************************************

EthernetPort.cpp
COPYRIGHT (c) 2015 Mark Underwood

Part of DCC++ BASE STATION for the Arduino Uno by Gregg E. Berman

**********************************************************************/

#include "EthernetPort.h"

char commandString[MAX_COMMAND_LENGTH+1];

// TODO: Use real MAC Address
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
// TODO: Replace this with DHCP setup if possible
IPAddress ip(192,168,0,27);
IPAddress myDns(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
const int port_no = 1235; // Make this configurable?

EthernetServer server(port_no);
EthernetClient client;
bool alreadyConnected = false;

const String SEND_START_FLAG = "SEND";
const String RECV_START_FLAG = "RECEIVE";

void EthernetPort::initialize() {
  Ethernet.begin(mac, ip, dns, gateway, subnet);
  server.begin();
}

void EthernetPort::process(void) {
  client = server.available();
  String buffer = String("");
  
  if (client) {
    // Xmit welcome message on connect
    if (!alreadyConnected) {
      client.flush();
      client.println("DCC++ Arduino DCCppOverTCP Server");
      alreadyConnected = true;
    }

    // Receive
    while (client.available() > 0) {
      char thisChar = client.read();
      if (thisChar == '>') {
	buffer += thisChar;
	if (this->parse_dccpptcp(buffer)) {
	  // Valid message received. Forward it to serial parser
	  buffer.toCharArray(commandString, MAX_COMMAND_LENGTH+1);
	  SerialCommand::parse(commandString);
	}
	break;
      } else {
	buffer += thisChar;
      }
    }
  }
}

void EthernetPort::sendReply(String buf) {
  if (client && alreadyConnected) {
    client.println(buf);
  }
}

bool EthernetPort::parse_dccpptcp(String&s) {
  char *retv;
  if (s.startsWith(SEND_START_FLAG) || s.startsWith(RECV_START_FLAG)) {
      s = s.substring(s.indexOf('<'), s.lastIndexOf('>')+1);
      return(true);
  } else {
    return(false);
  }
}

