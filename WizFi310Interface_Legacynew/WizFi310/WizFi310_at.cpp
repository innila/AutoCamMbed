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

#include "WizFi310.h"



void WizFi310::clearFlags()
{
    _state.ok = false;
    _state.failure = false;
    _state.res = RES_NULL;
    _state.n = 0;
}


int WizFi310::sendCommand(const char * cmd, Response res, int timeout, int opt)
{
    unsigned int i;
    Timer t;

    if (lockUart(timeout)) return -1;

    clearFlags();
    _state.res = res;

    WIZ_INFO("%s", cmd)

    for (i=0; i< strlen(cmd); i++)
    {
        putUart(cmd[i]);
    }

    if(opt == 1)
    {
        putUart('\r');
    }
    else if(opt == 2)
    {
        putUart('\r');
        putUart('\n');
    }
    unlockUart();

    if(timeout)
    {
        t.start();
        for(;;)
        {
            if (_state.ok && _state.res == RES_NULL){
                break;
            }
            
            if (_state.failure || t.read_ms() > timeout)
            {
                WIZ_WARN("failure of timeout[%d]ms\r\n",t.read_ms());
                _state.res = RES_NULL;
                t.stop();
                return -1;
            }
        }

        t.stop();
    }

    WIZ_INFO("[OK]\r\n");
    _state.res = RES_NULL;

    return 0;
}

int WizFi310::cmdAT()
{
    int resp;

    resp = sendCommand("AT");

    return resp;
}
int WizFi310::cmdTEST()
{
  return sendCommand("AT");
}

int WizFi310::cmdMECHO(bool flg)
{
    int status;
    char cmd[CFG_CMD_SIZE];

    sprintf(cmd,"AT+MECHO=%d",flg);
    status = sendCommand(cmd);

    return status;
}

int WizFi310::cmdUSET(int baud, char *flow)
{
    int status;
    char cmd[CFG_CMD_SIZE];

    sprintf(cmd,"AT+USET=%d,N,8,1,%s",baud, flow);
    status = sendCommand(cmd);

    if(status == 0)
    {
        wait(1);
        _state.buf->flush();
    }

    return status;
}

int WizFi310::cmdMMAC(const char *mac)
{
    int resp;
    char cmd[CFG_CMD_SIZE];

    if (mac)
    {
        sprintf(cmd, "AT+MMAC=%s",mac);
        resp = sendCommand(cmd);
    }
    else
    {
        sprintf(cmd, "AT+MMAC=?");
        resp = sendCommand(cmd, RES_MACADDRESS);
    }

    return resp;
}

int WizFi310::cmdWSET(WiFiMode mode, const char *ssid, const char *bssid, int channel)
{
    char cmd[CFG_CMD_SIZE];

    if(*bssid == NULL)
    {
        sprintf(cmd, "AT+WSET=%d,%s",mode, ssid);
    }
    else
    {
        sprintf(cmd, "AT+WSET=%d,%s,%s,%d",mode, ssid, bssid, channel);
    }

    return sendCommand(cmd);
}

int WizFi310::cmdWANT(AntennaMode mode)
{
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+WANT=%d",mode); 
    
    return sendCommand(cmd);
}

int WizFi310::cmdWNET(bool is_dhcp)
{
    char cmd[CFG_CMD_SIZE];

    if(is_dhcp == true)
    {
        sprintf(cmd, "AT+WNET=1");
    }
    else
    {
        sprintf(cmd, "AT+WNET=0,%s,%s,%s",_state.ip,_state.netmask,_state.gateway);
    }

    return sendCommand(cmd);
}

int WizFi310::cmdWSEC(WiFiMode mode, const char *key, const char *sec)
{
    char cmd[CFG_CMD_SIZE];

    if(*sec == NULL)
    {
        sprintf(cmd, "AT+WSEC=%d,,%s",mode, key);
    }
    else
    {
        sprintf(cmd, "AT+WSEC=%d,%s,%s",mode, sec, key);
    }

    return sendCommand(cmd);
}

