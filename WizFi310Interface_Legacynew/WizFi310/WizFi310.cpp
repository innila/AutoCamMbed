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

#include "mbed.h"
#include "WizFi310.h"

WizFi310 * WizFi310::_inst;


WizFi310::WizFi310(PinName tx,PinName rx,PinName cts, PinName rts,PinName reset, PinName alarm, int baud):
    _wizfi(tx,rx), _reset(reset)
{
    _inst = this;
    memset(&_state, 0, sizeof(_state));
    memset(&_con, 0, sizeof(_con));
    _state.initialized = false;
    _state.status = STAT_READY;
    _state.cid = -1;
    _state.buf = new CircBuffer<char>(CFG_DATA_SIZE);

    initUart(cts, rts, alarm, baud);
    _reset.output();

    setRts(true); // release

    /*
    wait_ms(500);
    cmdAT();
    cmdMECHO(false);
    if(cts != NC && rts != NC)
        cmdUSET(baud,"HW");
    else
        cmdUSET(baud,"N");

    // WizFi310 will restart by cmdUSET command.
    wait_ms(1000);
    cmdAT();
    */
}

int WizFi310::join(WiFiMode mode)
{
    char sec[10];

    if( cmdMMAC() ) return -1;

    if(mode == WM_AP)
        _state.wm = WM_AP;
    else
        _state.wm = WM_STATION;

    if ( cmdWNET(_state.dhcp) ) return -1;
    if ( cmdWSET(_state.wm, _state.ssid) ) return -1;


    switch (_state.sec)
    {
    case SEC_OPEN:
        strcpy(sec,"OPEN");
        break;
    case SEC_WEP:
        strcpy(sec,"WEP");
        break;
    case SEC_WPA_TKIP:
        strcpy(sec,"WPA");
        break;
    case SEC_WPA2_MIXED:
        strcpy(sec,"WPA2");
        break;
    default:
        strcpy(sec,"");
        break;
    }
        
    if ( cmdWSEC(_state.wm, _state.pass, sec) ) return -1;
    if ( cmdWJOIN() )   return -1;;
    _state.associated = true;

    return 0;
}

bool WizFi310::isAssociated()
{
    return _state.associated;
}

int WizFi310::setMacAddress (const char *mac)
{
    if (cmdMMAC(mac)) return -1;
    strncpy(_state.mac, mac, sizeof(_state.mac));
    return 0;
}

int WizFi310::getMacAddress (char *mac)
{
    if (cmdMMAC())  return -1;
    strcpy(mac, _state.mac);
    return 0;
}

int WizFi310::setAddress (const char *name)
{
    _state.dhcp = true;
    strncpy(_state.name, name, sizeof(_state.name));
    return 0;
}

int WizFi310::setAddress (const char *ip, const char *netmask, const char *gateway, const char *dns, const char *name)
{
    _state.dhcp = false;
    strncpy(_state.ip, ip, sizeof(_state.ip));
    strncpy(_state.netmask, netmask, sizeof(_state.netmask));
    strncpy(_state.gateway, gateway, sizeof(_state.gateway));
    strncpy(_state.nameserver, dns, sizeof(_state.nameserver));
    strncpy(_state.name, name, sizeof(_state.name));
    return 0;
}

int WizFi310::getAddress (char *ip, char *netmask, char *gateway)
{
    strcpy(ip, _state.ip);
    strcpy(netmask, _state.netmask);
    strcpy(gateway, _state.gateway);
    return 0;
}

int WizFi310::setSsid (const char *ssid)
{
    strncpy(_state.ssid, ssid, sizeof(_state.ssid));
    return 0;
}

int WizFi310::setSec ( Security sec, const char *phrase )
{
    _state.sec = sec;
    strncpy(_state.pass, phrase, strlen(phrase));
    return 0;
}

const char *WizFi310::getIPAddress(void)
{
    return _state.ip;
}

const char *WizFi310::getMACAddress(void)
{
    return _state.mac;
}

