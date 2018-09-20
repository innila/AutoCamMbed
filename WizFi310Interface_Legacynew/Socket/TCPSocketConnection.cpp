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

#include "TCPSocketConnection.h"
#include <algorithm>

TCPSocketConnection::TCPSocketConnection() {}

int TCPSocketConnection::connect(const char* host, const int port)
{
    if (set_address(host, port) != 0)   return -1;

    _server = false;
    _cid = _wizfi310->open(WizFi310::PROTO_TCP, get_address(), get_port());
    if (_cid < 0 )  return -1;

    return 0;
}

bool TCPSocketConnection::is_connected(void)
{
    bool _is_connected = _wizfi310->isConnected(_cid);
    if(!_is_connected)  _cid = -1;

    return _is_connected;
}

int TCPSocketConnection::send(char *data, int length)
{
    if (_cid < 0 || !is_connected())    return -1;

    return _wizfi310->send(_cid, data, length);
}

int TCPSocketConnection::send_all(char *data, int length)
{
    Timer tmr;
    int idx = 0;

    if(_cid <0 || !is_connected())  return -1;

    tmr.start();
    while((tmr.read_ms() < _timeout) || _blocking)
    {
        idx += _wizfi310->send(_cid, &data[idx], length - idx);
        if(idx < 0) return -1;

        if(idx == length)
            return idx;
    }
    return (idx == 0) ? -1 : idx;
}

int TCPSocketConnection::receive(char* data, int length)
{
    Timer tmr;
    int time = -1;
    int len = 0;

    if(_cid < 0 || !is_connected()) return -1;

    if(_blocking)
    {
        tmr.start();
        while(time < _timeout)
        {
			len = _wizfi310->readable(_cid);
			if(len == -1)
				return len;
			
			if(len > 0)
			{
				WIZ_DBG("receive readable : %d\r\n",len);
				break;
			}
            time = tmr.read_ms();
        }
        if(time >= _timeout)
        {
        	  //WIZ_DBG("receive timeout");
            return 0;
        }
    }

	return _wizfi310->recv(_cid, data, length);
}

int TCPSocketConnection::receive_all(char* data, int length)
{
    Timer tmr;
    int idx = 0;
    int time = -1;

    if(_cid < 0 || !is_connected()) return -1;

    tmr.start();

    while(time < _timeout || _blocking)
    {
        idx += _wizfi310->recv(_cid, &data[idx], length - idx);
        if (idx < 0)    return -1;

        if(idx == length)
            break;

        time = tmr.read_ms();
    }

    return (idx == 0) ? -1 : idx;
}

void TCPSocketConnection::acceptCID (int cid)
{
    char *ip;
    int port;
    _server = true;
    _cid = cid;


    if( _wizfi310->cmdSMGMT(cid) )  return;
    if( !_wizfi310->getRemote(_cid, &ip, &port))
    {
        set_address(ip, port);
    }
}
