#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTAsync.h"


#define ADDRESS                     "ssl://smartfleet.sktelecom.com:9900"
#define CLIENTID                    "ExampleClientPub"
#define RPC_REQ_TOPIC               "v1/sensors/me/rpc/request/+"
#define SENDING_TOPIC               "v1/sensors/me/tre"
#define USERNAME                    "A123456789123456789D"
#define MICROTRIP_PAYLOAD           "{\"ty\":2,\"ts\":9999999999999,\"pld\":{\"tid\":1000,\"lon\":127.999999,\"lat\":37.510296,\"alt\":106,\"sp\":75,\"dop\":14,\"nos\":6,\"clt\":1507970678509}}"
#define TRIP_PAYLOAD                "{\"ty\":1,\"ts\":9999999999999,\"pld\":{\"tid\":1000,\"stt\":1511174279847,\"edt\":1511174337972,\"dis\":2559,\"tdis\":1023123,\"fc\":57355568,\"stlat\":37.509141,\"stlon\":127.063228,\"edlat\":37.520683,\"edlon\":127.06396,\"ctp\":100,\"coe\":1231,\"fct\":1923,\"hsts\":97,\"mesp\":43,\"idt\":440,\"btv\":9.3,\"gnv\":14.6,\"wut\":300,\"dtvt\":100}}"
#define RPC_RESP_PAYLOAD            "{\"results\" : 2000}"
#define QOS                         1
#define TIMEOUT                     10000L

volatile MQTTAsync_token deliveredtoken;

int disc_finished = 0;
int subscribed = 0;
int finished = 0;

void connlost(void *context, char *cause)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int rc;

	printf("\nConnection lost\n");
	if (cause)
		printf("     cause: %s\n", cause);

	printf("Reconnecting\n");
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		finished = 1;
	}
}




void onDisconnect(void* context, MQTTAsync_successData* response)
{
	printf("Successful disconnection\n");
	disc_finished = 1;
}


void onSubscribe(void* context, MQTTAsync_successData* response)
{
	printf("Subscribe succeeded\n");
	subscribed = 1;
}

void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Subscribe failed, rc %d\n", response ? response->code : 0);
	finished = 1;
}


void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Connect failed, rc %d\n", response ? response->code : 0);
	finished = 1;
}

void onSend(void* context, MQTTAsync_successData* response)
{
	printf("Message with token value %d delivery confirmed\n", response->token);
}


void onConnect(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

	int rc;

	printf("Successful connection to Smart[Fleet] Platform\nPress Q<Enter> to quit\n\n");

	printf("Subscribing to topic %s\n\n"
           ,RPC_REQ_TOPIC);
	opts.onSuccess = onSubscribe;
	opts.onFailure = onSubscribeFailure;
	opts.context = client;

	deliveredtoken = 0;

	if ((rc = MQTTAsync_subscribe(client, RPC_REQ_TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

    opts.onSuccess = onSend;


    /* Publish a microtrip message */

    pubmsg.payload = MICROTRIP_PAYLOAD;
    pubmsg.payloadlen = strlen(MICROTRIP_PAYLOAD);
    pubmsg.qos = 0;
    pubmsg.retained = 0;
    deliveredtoken = 0;

    if ((rc = MQTTAsync_sendMessage(client, SENDING_TOPIC, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start sendMessage, return code %d\n", rc);
		exit(EXIT_FAILURE);
	} else {
        printf("Sending a Microtrip Message to Smart[Fleet] %s on topic %s\n\n",
            MICROTRIP_PAYLOAD, SENDING_TOPIC);
    }

    /* Publish a trip message */
    
    pubmsg.payload = TRIP_PAYLOAD;
    pubmsg.payloadlen = strlen(TRIP_PAYLOAD);
    pubmsg.qos = 1;
    pubmsg.retained = 0;

    if ((rc = MQTTAsync_sendMessage(client, SENDING_TOPIC, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start sendMessage, return code %d\n", rc);
		exit(EXIT_FAILURE);
	} else {
        printf("Sending a Trip Message to Smart[Fleet] %s on topic %s\n\n",
            TRIP_PAYLOAD, SENDING_TOPIC);
    }
}


int msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    int i;
    char* payloadptr;
    char delemeterChar = '/';
    int rc;

    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = message->payload;
    for(i=0; i<message->payloadlen; i++)
    {
        putchar(*payloadptr++);
    }
    putchar('\n');

    char *ptr = strrchr(topicName, '/');
    char *topic = malloc(sizeof(char) * 70);

    strcpy(topic, "v1/sensors/me/rpc/response");

    strcat(topic, ptr);

    opts.onSuccess = onSend;
    opts.context = client;

    pubmsg.payload = RPC_RESP_PAYLOAD;
    pubmsg.payloadlen = strlen(RPC_RESP_PAYLOAD);
    pubmsg.qos = 1;
    pubmsg.retained = 0;

    if ((rc = MQTTAsync_sendMessage(client, topic, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start sendMessage, return code %d\n", rc);
		exit(EXIT_FAILURE);
	} else {
        printf("Sending a RPC Response Message to Smart[Fleet] %s on topic %s\n\n",
            RPC_RESP_PAYLOAD, topic);
    }

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

int main(int argc, char* argv[])
{
	MQTTAsync client;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;
	int rc;
	int ch;

	MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

	MQTTAsync_setCallbacks(client, client, connlost, msgarrvd, NULL);

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.context = client;
    conn_opts.username = USERNAME;

    ssl_opts.enableServerCertAuth = 1;
    conn_opts.ssl = &ssl_opts;

	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

	while	(!subscribed)
		#if defined(WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif

	if (finished)
		goto exit;

	do 
	{
		ch = getchar();
	} while (ch!='Q' && ch != 'q');

	disc_opts.onSuccess = onDisconnect;
	if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start disconnect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}
 	while	(!disc_finished)
		#if defined(WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif

exit:
	MQTTAsync_destroy(&client);
 	return rc;
}