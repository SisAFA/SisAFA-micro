/*
* Description:
* Initial sketch for configuring MQTT message comunication
*
* Steps:
*        1. Turn the S1 switch to the Prog(right side)
*        2. Turn the S2 switch to the Arduino side(left side)
*        3. Set the UART select switch to middle one.
*        4. Upload the sketch to the Arduino board
*        5. Turn the S1 switch to the comm(left side)
*        6. RST the board
*/

#include <SoftwareSerial.h>
#include "GPS_SIM908.h"
#include <PubSubClient.h>
#include "SIM908Client.h"
#include "Timer.h"

#define ALARM_OFF 0
#define ALARM_ON 1
#define ALARM_BUZZ 2

int gpsLoopTime = 900000; //15 minutes
int alarmLoopTime = 60000; //1 minute

//Host Name  : matheusfonseca.me
byte server[] = { 107, 170, 177, 5 };
int  port = 1883;

//byte server[] = { 85, 119, 83, 194 };

char *apn = "zap.vivo.com.br";
char *usr = "vivo";
char *psw = "vivo";

/* Current State */
int curState = ALARM_OFF;

SIM908Client simClient(0,1,5,4,3);

PubSubClient mqttClient(server, port, callback, simClient);
// Callback function
void callback(char* topic, byte* payload, unsigned int length) {
  // In order to republish this payload, a copy must be made
  // as the orignal payload buffer will be overwritten whilst
  // constructing the PUBLISH packet.

  // Allocate the correct amount of memory for the payload copy
  byte* p = (byte*)malloc(length);
  // Copy the payload to the new buffer
  memcpy(p,payload,length);
  //mqttClient.publish("sisafa/sisafa_test/test", p, length);

  Serial.print("incomming: ");
  Serial.print(topic);
  Serial.print(" - ");
  Serial.print((char *)payload);
  Serial.println();

  // Free the memory
  free(p);
}

void setup()
{
  //start serial comunication at baud rate 9600
  //start shield in gsm mode
  simClient.begin(9600);
  //attaches GPRS network and creates a web connection
  simClient.attach(apn,usr,psw);

  //setup used message protocol
  //TODO: generalize for different types of protocols. ex.:
  // MQTT, HTTP, SMS
  if (mqttClient.connect("10129", "sisafa_test", "T5KIP1")) {
    if(!mqttClient.publish("sisafa/sisafa_test/test","lalalalala")){
      Serial.println("message not sent...");
    }
    else{
      Serial.println("message delivered");
    }
  }
  else{
    Serial.println("Connection not stabilished...");
  }
}

void loop()
{
  mqttClient.loop();

 Timer gpsTimer(gpsLoopTime);
 Timer alarmTimer(alarmLoopTime);

 switch(curState){
   case ALARM_OFF:{
     handleAlarmOff();
     break;
   }
   case ALARM_ON:{
     handleAlarmOn(gpsTimer);
     break;
   }
   case ALARM_BUZZ:{
     handleAlarmBuzz(alarmTimer);
     break;
   }
   default:{
     break;
   }
 }

}

//method for handling messages recieved from the web mosquitto
void msg(String topic, String payload, char * bytes, unsigned int length)
{
  Serial.print("incomming: ");
  Serial.print(topic);
  Serial.print(" - ");
  Serial.print(payload);
  Serial.println();

  // first char from payload converted from ASCII to int [0-9]
  int op = payload[0] - 48;

  switch(op){
    case ALARM_ON:{
      activateAlarm(120000);
      break;
    }
    case ALARM_OFF:{
      deactivateAlarm();
      break;
    }
    case ALARM_BUZZ:{
      buzzAlarm();
      break;
    }
    default:{
      //default case
    }
  }
}

//setting states
void activateAlarm(int dt)
{
  if(curState != ALARM_ON)
  {
    // deactivate buzz
    // deactivate ignition/fuel
    curState = ALARM_ON;
    gpsLoopTime = dt;
  }
}

void deactivateAlarm()
{
  if(curState == ALARM_ON)
  {
    // deactivate alarm
    // activate ignition/fuel
    curState = ALARM_OFF;
    gpsLoopTime = 900000;
  }
}

void buzzAlarm()
{
  if(curState != ALARM_OFF)
  {
    curState = ALARM_BUZZ;
    // activate buzz and lights
    // set gps loop interval to zero
    // send msg
  }
}

//handle alarm state
void handleAlarmOff()
{
  // wait for turn on signal
}

void handleAlarmOn(Timer gpsTimer)
{
  if(gpsTimer.expired())
  {
   wakeupGps(simClient);
   //  get GPS data
   Timer dataTimer(300000);
   double lat = 0;
   char latDir = 'I';
   double lon = 0;
   char lonDir = 'I';
   char *utc;

   char msgBuf[300];
   while(1)
   {
     utc = UTC();
     lat = latitude();
     latDir = lat_dir();
     lon = longitude();
     lonDir = lon_dir();
     altitude();
     if(dataTimer.expired())
     {
       break;
     }
   }
   //  build msg
    gpsTimer.countdown_ms(gpsLoopTime);
  }
}

void handleAlarmBuzz(Timer alarmTimer)
{
 if(alarmTimer.expired())
 {
   //toogle buzz
   alarmTimer.countdown_ms(alarmLoopTime);
 }
}

int wakeupGps(SIM908Client client)
{
  Serial.println("AT");
  delay(2000);
  //turn on GPS power supply
  Serial.println("AT+CGPSPWR=1");
  delay(1000);
  //reset GPS in autonomy mode
  Serial.println("AT+CGPSRST=1");
  delay(1000);
}
