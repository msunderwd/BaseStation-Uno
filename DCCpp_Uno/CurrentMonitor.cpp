/**********************************************************************

CurrentMonitor.cpp
COPYRIGHT (c) 2013-2015 Gregg E. Berman

Part of DCC++ BASE STATION for the Arduino Uno 

**********************************************************************/

#include "DCCpp_Uno.h"
#include "CurrentMonitor.h"
#include "Utils.h"

///////////////////////////////////////////////////////////////////////////////

CurrentMonitor::CurrentMonitor(int pin, char *msg){
    this->pin=pin;
    this->msg=msg;
    current=0;
  } // CurrentMonitor::CurrentMonitor
  
boolean CurrentMonitor::checkTime(){
  if(millis()-sampleTime<CURRENT_SAMPLE_TIME)            // no need to check current yet
    return(false);
  sampleTime=millis();
  return(true);  
} // CurrentMonitor::checkTime
  
void CurrentMonitor::check(){
  current=analogRead(pin)*CURRENT_SAMPLE_SMOOTHING+current*(1.0-CURRENT_SAMPLE_SMOOTHING);        // compute new exponentially-smoothed current
  if(current>CURRENT_SAMPLE_MAX && digitalRead(SIGNAL_ENABLE_PIN_PROG)==HIGH){                    // current overload and Prog Signal is on (or could have checked Main Signal, since both are always on or off together)
    digitalWrite(SIGNAL_ENABLE_PIN_PROG,LOW);                                                     // disable both Motor Shield Channels
    digitalWrite(SIGNAL_ENABLE_PIN_MAIN,LOW);                                                     // regardless of which caused current overload
    reply_buffer = String(msg);
    sendReply(msg);
    //Serial.print(msg);                                                                            // print corresponding error message
  }    
} // CurrentMonitor::check  

long int CurrentMonitor::sampleTime=0;

