/*
 * MQTTClient.cpp
 *
 * Created on: Mai 25, 2015
 * Author: 256dpi
 * 
 * Description:
 * This file contains the implementation of MQTTClient.h definitions
 * 
 * 
 */

#include "MQTTClient.h"

void messageArrived(MQTT::MessageData& messageData) {
  MQTT::Message &message = messageData.message;
  // null terminate topic to create String object
  int len = messageData.topicName.lenstring.len;
  char topic[len+1];
  memcpy(topic, messageData.topicName.lenstring.data, (size_t)len);
  topic[len] = '\0';
  // null terminate payload
  char * payload = (char *)message.payload;
  payload[message.payloadlen] = '\0';
  messageReceived(String(topic), String(payload), (char*)message.payload, (unsigned int)message.payloadlen);
}
MQTTClient::MQTTClient(const char * _hostname, int _port, Client& _client) {
  this->client = new MQTT::Client<Network, Timer, MQTT_BUFFER_SIZE, 0>(this->network);
  this->network.setClient(&_client);
  this->client->setDefaultMessageHandler(messageArrived);
  this->hostname = _hostname;
  this->port = _port;
}
boolean MQTTClient::connect(const char * clientId) {
  return this->connect(clientId, "", "");
}
boolean MQTTClient::connect(const char * clientId, const char * username, const char * password) {
   if(!this->network.connect((char*)this->hostname, this->port)) {
    return false;
  }
  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
  data.clientID.cstring = (char*)clientId;
  if(username && password) {
    data.username.cstring = (char*)username;
    data.password.cstring = (char*)password;
  }
  return this->client->connect(data) == 0;
}
void MQTTClient::publish(String topic) {
this->publish(topic.c_str(), "");
}
void MQTTClient::publish(String topic, String payload) {
this->publish(topic.c_str(), payload.c_str());
}
void MQTTClient::publish(const char * topic, String payload) {
this->publish(topic, payload.c_str());
}
void MQTTClient::publish(const char * topic, const char * payload) {
MQTT::Message message;
message.qos = MQTT::QOS0;
message.retained = false;
message.dup = false;
message.payload = (char*)payload;
message.payloadlen = strlen(payload);
client->publish(topic, message);
}
void MQTTClient::subscribe(String topic) {
this->subscribe(topic.c_str());
}
void MQTTClient::subscribe(const char * topic) {
client->subscribe(topic, MQTT::QOS0, NULL);
}
void MQTTClient::unsubscribe(String topic) {
this->unsubscribe(topic.c_str());
}
void MQTTClient::unsubscribe(const char * topic) {
client->unsubscribe(topic);
}
void MQTTClient::loop() {
this->client->yield();
}
boolean MQTTClient::connected() {
return this->client->isConnected();
}
void MQTTClient::disconnect() {
this->client->disconnect();
}
