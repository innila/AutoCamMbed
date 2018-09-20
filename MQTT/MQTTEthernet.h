
#if !defined(MQTTETHERNET_H)
#define MQTTETHERNET_H

#include "MQTTmbed.h"
#include "EthernetInterface.h"
#include "MQTTSocket.h"


uint8_t mac_addr[6] = {0x00, 0x08, 0xDC, 0x1E, 0x72, 0x1B}; // your mac address
const char * ip_addr = "222.98.173.249"; // your ip
const char * gw_addr = "222.98.173.254"; // your gateway
const char * snmask = "255.255.255.192"; // your subnetmask

class MQTTEthernet : public MQTTSocket
{
public:    
    MQTTEthernet()
    {
        wait(1);
        this->createSocket();
        //eth.init(mac_addr,ip_addr,snmask,gw_addr);                          // Do not use DHCP! If you use DHCP use "eth.init(mac_addr);".
        eth.init(mac_addr);
        eth.connect();
    }
    
    EthernetInterface& getEth()
    {
        return eth;
    }
    
    void reconnect()
    {
        eth.connect();  // nothing I've tried actually works to reconnect 
    }
    
private:

    EthernetInterface eth;
    
};


#endif
