/**********************************************************************

Config.h
COPYRIGHT (c) 2013-2015 Gregg E. Berman

Part of DCC++ BASE STATION for the Arduino Uno 

**********************************************************************/

/////////////////////////////////////////////////////////////////////////////////////
//
// DEFINE MOTOR SHIELD PINS - SET MOTOR_SHIELD_TYPE ACCORDING TO THE FOLLOWING TABLE:
//
//  0 = ARDUINO MOTOR SHIELD          (MAX 18V/2A PER CHANNEL)
//  1 = POLOLU MC33926 MOTOR SHIELD   (MAX 28V/3A PER CHANNEL)

#define MOTOR_SHIELD_TYPE   0

/////////////////////////////////////////////////////////////////////////////////////
//
// DEFINE NUMBER OF MAIN TRACK REGISTER

#define MAX_MAIN_REGISTERS 12

/////////////////////////////////////////////////////////////////////////////////////
//
// DEFINE COMMUNICATIONS INTERFACE TYPE
//
//  0 = Built-in Serial Port
//  1 = Arduino Ethernet/SD Card Shield

#define COMM_TYPE   0

/////////////////////////////////////////////////////////////////////////////////////
//
// ENABLE WITHROTTLE INTERFACE
//
//  0 = Disable
//  1 = Enable

#define WITHROTTLE   1

/////////////////////////////////////////////////////////////////////////////////////
//
// DEFINE NAME OF ETHERNET LIBRARY TO INCLUDE (DIFFERENT SHIELDS MAY USE THEIR OWN LIBRARIES)
// *** ALSO MUST ADD THIS AS AN EXPLICIT INCLUDE FILE TO "DCCpp_Uno" ***

#define ETHERNET_LIBRARY  <EthernetV2_0.h>

/////////////////////////////////////////////////////////////////////////////////////


