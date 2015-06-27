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
#include <PubSubClient.h>
#include "SIM908Client.h"
#include "Timer.h"

#define ALARM_OFF  0
#define ALARM_ON   1
#define ALARM_BUZZ 2

#define MAX_TRYIES 3

int gpsLoopTime = 1200000;//20 minutes
int alarmLoopTime = 60000; //1 minute

//Host Name  : matheusfonseca.me
byte server[] = { 107, 170, 177, 5 };
int  port = 1883;

char *apn = "zap.vivo.com.br";
char *usr = "vivo";
char *psw = "vivo";

/* Current State */
int curState = ALARM_OFF;

char *pwrOpTopic = "sisafa/sisafa_test/test";

SIM908Client simClient(0,1,5,4,3);

PubSubClient mqttClient(server, port, msg_callback, simClient);

Timer gpsTimer(gpsLoopTime);
Timer alarmTimer(alarmLoopTime);

void setup()
{
    //starting client with baud rate 9600
    simClient.begin(9600);
    //starting GPS module
    simClient.startGPS();
    //attaching GPRS network and creating a web connection
    int res = simClient.attach(apn,usr,psw);
    //setup used message protocol
    if (mqttClient.connect("10k2D129", "sisafa_test", "T5KIP1")) {
        //when connected, must subscribe topic power_op
        mqttClient.subscribe(pwrOpTopic);
    }
    else{
      //restart shield
    }
}

void loop()
{
    mqttClient.loop();

    switch(curState){
        case ALARM_OFF:{
            handleAlarmOff();
            break;
        }
        case ALARM_ON:{
            handleAlarmOn();
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

//function for handling messages recieved from the server
void msg_callback(char* topic, byte* payload, unsigned int length)
{
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

void handleAlarmOn()
{
    if(gpsTimer.expired())
    {
        //  get GPS data
        Timer dataTimer(300000);
        double lat = 0;
        char latDir = 'I';
        double lon = 0;
        char lonDir = 'I';
        char *utc;

        char msgBuf[300];
        while(true)
        {
            simClient.getGPS();
            if(dataTimer.expired())
            {
                break;
            }
        }
        //  build msg
        sprintf(msgBuf,"lat:%lf\tldir:%c\nlon:%lf\tldir:%c\ntime:%s",lat,latDir,lon,lonDir,utc);
        gpsTimer.countdown_ms(gpsLoopTime);
    }
    else{
        delay(5000);
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
