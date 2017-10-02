#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <string.h>
#include <list>
#include "json.hpp"
#include "MQTTClient.h"
#include <unistd.h>
#include <sstream>

#define ADDRESS     		"tcp://localhost:1883"
#define CLIENTID    		"ESP_AUTO_DELETE"
#define ESP_PUB_TOPIC   "esphome/from/ping"
#define ESP_SUB_TOPIC   "esphome/to/ping"
#define HOME_PUB_TOPIC  "homebridge/to/get"
#define HOME_SUB_TOPIC  "homebridge/from/response"

#define QOS         		1
#define TIMEOUT     		10000L
#define PING_WAIT				10
using namespace std;
using json = nlohmann::json;

volatile MQTTClient_deliveryToken deliveredtoken;

list<string> deviceToDelete;
int deviceIndex = 0;
void delivered(void *context, MQTTClient_deliveryToken dt)
{
    deliveredtoken = dt;
}
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char* payloadptr;
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    payloadptr = (char *) message->payload;
		cout << "message=" << payloadptr<< endl;

		string payloadString = string(payloadptr);	

		// Filter out unwanted characters		
		while(payloadString.find('') > 0){
			int loc = payloadString.find('');
			printf("CHARACTER IS FOUND at %d!\n", loc);
			if(loc > 0){
				payloadString.replace(payloadString.find(''),1,"");
			}else{
				break;
			}
		}

		auto j = json::parse(payloadString);

		if(!strcmp(topicName, &ESP_SUB_TOPIC[0])){ // If topic is ESP_SUB_TOPIC
			cout << "got message from ESP_SUB_TOPIC" << endl;
			string parsedName = j.at("name").get<std::string>();
			auto it = find(deviceToDelete.begin(), deviceToDelete.end(), parsedName);
			if(it != deviceToDelete.end()){
				cout << "erasing parsedName=" << parsedName << endl;
				deviceToDelete.erase(it);
			}	
		}else if(!strcmp(topicName, &HOME_SUB_TOPIC[0])){
			cout << "got message from HOME_SUB_TOPIC" << endl;
			
			for (json::iterator it = j.begin(); it != j.end(); ++it) {
			  std::cout << it.key() << "\n";
				deviceToDelete.push_front(it.key());
				++i;
			}	
		}else{
			cout << "Received unrecognized message from topic: " << topicName << endl;
		}	
		
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}
void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}
int main(int argc, char* argv[])
{
    const char* PAYLOAD = "{\"name\":\"*\"}";
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_deliveryToken token;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    int rc;
    int ch;
    MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    MQTTClient_subscribe(client, HOME_SUB_TOPIC, QOS);
    MQTTClient_subscribe(client, ESP_SUB_TOPIC, QOS);

    pubmsg.payload 		= (char *) PAYLOAD;
    pubmsg.payloadlen = strlen(PAYLOAD);
    pubmsg.qos 				= QOS;
    pubmsg.retained 	= 0;

		// Publish to HOME_PUB_TOPIC to get list of homebridge 
		// registered MQTT accessories


    MQTTClient_publishMessage(client, HOME_PUB_TOPIC, &pubmsg, &token);
    printf("Waiting for up to %d seconds for publication of %s\n"
            "on topic %s for client with ClientID: %s\n",
            (int)(TIMEOUT/1000), PAYLOAD, HOME_PUB_TOPIC, CLIENTID);
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    printf("Message with delivery token %d delivered\n", token);
    
		MQTTClient_publishMessage(client, ESP_PUB_TOPIC, &pubmsg, &token);
    printf("Waiting for up to %d seconds for publication of %s\n"
            "on topic %s for client with ClientID: %s\n",
            (int)(TIMEOUT/1000), PAYLOAD, HOME_PUB_TOPIC, CLIENTID);
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
		
		sleep(PING_WAIT);
		MQTTClient_unsubscribe(client,HOME_SUB_TOPIC);
		cout << "Final list for delete is " << endl;
		for(string s : deviceToDelete){
			//cout << s << endl;
    	ostringstream out;
			out << "{\"name\":\"" << s << "\"}";
			pubmsg.payload = (char *) out.str().c_str();
    	pubmsg.payloadlen = strlen(out.str().c_str());
    	pubmsg.qos = QOS;
    	pubmsg.retained = 0;
    	MQTTClient_publishMessage(client, "homebridge/to/remove", &pubmsg, &token);
		}

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}
