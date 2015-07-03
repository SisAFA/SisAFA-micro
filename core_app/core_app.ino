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

#define ALARM_OFF  0
#define ALARM_ON   1
#define ALARM_BUZZ 2

#define MAX_TRYIES 3

//Movement detection
const int Ativar = 10; // Alarm button ativation connected to Digital 10
const int buzzerPin = A1; // buzzer Pin connected to Analog 1
const int interruptor1 = A0; // interruptor do sensor de porta para Analog 0
const int interruptor2 = A2; // interruptor do sensor de porta para Analog 2
const int vibra = 2;

//Accelerometer Pins
const int x = A3; // X pin connected to Analog 3
const int y = A4; // Y pin connected to Analog 4
const int z = A5; // Z pin connected to Analog 5

//Alarm LED
const int ledPin = 8; // LED connected to Digital 8
int tolerance=40; // Sensitivity of the Alarm
boolean calibrate = false; // When accelerometer is calibrated - changes to true
boolean shouldAlarm = false; // When motion is detected - changes to true

//Fuel Control
const int fuelPin = 6;

//Accelerometer limits
int xMin; //Minimum x Value
int xMax; //Maximum x Value
int xVal; //Current x Value
int yMin; //Minimum y Value
int yMax; //Maximum y Value
int yVal; //Current y Value
int zMin; //Minimum z Value
int zMax; //Maximum z Value
int zVal; //Current z Value

//Internet Connection
byte server[] = { 107, 170, 177, 5 }; // matheusfonseca.me
int  port = 1883;
char *apn = "zap.vivo.com.br";
char *usr = "vivo";
char *psw = "vivo";

//MQTT topics
char *pwrTopic = "sisafa_test/power";
char *statusTopic = "sisafa_test/status";
char *gpsTopic = "sisafa_test/gps";

//Clients
SIM908Client simClient(0,1,5,4,3);
PubSubClient mqttClient(server, port, msg_callback, simClient);

//control variables
int curState = ALARM_OFF;
int alarmLoopTime = 60000; //1 minute

void setup()
{

    //Set the LED Pin
    pinMode(ledPin, OUTPUT);
    pinMode (Ativar,INPUT);
    //Vibration
    pinMode(vibra, INPUT);
    //Set the Fuel pin
    pinMode(fuelPin, OUTPUT);

    delay(500);
    calibrateAccel();
    simClient.begin(9600);
    initModule();
}

void initModule()
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
}

void loop()
{
    mqttClient.loop();

    boolean ativa = digitalRead(Ativar);
     // If the button is pressed, initialise and recalibrate the Accelerometer limits.
     if(ativa && !calibrate)
     {
         activateAlarm();
     }else if(!ativa && calibrate)
     {
         calibrate = false;
         curState = ALARM_OFF;
         deactivateAlarm();
     }

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

//function for handling messages recieved from the server
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

//setting states
void activateAlarm()
{
    if(curState != ALARM_ON)
    {
        curState = ALARM_ON;
        // deactivate buzz
        // deactivate ignition/fuel
        digitalWrite(fuelPin, LOW);
        mqttClient.publish(statusTopic,"0");
        calibrateAccel();
    }
}

void deactivateAlarm()
{
    if(curState != ALARM_OFF)
    {
        curState = ALARM_OFF;
        // deactivate alarm
        // activate ignition/fuel
        digitalWrite(fuelPin, HIGH);
        mqttClient.publish(statusTopic,"1");
    }
}

void buzzAlarm()
{
    if(curState != ALARM_OFF)
    {
        curState = ALARM_BUZZ;
        mqttClient.publish(statusTopic,"2");
    }
}

//handle alarm state
void handleAlarmOff()
{
    //wait for a signal to activate alarm
}

void handleAlarmOn()
{
    // Once the accelerometer is calibrated - check for movement
     if(checkMotion() || vibration() || checkDoors()){
         buzzAlarm();
     }
}

boolean checkDoors()
{
     return (analogRead(interruptor1)>500
     || analogRead(interruptor2)>500);
}

void handleAlarmBuzz()
{
    alarm();
    sendGps();
}

//Function used to make the alarm sound, and blink the LED.
void alarm(){
    // sound the alarm and blink LED
    digitalWrite(ledPin, HIGH);
    buzz(10,500,600);
    digitalWrite(ledPin, LOW);
}

void sendGps()
{
    digitalWrite(ledPin, HIGH);
    tone(buzzerPin, 10);
    mqttClient.publish(gpsTopic,simClient.getGPS());
    noTone(buzzerPin);
    digitalWrite(ledPin, LOW);
}

// Function used to calibrate the Accelerometer
void calibrateAccel(){
    // Calibration sequence initialisation sound - 3 seconds before calibration begins
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
    buzz(2,40,600);
    calibrate=true;
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

boolean vibration()
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
