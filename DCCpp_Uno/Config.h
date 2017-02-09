/**********************************************************************

Config.h
COPYRIGHT (c) 2013-2016 Gregg E. Berman

Part of DCC++ BASE STATION for the Arduino

**********************************************************************/

/////////////////////////////////////////////////////////////////////////////////////
//
// DEFINE MOTOR_SHIELD_TYPE ACCORDING TO THE FOLLOWING TABLE:
//
//  0 = ARDUINO MOTOR SHIELD          (MAX 18V/2A PER CHANNEL)
//  1 = POLOLU MC33926 MOTOR SHIELD   (MAX 28V/3A PER CHANNEL)

#define MOTOR_SHIELD_TYPE   1

/////////////////////////////////////////////////////////////////////////////////////
//
// DEFINE NUMBER OF MAIN TRACK REGISTER

#define MAX_MAIN_REGISTERS 12

/////////////////////////////////////////////////////////////////////////////////////
//
// DEFINE COMMUNICATIONS INTERFACE
//
//  0 = Built-in Serial Port
//  1 = Arduino.cc Ethernet/SD-Card Shield
//  2 = Arduino.org Ethernet/SD-Card Shield
//  3 = Seeed Studio Ethernet/SD-Card Shield W5200

#define COMM_INTERFACE   1

/////////////////////////////////////////////////////////////////////////////////////
//
// DEFINE STATIC IP ADDRESS *OR* COMMENT OUT TO USE DHCP
//

//#define IP_ADDRESS { 192, 168, 1, 200 }
//#define IP_ADDRESS { 192, 168, 0, 42 }

/////////////////////////////////////////////////////////////////////////////////////
//
// DEFINE PORT TO USE FOR ETHERNET COMMUNICATIONS INTERFACE
//

#define ETHERNET_PORT 2560

/////////////////////////////////////////////////////////////////////////////////////
//
// DEFINE MAC ADDRESS ARRAY FOR ETHERNET COMMUNICATIONS INTERFACE
//

#define MAC_ADDRESS {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEF }

/////////////////////////////////////////////////////////////////////////////////////
//
// ENABLE THE WITHROTTLE INTERFACE
//

#define WITHROTTLE_SUPPORT 1

/////////////////////////////////////////////////////////////////////////////////////
//
// ENABLE BONJOUR/ZEROCONF SUPPORT
//
// NOTE: You can set the name of your service here, but make sure it ends with
// "._withrottle" or the WiThrottle app won't be able to find it.
//
// (if you know about Bonjour/Zeroconf names, this means "leave off the ._tcp.local" part)

#define BONJOUR 0
#define BONJOUR_NAME "DCCpp._withrottle"

/////////////////////////////////////////////////////////////////////////////////////

