/**********************************************************************

Utils.h
COPYRIGHT (c) 2015 Mark Underwood

Part of DCC++ BASE STATION for the Arduino Uno by Gregg E. Berman

**********************************************************************/

#ifndef UTILS_H
#define UTILS_H

extern String reply_buffer;

void sendReply(String buf); // Send a reply message to ALL open ports.

#endif // UTILS_H
