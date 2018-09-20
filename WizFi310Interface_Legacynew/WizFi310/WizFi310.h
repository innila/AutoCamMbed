/*
 * Copyright (C) 2013 gsfan, MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/* Copyright (C) 2014 Wiznet, MIT License
 *  port to the Wiznet Module WizFi250
 */
/* Copyright (C) 2017 Wiznet, MIT License
 *  port to the Wiznet Module WizFi310
 */

#ifndef WIZFI310_H_
#define WIZFI310_H_

#include "WizFi310_conf.h"

#include "mbed.h"
#include "RawSerial.h"
#include "Serial.h"
#include "CBuffer.h"
#include <ctype.h>
#include <stdlib.h>
#include <string>

using namespace std;

#define BUF_SIZE 1600

//Debug is disabled by default
#if 1
#define WIZ_DBG(x, ...) std::printf("[WizFi310: DBG]" x "\r\n", ##__VA_ARGS__);
#define WIZ_WARN(x, ...) std::printf("[WizFi310: WARN]" x "\r\n", ##__VA_ARGS__);
#define WIZ_ERR(x, ...) std::printf("[WizFi310: ERR]" x "\r\n", ##__VA_ARGS__);
#define WIZ_INFO(x, ...) std::printf("[WizFi310: INFO]" x "\r\n", ##__VA_ARGS__);
#else
#define WIZ_DBG(x, ...)
#define WIZ_WARN(x, ...)
#define WIZ_ERR(x, ...)
#define WIZ_INFO(x, ...)
#endif


class WizFi310
{
public:

    enum AntennaMode{
        PCB = 0,
        UFL = 1,
        AUTO = 3,
    };

    enum WiFiMode {
        WM_STATION = 0,
        WM_AP = 1,
    };

    /** Wi-Fi security
     */
    enum Security {
        // kaizen need to change
        SEC_AUTO        = 0,
        SEC_OPEN        = 1,
        SEC_WEP         = 2,
        SEC_WPA_TKIP    = 3,
        SEC_WPA_AES     = 4,
        SEC_WPA2_AES    = 5,
        SEC_WPA2_TKIP   = 6,
        SEC_WPA2_MIXED  = 7,
    };

    /** TCP/IP protocol
     */
    enum Protocol {
        PROTO_UDP = 0,
        PROTO_TCP = 1,
    };

    /** Client/Server
     */
    enum Type {
        TYPE_CLIENT = 0,
        TYPE_SERVER = 1,
    };

    enum Response {
        RES_NULL,
        RES_MACADDRESS,
        RES_WJOIN,
        RES_CONNECT,
        RES_SSEND,
        RES_FDNS,
        RES_SMGMT,
        RES_WSTATUS,
        
    };

    enum Mode {
        MODE_COMMAND,
        MODE_CMDRESP,
        MODE_DATA_RX,
        MODE_DATA_RXUDP,
        MODE_DATA_RXUDP_BULK,
    };

    enum Status {
        STAT_NONE,
        STAT_READY,
        STAT_STANDBY,
        STAT_WAKEUP,
        STAT_DEEPSLEEP,
    };


    WizFi310 (PinName tx, PinName rx, PinName cts, PinName rts, PinName reset, PinName alarm = NC, int baud = 115200);

    // --------- WizFi250_at.cpp ---------
    void clearFlags     ();
    int  sendCommand    (const char * cmd, Response res = RES_NULL, int timeout = DEFAULT_WAIT_RESP_TIMEOUT, int opt = 1);

    int cmdAT       ();
    int cmdMECHO    (bool flg);
    int cmdUSET     (int baud, char *flow);
    int cmdMMAC     (const char *mac = NULL);
    int cmdWSET     (WiFiMode mode, const char *ssid, const char *bssid = NULL, int channel = 1);
    int cmdWANT     (AntennaMode mode);
    int cmdWNET     (bool is_dhcp);
    int cmdWSEC     (WiFiMode mode, const char *key, const char *sec = NULL);
    int cmdWJOIN    ();
    int cmdWLEAVE   ();
    int cmdWSTATUS  ();
    int cmdSCON     ( const char *openType, const char *socketType, int localPort, const char *dataMode = "0");
    int cmdSCON     ( const char *openType, const char *socketType, const char *remoteIp, int remotePort, int localPort = 0, const char *dataMode = "0");
    int cmdSSEND    ( const char *data, int cid, int sendSize, const char *remoteIp = NULL, int remotePort = 0, int Timeout = 2000 );
    int cmdCLOSE    ( int cid );
    int cmdFDNS     (const char *host);
    int cmdSMGMT    ( int cid );
    int cmdTEST     ();


