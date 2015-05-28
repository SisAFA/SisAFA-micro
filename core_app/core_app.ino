// Product name: GPS/GPRS/GSM Module V3.0
// # Product SKU : TEL0051
// # Version     : 1.2

// # Description:
// # Initial sketch for configuring MQTT message comunication

// # Steps:
// #        1. Turn the S1 switch to the Prog(right side)
// #        2. Turn the S2 switch to the Arduino side(left side)
// #        3. Set the UART select switch to middle one.
// #        4. Upload the sketch to the Arduino board
// #        5. Turn the S1 switch to the comm(left side)
// #        6. RST the board

#include <SoftwareSerial.h>
#include "GPS_SIM908.h"
#include "MQTTClient.h"
#include "SIM908Client.h"


char *server = "matheusfonseca.me";
int port = 1883;

char *apn = "zap.vivo.com.br";
char *usr = "vivo";
char *psw = "vivo";


//cliente para o servidor matheusfonseca.me
SIM908Client client(0,1,5,4,3);

MQTTClient mqttClient(server,port,client);

bool notConnected = false;

void setup()
{
  //start serial comunication at baud rate 9600
  //start shield in gsm mode
  client.begin(9600);
  //attach GPRS network and creates a web connection
  client.attach(apn,usr,psw);
  if (mqttClient.connect("7", "sisafa_test", "T5KIP1")) {
//    Serial.println("connected!");
    mqttClient.subscribe("sisafa/sisafa_test/test");
  } else {
//    Serial.println("not connected!");
  }
}
void loop()
{
  mqttClient.loop();
}

void messageReceived(String topic, String payload, char * bytes, unsigned int length) {
Serial.print("incomming: ");
Serial.print(topic);
Serial.print(" - ");
Serial.print(payload);
Serial.println();
}
