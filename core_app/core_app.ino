/*
* Description:
* startial sketch for configuring MQTT message comunication
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

//state machine defstartion
#define ALARM_OFF  0
#define ALARM_ON   1
#define ALARM_BUZZ 2

/*================================ Analog pins =========================================*/
const int buzzerPin = A0; // buzzer Pin connected to Analog 1
const int z = A2; // Z pin connected to Analog 2
const int y = A3; // Y pin connected to Analog 3
const int x = A4; // X pin connected to Analog 4

/*================================ Digital pins ========================================*/
const int start = 8;         // Alarm button enabletion connected to Digital 10
const int interrupter = 9;   // Doors interrupters pin
const int fuel = 10;          //fuel Control
const int vibra = 13;         // checkVibrate sensor pin

/*============================= controll variables =====================================*/
int tolerance = 40;          // Sensitivity of the Alarm
boolean calibrate = false;   // When accelerometer is calibrated - changes to true
boolean shouldAlarm = false; // When motion is detected - changes to true
int curState = ALARM_OFF;    // Alarm machine state
boolean sendGpsData = false;     // need to send gps data

/*============================ Accelerometer limits ====================================*/
int xMin; //Minimum x Value
int xMax; //Maximum x Value
int xVal; //Current x Value
int yMin; //Minimum y Value
int yMax; //Maximum y Value
int yVal; //Current y Value
int zMin; //Minimum z Value
int zMax; //Maximum z Value
int zVal; //Current z Value

/*============================ Internet Connection =====================================*/
byte server[] = { 107, 170, 177, 5 }; // matheusfonseca.me
int  port = 1883;
char *apn = "zap.vivo.com.br";
char *usr = "vivo";
char *psw = "vivo";

/*=============================== MQTT Topics ==========================================*/
char *pwrTopic = "sisafa_test/power";
char *stsTopic = "sisafa_test/status";
char *gpsTopic = "sisafa_test/gps";

/*================================= Clients ============================================*/
SIM908Client simClient(0,1,5,4,3);
PubSubClient mqttClient(server, port, msg_callback, simClient);

void setup()
{
    setPins();    
    delay(500);
    setFuel(true);
    calibrateAccel();
    startModule();
}

void loop()
{
    mqttClient.loop();

    boolean enable = digitalRead(start);
     // If the button is pressed, start and recalibrate the Accelerometer limits.
     if(enable && !calibrate)
         activateAlarm();
     else if(!enable && calibrate)
         deactivateAlarm();

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
            handleAlarmBuzz();
            break;
        }
        default:{
            break;
        }
    }
}

/*====================== Handling Recieved Messages ==================================*/
void msg_callback(char* topic, byte* payload, unsigned int length)
{
    // first char from payload converted from ASCII to int [0-9]
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
          break;
        }
    }
}

/*=============================== Setting States =======================================*/
void activateAlarm()
{
    if(curState != ALARM_ON)
    {
        curState = ALARM_ON;
        // deactivate buzz
        // deactivate ignition/start
        digitalWrite(fuel, LOW);
        mqttClient.publish(stsTopic,"0");
        calibrateAccel();
        sendGpsData=false;
    }
}

void deactivateAlarm()
{
    calibrate = false;
    if(curState != ALARM_OFF)
    {
        curState = ALARM_OFF;
        digitalWrite(fuel, HIGH);
        mqttClient.publish(stsTopic,"1");
    }
}

void buzzAlarm()
{
    curState = ALARM_BUZZ;
    sendGpsData = true;
    mqttClient.publish(stsTopic,"2");
}

//handle alarm state
void handleAlarmOff()
{
    //wait for a signal to activate alarm
}

void handleAlarmOn()
{
    // Once the accelerometer is calibrated - check for movement
     if(checkMotion() || checkVibrate() || checkDoors()){
         buzzAlarm();
     }
}

boolean checkDoors()
{
    return digitalRead(interrupter);
}

void handleAlarmBuzz()
{
    alarm();
    sendGps();
}

//Function used to make the alarm sound, and blink the LED.
void alarm(){
    buzz(10,500,600);
}

void sendGps()
{
    if(sendGpsData) {
      tone(buzzerPin, 100);
      mqttClient.publish(gpsTopic,simClient.getGPS());
      sendGpsData = false;
      noTone(buzzerPin);
      delay(500);  
    }
}

//Function used to detect motion. Tolerance variable adjusts the sensitivity of movement detected.
boolean checkMotion(){
     boolean tempB=false;
     xVal = analogRead(x);
     yVal = analogRead(y);
     zVal = analogRead(z);

     if(xVal > xMax|| xVal < xMin){
         tempB=true;
     }

     if(yVal > yMax || yVal < yMin){
         tempB=true;
     }

     if(zVal > zMax || zVal < zMin){
         tempB=true;
     }
   return tempB;
}

boolean checkVibrate()
{
     return digitalRead(vibra);
}

//This is the function used to sound the buzzer
void buzz(int reps, int rate, int wait)
{
    for(int i=0; i<reps; i++){
      tone(buzzerPin, 10);
      delay(100);
      noTone(buzzerPin);
      delay(rate);
    }
    delay(wait);
}

/*============================= Setup Functions ======================================*/
void setPins()
{
  //Setup pins
    pinMode (start,INPUT);
    pinMode(vibra, INPUT);
    pinMode(interrupter, INPUT);
    pinMode(A1, OUTPUT);
    pinMode(fuel, OUTPUT);
    pinMode(11, OUTPUT);
    pinMode(12, OUTPUT);
   
    //all input pins should be initialized here
    digitalWrite(interrupter,LOW);
    digitalWrite(A1, LOW);
    digitalWrite(11, HIGH);
    digitalWrite(12, LOW);
}  

void setFuel(boolean op) 
{
   if(op) 
       digitalWrite(fuel,HIGH);
   else
       digitalWrite(fuel,LOW);
}

// Function used to calibrate the Accelerometer
void calibrateAccel(){
    // Calibration sequence startialisation sound - 3 seconds before calibration begins
    buzz(1,150,400);
    //calibrate the Accelerometer (should take about 0.5 seconds)
    for (int i=0; i<50; i++){
        // Calibrate X Values
        xVal = analogRead(x);
        xMax=xVal+tolerance;
        xMin=xVal-tolerance;

        //  Calibrate Y Values
        yVal = analogRead(y);
        yMax=yVal+tolerance;
        yMin=yVal-tolerance;

        // Calibrate Z Values
        zVal = analogRead(z);
        zMax=zVal+tolerance;
        zMin=zVal-tolerance;

        //Delay 10msec between readings
        delay(10);
    }
    //End of calibration sequence sound. ARMED.
    calibrate=true;
}

void startModule()
{
     boolean subscribed = false;
     while(!subscribed){
        //starting client with baud rate 9600
        simClient.begin(9600);

        //starting GPS module
        simClient.startGPS();

        //attaching GPRS network and creating a web connection
        simClient.attach(apn,usr,psw);

        //setup used message protocol
        if (mqttClient.connect("10k2D129", "sisafa_test", "T5KIP1")) {
            //when connected, must subscribe topic power
            subscribed = mqttClient.subscribe(pwrTopic);
        }
    }
    buzz(2,40,600);
}