    static WizFi310 * getInstance() {
        return _inst;
    };


    // --------- WizFi2550_sock.cpp ---------
    int getHostByName   (const char * host, char *ip);
    int open            (Protocol proto, const char *ip, int remotePort, int localPort = 0, void(*func)(int) = NULL);
    int listen          (Protocol proto, int port, void(*func)(int)=NULL);
    int close           (int cid);
    void initCon        (int cid, bool connected);
    int send            (int cid, const char *buf, int len);
    int sendto          (int cid, const char *buf, int len, const char *ip, int port);
    int recv            (int cid, char *buf, int len);
    int recvfrom        (int cid, char *buf, int len, char *ip, int *port);
    int readable        (int cid);
    bool isConnected    (int cid);
    int accept          (int cid);
    int getRemote       (int cid, char **ip, int *port);
        

protected:
    static WizFi310 * _inst;


    // Serial _wizfi
    RawSerial _wizfi;
    int _baud;
    DigitalIn *_cts;
    DigitalOut *_rts;
    int _flow;

    DigitalInOut _reset;

    struct STATE {
        WiFiMode wm;
        
        Security sec;
        char ssid[35];
        char pass[66];
        char ip[16];
        char netmask[16];
        char gateway[16];
        char nameserver[16];
        char mac[18];
        char resolv[16];
        char name[32];
        int rssi;
        bool dhcp;
        time_t time;

        bool initialized;
        bool associated;
        volatile Mode mode;
        volatile Status status;
        bool escape;
        volatile bool ok, failure;
        volatile Response res;
        int cid;
        int n;
        CircBuffer<char> *buf;
        char dbgRespBuf[BUF_SIZE];
        //char dummyBuf[BUF_SIZE];
    } _state;


public:
    struct CONNECTION {
        Protocol protocol;
        Type type;
        bool connected;
        char ip[16];
        int port;
        int send_length;
        int recv_length;
        CircBuffer<char> *buf;
        volatile bool received;
        volatile int parent;
        volatile bool accept;
        void(*func)(int);
    } _con[8];

    // --------- WizFi310.cpp ---------

    int join(WiFiMode mode);
    bool isAssociated();

    //int limitedap ();
    //int dissociate ();
    /*
    int disconnect () {
        return dissociate();
    }
    */

    Status getStatus ();

    int setMacAddress   (const char *mac);
    int getMacAddress   (char *mac);
    int setAddress      (const char *name = NULL);
    int setAddress      (const char *ip, const char *netmask, const char *gateway, const char *dns = NULL, const char *name = NULL);
    int getAddress      (char *ip, char *netmask, char *gateway);
    int setSsid         (const char *ssid);
    int setSec          (Security sec, const char *phrase);

    const char* getIPAddress    (void);
    const char* getMACAddress   (void);

    // --------- WizFi250_msg.cpp ---------
    void recvData       ( char c );
    int  parseMessage   ();
    void msgOk          (const char *buf);
    void msgError       (const char *buf);
    void msgConnect     (const char *buf);
    void msgDisconnect  (const char *buf);
    void msgListen      (const char *buf);
    //daniel
    void msgMQTTConnect (const char *buf);
    void msgMQTTDisconnect (const char *buf);

    void resMacAddress  (const char *buf);
    void resWJOIN       (const char *buf);
    void resConnect     (const char *buf);
    void resSSEND       (const char *buf);
    void resFDNS        (const char *buf);
    void resSMGMT       (const char *buf);
    void resWSTATUS     (const char *buf);


    // --------- WizFi250_hal.cpp ---------
    void setReset   (bool flg);
    void isrUart    ();
    int  getUart    ();
    void putUart    (char c);
    void setRts     (bool flg);
    int  lockUart   (int ms);
    void unlockUart ();
    void initUart   (PinName cts, PinName rts, PinName alarm, int baud);


    // --------- WizFi2550_util.cpp ---------
    int x2i     (char c);
    int i2x     (int i);

    // --------- WizFi250_ifc.cpp (For NetworkSocketAPI) ---------
    /**
    * Startup the WizFi310
    *
    * @return true only if WizFi310 was setup correctly
    */
    bool startup(void);

    /**
    * Reset WizFi310
    *
    * @return true only if WizFi310 resets successfully
    */
    bool reset(void);

    /**
    * Disconnect WizFi310 from AP
    *
    * @return true only if WizFi310 is disconnected successfully
    */
    bool disconnect(void);

    /**
    * Check if WizFi310 is conenected
    *
    * @return true only if the chip has an IP address
    */
    bool isConnected(void);
    
    
    //daniel for mqtt
    char rcvd_mqtt_topic[128];

};

#endif /* WIZFI250_H_ */