int WizFi310::cmdWJOIN()
{
    //if( sendCommand("AT+WJOIN", RES_WJOIN, CFG_JOIN_TIMEOUT) )
    if( sendCommand("AT+WJOIN", RES_NULL, CFG_JOIN_TIMEOUT) )
    {
        WIZ_ERR("cmdWJOIN");
        return -1;
    }

    if( cmdWSTATUS() )
        return -1;

    WIZ_INFO("WizFi310 is successfully join to AP");

    return 0;
}

int WizFi310::cmdWLEAVE()
{
    return sendCommand("AT+WLEAVE");
}


int WizFi310::cmdWSTATUS()
{
    if( sendCommand("AT+WSTATUS", RES_WSTATUS, DEFAULT_WAIT_RESP_TIMEOUT) )
    {
        WIZ_ERR("cmdWSTATUS");
        return -1;
    }

    WIZ_INFO("IP      : %s", _state.ip);
    WIZ_INFO("Gateway : %s", _state.gateway);

    return 0;
}

int WizFi310::cmdSCON ( const char *openType, const char *socketType, int localPort, const char *dataMode)
{
    char cmd[CFG_CMD_SIZE];

    sprintf(cmd,"AT+SCON=%s,%s,,,%d,%s",openType, socketType, localPort, dataMode);
    return sendCommand(cmd);
}

int WizFi310::cmdSCON ( const char *openType, const char *socketType, const char *remoteIp, int remotePort, int localPort, const char *dataMode)
{
    int resp;
    char cmd[CFG_CMD_SIZE];

    if(localPort == 0)
        sprintf(cmd,"AT+SCON=%s,%s,%s,%d,%s,%s",openType, socketType, remoteIp, remotePort, "", dataMode);
    else
        sprintf(cmd,"AT+SCON=%s,%s,%s,%d,%d,%s",openType, socketType, remoteIp, remotePort, localPort, dataMode);

    resp = sendCommand(cmd, RES_CONNECT, 30000 );

    return resp;
}

int WizFi310::cmdSSEND ( const char *data, int cid, int sendSize, const char *remoteIp, int remotePort, int Timeout )
{
    int i, resp;
    Timer t;
    char cmd[CFG_CMD_SIZE];

    if (lockUart(Timeout))    return -1;

    clearFlags();
    if(remoteIp == NULL)
    {
        sprintf(cmd,"AT+SSEND=%d,,,%d",cid, sendSize);
    }
    else
    {
        sprintf(cmd,"AT+SSEND=%d,%s,%d,%d",cid, remoteIp, remotePort, sendSize);
    }

    _con[cid].send_length = sendSize;

    resp = sendCommand(cmd, RES_SSEND, 2000, 1);

    unlockUart();
    if(resp){
        WIZ_DBG("Fail cmdSSEND")
       return -1;
    }

    for(i=0; i<sendSize; i++)
    {
        putUart(data[i]);
    }
    unlockUart();

    if(Timeout)
    {
        t.start();
        for(;;)
        {
            if (_state.ok) break;
            if (_state.failure || t.read_ms() > Timeout)
            {
                WIZ_WARN("failure or timeout\r\n");
                return -1;
            }
        }
        t.stop();
    }

    wait(0.05);
    WIZ_INFO("%s\r\n",data);

    return i;
}


int WizFi310::cmdCLOSE ( int cid )
{
    char cmd[CFG_CMD_SIZE];

    sprintf(cmd,"AT+SMGMT=%d",cid);
    return sendCommand(cmd);
}


int WizFi310::cmdFDNS (const char *host)
{
    char cmd[CFG_CMD_SIZE];
    int resp;

    sprintf(cmd,"AT+FDNS=%s,3000",host);
    resp = sendCommand(cmd, RES_FDNS);

    WIZ_DBG("%s",_state.resolv);
    return resp;
}

int WizFi310::cmdSMGMT ( int cid )
{
    int resp;

    resp = sendCommand("AT+SMGMT=?", RES_SMGMT);
    return resp;
}
