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

#include "UDPSocket.h"

#include <string>
#include <algorithm>

UDPSocket::UDPSocket()
{
    endpoint_connected = false;
}

int UDPSocket::init(void)
{
    _server = false;
    return 0;
}

// Server initialization
int UDPSocket::bind(int port)
{
    if(port == 0)
    {
        _port = 5000;
        _server = false;
    }
    else
    {
        _port = port;
        _server = true;
    }
    
    return 0;
}

int UDPSocket::sendTo(Endpoint &remote, char *packet, int length)
{
    Timer tmr;
    int idx = 0;

    if (_cid < 0 && _wizfi310->isAssociated())
    {
        if (_server)
        {
            _cid = _wizfi310->listen(WizFi310::PROTO_UDP, _port);
        }
        else
        {
            _cid = _wizfi310->open(WizFi310::PROTO_UDP, remote.get_address(), remote.get_port(), _port);
        }
        if (_cid < 0)   return -1;
    }

    tmr.start();

    while ((tmr.read_ms() < _timeout) || _blocking)
    {
        if(_server)
        {
            idx += _wizfi310->sendto(_cid, packet, length, remote.get_address(), remote.get_port());
        }
        else
        {
            idx += _wizfi310->send(_cid, packet, length);
        }
        if (idx < 0)
        {
            if (!_wizfi310->isConnected(_cid)) _cid = -1;
        }

        if (idx == length)
            return idx;
    }
    return (idx == 0) ? -1 : idx;
}


int UDPSocket::receiveFrom(Endpoint &remote, char *buffer, int length)
{
    Timer tmr;
    int idx = 0;
    int time = -1;
    char ip[16];
    int port;

    if(_cid < 0 && _wizfi310->isAssociated())
    {
        // Socket open
        if (_server) {
            _cid = _wizfi310->listen(WizFi310::PROTO_UDP, _port);
            //WIZ_DBG("TEST CID : %d",_cid);
        }
        else
        {
            _cid = _wizfi310->open(WizFi310::PROTO_UDP, remote.get_address(), remote.get_port(), _port);
        }
        if (_cid < 0)   return -1;
    }

    if (_blocking)
    {
        tmr.start();
        while (time < _timeout + 20)
        {
            if(_wizfi310->readable(_cid))
            {
                break;
            }
            time = tmr.read_ms();
        }
        if (time >= _timeout + 20)
        {
            return -1;
        }
    }
    else
    {
        if(!_wizfi310->readable(_cid))
        {
            return 0;
        }
    }
   

    tmr.reset();
    time = -1;
    tmr.start();

    while (time < _timeout)
    {
        if(_server)
        {
            idx += _wizfi310->recvfrom(_cid, &buffer[idx], length - idx, ip, &port);
        }
        else
        {
            //idx += _wizfi250->recv(_cid, &buffer[idx], length - idx);
            idx += _wizfi310->recvfrom(_cid, &buffer[idx], length - idx, ip, &port);
        }

        if (idx == length)
            break;

        
        time = tmr.read_ms();
    }

    remote.set_address(ip, port);
    return (idx==0) ? -1 : idx;
}
