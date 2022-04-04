#include<iostream>
#include<fstream>
#include "stdio.h"
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include "string.h"
#include <bits/stdc++.h> 

#include "stdlib.h"
#include <string>
#include "MQTTClient.h"
#include <wiringPi.h>

#define ADDRESS "tcp://192.168.222.70:1883"
#define CLIENTID "rpi2"
#define AUTHMETHOD "azaz"
#define AUTHTOKEN "mashwani10"
#define TOPIC "eee513/CPUTemp"
#define PAYLOAD "Hello World!"
#define QOS 2
#define TIMEOUT 10000L

using namespace std;
ofstream jsonfile;
volatile MQTTClient_deliveryToken deliveredtoken;
void delivered(void *context, MQTTClient_deliveryToken dt) {
printf("Message with token value %d delivery confirmed\n", dt);
deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
char* payloadptr;
string payloadmsg;

printf("Message arrived\n");
printf(" topic: %s\n", topicName);

printf(" message: ");
payloadptr = (char*) message->payload;

printf(payloadptr);
for (int i = 0; i <message->payloadlen; i++) {
        payloadmsg.push_back(*payloadptr++);
   }
printf("\n let see roll \n");

//string pitch = payloadmsg.substr(26,4);
string roll = payloadmsg.substr(39,4);
//cout<< pitch <<endl;
cout<< roll <<endl;

//float x = stof(pitch);
float y = stof(roll);

/*if (x>5.0) {
   digitalWrite(2, 0);
   digitalWrite(0, 1);
}
else if(x<-5.0) {
   digitalWrite(2, 1);
   digitalWrite(0, 0);
}
else{
   digitalWrite(2, 0);
   digitalWrite(0, 0);
}*/
if (y>5.0) {
   digitalWrite(4, 0);
   digitalWrite(5, 1);
}
else if(y<-5.0) {
   digitalWrite(4, 1);
   digitalWrite(5, 0);
}
else{
   digitalWrite(4, 0);
   digitalWrite(5, 0);
}
putchar('\n');
MQTTClient_freeMessage(&message);
MQTTClient_free(topicName);
return 1;
}
void connlost(void *context, char *cause) {
printf("\nConnection lost\n");
printf(" cause: %s\n", cause);
}
int main(int argc, char* argv[]) {
wiringPiSetup();			// Setup the library
pinMode(2, OUTPUT);
pinMode(0, OUTPUT);
pinMode(4, OUTPUT);
pinMode(5, OUTPUT);
MQTTClient client;
MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
int rc;
int ch;
MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
opts.keepAliveInterval = 20;
opts.cleansession = 1;
opts.username = AUTHMETHOD;
opts.password = AUTHTOKEN;
MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
printf("Failed to connect, return code %d\n", rc);
exit(-1);
}
printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
"Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
MQTTClient_subscribe(client, TOPIC, QOS);

do{

ch = getchar();

} while(ch!='Q' && ch != 'q');
MQTTClient_disconnect(client, 10000);
MQTTClient_destroy(&client);
return rc;
}
