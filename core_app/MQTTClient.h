/*
 * MQTTClient.h
 *
 * Created on: Mai 25, 2015
 * Author: 256dpi
 * 
 * Description:
 * This file specifyies the interface and methods needed for configuring 
 * a MQTT client, see MQTTClient.cpp for implementation
 * 
 * Limitations:
 * This library only supports QoS 0 (Quality of Service zero) 
 * This library needs a web client (i.e. EthernetClient, WifiClient) in 
 * order to send and receive data from the web.
 * 
 */

#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H
#ifndef MQTT_BUFFER_SIZE
#define MQTT_BUFFER_SIZE 128
#endif
#define MQTTCLIENT_QOS1 0
#define MQTTCLIENT_QOS2 0
#include <Arduino.h>
#include <Client.h>
#include <Stream.h>
#include <MQTTClient.h>
#include "Network.h"
#include "Timer.h"
void messageReceived(String topic, String payload, char * bytes, unsigned int length);
class MQTTClient {
private:
Network network;
MQTT::Client<Network, Timer, MQTT_BUFFER_SIZE, 0> * client;
const char * hostname;
int port;
public:
MQTTClient(const char * hostname, int port, Client& client);
boolean connect(const char * clientId);
boolean connect(const char * clientId, const char* username, const char* password);
void publish(String topic);
void publish(String topic, String payload);
void publish(const char * topic, String payload);
void publish(const char * topic, const char * payload);
void subscribe(String topic);
void subscribe(const char * topic);
void unsubscribe(String topic);
void unsubscribe(const char * topic);
void loop();
boolean connected();
void disconnect();
};
#endif

