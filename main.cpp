// PIR Motion Sensor Modlue : HC-SR501
#include "mbed.h"
#include "Servo.h"
#include "WizFi310Interface.h"
#include "MQTTClient.h"
#include "MQTTmbed.h"
#include "MQTTSocket.h"


#define SECURE WizFi310::SEC_WPA2_MIXED
#define SSID "CDI"
#define PASS "Cdi*1717"
//DigitalOut myled(D13);

#define USE_DHCP    1
//--------- Have to modify the mac address-------------
unsigned char MAC_Addr[6] = {0x00,0x08,0xDC,0x12,0x34,0x56};
#if defined(TARGET_WIZwiki_W7500)
    WizFi310Interface wizfi310(D1, D0, D7, D6, D8, NC, 115200);
    Serial pc(USBTX, USBRX);
#endif

Servo myservo(D5);
DigitalIn PIR[5] = {A1,A2,A3,A4,A5};  //D8

//turn servo anti clockwise
void turn_servo_anti(int turn_val)
{
    int i;
    for(i=0;i<turn_val;++i)
    {
        myservo = 1;
        wait_ms(370);
    }
    myservo = 0.5;
    wait(0.5);
}

//turn servo clockwise
void turn_servo_clock(int turn_val)
{
    int i;
    for(i=0;i<turn_val;++i)
    {
        myservo = 0;
       wait_ms(370);
    }
    myservo = 0.5;
    wait(0.5);
}

//on_message arrived from home/autocam/start
int start_pir = 6;
int arrivedcount = 0;
void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\n", message.qos, message.retained, message.dup, message.id);
    printf("Payload %.*s\n", message.payloadlen, (char*)message.payload);
    ++arrivedcount;
    char a[10];
    sprintf(a,"%.*s", message.payloadlen, (char*)message.payload);
    char pir = a[0];
    printf("Value: %c\n",pir);
    if(start_pir == 6)
    {
        if(pir == '0')
            start_pir = 0;
        else if(pir == '1')
            start_pir = 1;
        else if(pir == '2')
            start_pir = 2;
        else if(pir == '3')
            start_pir = 3;
        else if(pir == '4')
            start_pir = 4;
    }
    printf("Start PIR: %d\n",start_pir);
    wait(1);
    
}

//wizfi class to connect to mqtt
class MQTTWIZ: public MQTTSocket
{
public:    
    MQTTWIZ()
    {
        wait(1);
        this->createSocket();
    }
};


int main() 
{
    pc.baud(115200);
    printf("WizFi310  STATION. \r\n");
    wizfi310.init();
    printf("After Initialisation. \r\n");

    printf("After Set Address. \r\n");
    if ( wizfi310.connect(SECURE, SSID, PASS, WizFi310::WM_STATION))      return -1;
    printf("After Connect. \r\n");
    printf("IP Address is %s\r\n", wizfi310.getIPAddress()); 
    
    MQTTWIZ ipstack = MQTTWIZ();
    MQTT::Client<MQTTWIZ, Countdown> client = MQTT::Client<MQTTWIZ, Countdown>(ipstack);
    
    char* hostname = "172.16.73.4";
    int port = 1883;
    
    int rc = ipstack.connect(hostname, port);
    if (rc != 0)
        printf("rc from TCP connect is %d\n", rc);
    printf("rc from TCP connect is %d\n", rc);
    
    char MQTTClientID[30];
    
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       
    data.MQTTVersion = 3;
    sprintf(MQTTClientID,"WIZwiki-W7500-client-%d",rand()%1000);
    data.clientID.cstring = MQTTClientID;
    data.username.cstring = "testuser";
    data.password.cstring = "testpassword";
    char topic[20] = "home/autocam/start"; 

    if ((rc = client.connect(data)) != 0)
        printf("rc from MQTT connect is %d\n", rc);
    if ((rc = client.subscribe(topic, MQTT::QOS2, messageArrived)) != 0)
            pc.printf("rc from MQTT subscribe is %d\r\n", rc);
    while(1)
    {
        if(start_pir==6)
        {
            client.yield(1000);
            wait(1);
        }
        else
        {
            if ((rc = client.unsubscribe(topic)) != 0)
                pc.printf("rc from unsubscribe was %d\r\n", rc);
            MQTT::Message message;
            message.qos = MQTT::QOS0;
            message.retained = false;
            message.dup = false;
            char buf[100];
            sprintf(buf, "%s","Recieved");
            message.payload = (void*)buf;
            message.payloadlen = strlen(buf);
            rc = client.publish("home/autocam/recieve", message);
            while(rc!=0)
                rc = client.publish("home/autocam/recieve", message);
            break;
        }
    }
        
    MQTT::Message message;
    char buf[100];
     message.qos = MQTT::QOS0;
     message.retained = false;
     message.dup = false;
    pc.printf("Hello WizWIki-W7500!\n\r");
    pc.printf("===========================================\n\r");
    int last_PIR=start_pir,val;
    pc.printf("\nStarting PIR Calibration");
    wait(25);
    pc.printf("\nCalibration Completed");
    while(1) {
        for(int p=0;p<5;p++)
        {
            float pir_val = PIR[p];
            if((PIR[p]==1) && (last_PIR!=p))
            {
                pc.printf("Last PIR: %d ",last_PIR);
                pc.printf("Sensor number : %d ",p,pir_val);
                sprintf(buf, "%d",p);
                message.payload = (void*)buf;
                message.payloadlen = strlen(buf);
                rc = client.publish("home/autocam",message);
                printf("home/sensor : %d\r\n",message.payload);
                if(p > last_PIR)
                {
                    val = p - last_PIR;
                    pc.printf("Anti - %d ",val);
                    turn_servo_anti(val);
                }
                else if(last_PIR > p)
                {
                    val = last_PIR - p;
                    pc.printf("Clock - %d ",val);
                    turn_servo_clock(val);
                }
                last_PIR = p;
                printf("\n");
            }
        }
    }
}