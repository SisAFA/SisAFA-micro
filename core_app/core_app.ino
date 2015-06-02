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
#include "MQTTClient.h"
#include "SIM908Client.h"

#define ALARM_OFF 0
#define ALARM_ON 1
#define ALARM_BUZZ 2

#define GPS_TIME_DFT 30000

int gpsLoopTime = 900000; //15 minutes
int alarmLoopTime = 30000; //30 seconds

//Host Name  : test.mosquitto.org
char *server = "85.119.83.194";
int port = 8080;

char *apn = "zap.vivo.com.br";
char *usr = "vivo";
char *psw = "vivo";

/* Current State */
int curState = 0;

SIM908Client client(0,1,5,4,3);

MQTTClient mqttClient(server,port,client);

void setup()
{
  //start serial comunication at baud rate 9600
  //start shield in gsm mode
  client.begin(9600);
  //attaches GPRS network and creates a web connection
  client.attach(apn,usr,psw);

  if (mqttClient.connect("7", "sisafa_test", "T5KIP1")) {
    mqttClient.subscribe("sisafa/sisafa_test/test");
  } else {

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
      handleAlarmBuzz();
      break;
    }
    default:{
      break;
    }
  }

}

//method for handling messages recieved from the web mosquitto
void messageReceived(String topic, String payload, char * bytes, unsigned int length)
{
  Serial.print("incomming: ");
  Serial.print(topic);
  Serial.print(" - ");
  Serial.print(payload);
  Serial.println();

  int op = payload[0] - 48;

  switch(op){
    case ALARM_ON:{
      activateAlarm();
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
    gpsLoopTime = GPS_TIME_DFT;
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
    //  reset default gps time
    //  build msg
     gpsTimer(gpsLoopTime);
   }
}

void handleAlarmBuzz(Timer alarmTimer)
{
  if(alarmTimer.expired())
  {
    //toogle buzz
    alarmTimer(alarmLoopTime);
  }
}
