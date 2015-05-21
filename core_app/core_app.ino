// Product name: GPS/GPRS/GSM Module V3.0
// # Product SKU : TEL0051
// # Version     : 1.2
 
// # Description:
// # The sketch for driving the gps mode via the Arduino board
 
// # Steps:
// #        1. Turn the S1 switch to the Prog(right side)
// #        2. Turn the S2 switch to the Arduino side(left side)
// #        3. Set the UART select switch to middle one.
// #        4. Upload the sketch to the Arduino board
// #        5. Turn the S1 switch to the comm(left side) 
// #        6. RST the board 
 
// #        If you get 'inf' values, go outdoors and wait until it is connected.
// #        wiki link- http://www.dfrobot.com/wiki/index.php/GPS/GPRS/GSM_Module_V3.0_(SKU:TEL0051)
 
#include "gps_gsm_sim908.h"
#include "MQTTClient.h"
#include <GSM.h>


char *server = "matheusfonseca.me";
int port = 1883;

GSMClient client;
GPRS gprs;
GSM gsmAccess;

MQTTClient mqttClient(server,port,client);

bool notConnected = false;

int8_t sendATcommand(char* ATcommand, char* expected_answer, unsigned int timeout);

//TDODO fix-me, use the passed parameters
int connect_to_apn(char* apn, char* user, char* password);

void setup()
{
  pinMode(3,OUTPUT);//The default digital driver pins for the GSM and GPS mode
  pinMode(4,OUTPUT);
  pinMode(5,OUTPUT);
  digitalWrite(5,HIGH);
  delay(1500);
  digitalWrite(5,LOW);
  digitalWrite(3,LOW);//Enable GSM mode
  digitalWrite(4,HIGH);//Disable GPS mode
  delay(2000);
  Serial.begin(9600);
  while (sendATcommand("AT","OK",1000) == 0); 
  while (sendATcommand("AT+CREG?", "+CREG: 0,1", 2000) == 0);  

  connect_to_apn(NULL,NULL,NULL);

  while (sendATcommand("AT+SAPBR=1,1", "OK", 20000) == 0)
  {
      delay(5000);
  }
  //cliente para o servidor matheusfonseca.me 
  
   while(notConnected)
{
  if((gsmAccess.begin() == GSM_READY) &&
  (gprs.attachGPRS("zap.vivo.com.br", "vivo", "vivo") == GPRS_READY))
    notConnected = false;
  else
  {
    Serial.println("Not connected");
    delay(1000);
  }
}
  
  Serial.println("connecting...");
  if (mqttClient.connect("arduino", "sisafa_user", "T5KIP1")) {
    Serial.println("connected!");
    //client.subscribe("/another/topic");
    // client.unsubscribe("/another/topic");
  } else {
    Serial.println("not connected!");
  }
}
void loop()
{
   
}

int connect_to_apn(char* apn, char* user, char* password)
{
  sendATcommand("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", "OK", 2000);
  sendATcommand("AT+SAPBR=3,1,\"APN\",\"zap.vivo.com.br\"", "OK", 2000);
  sendATcommand("AT+SAPBR=3,1,\"USER\",\"vivo\"", "OK", 2000);
  sendATcommand("AT+SAPBR=3,1,\"PWD\",\"vivo\"", "OK", 2000);
}  

int8_t sendATcommand(char* ATcommand, char* expected_answer, unsigned int timeout)
{

    uint8_t x=0,  answer=0;
    char response[100];
    unsigned long previous;

    memset(response, '\0', 100);    // Initialize the string
    
    delay(100);
    
    while( Serial.available() > 0) Serial.read();    // Clean the input buffer
    
    Serial.println(ATcommand);    // Send the AT command 


    x = 0;
    previous = millis();

    // this loop waits for the answer
    do{
        if(Serial.available() != 0){    
            // if there are data in the UART input buffer, reads it and checks for the asnwer
            response[x] = Serial.read();
            x++;
            // check if the desired answer  is in the response of the module
            if (strstr(response, expected_answer) != NULL)    
            {
                answer = 1;
            }
        }
         // Waits for the asnwer with time out
    }while((answer == 0) && ((millis() - previous) < timeout));   

    return answer;
}

void messageReceived(String topic, String payload, char * bytes, unsigned int length) {
Serial.print("incomming: ");
Serial.print(topic);
Serial.print(" - ");
Serial.print(payload);
Serial.println();
}

