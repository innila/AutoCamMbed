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

#ifdef CFG_ENABLE_RTOS
#undef WIZ_DBG
#define WIZ_DBG(x, ...)
#endif

//daniel
char g_asyncbuf[256];

// This function is operating in ISR. So you can't use debug message.
void WizFi310::recvData ( char c )
{
    static int cid, sub, len, count;
    static int is_mqtt_data = 0;
    char tbf[10];
    
    switch(_state.mode)
    {
        case MODE_COMMAND:
            switch(c)
            {
                case 0:
                case 0x0a:  // LF
                case 0x0d:  // CR
                break;

                case '{':
                _state.buf->flush();
                _state.mode = MODE_DATA_RX;
                sub = 0;
                break;

                default:
                _state.buf->flush();
                _state.buf->queue(c);
                _state.mode = MODE_CMDRESP;
                break;
            }
            break;

        case MODE_CMDRESP:
            switch(c)
            {
                case 0:
                    break;
                case 0x0a: // LF
                    break;
                case 0x0d: // CR
                    if (_flow == 2) setRts(false);      // block
                    _state.mode = MODE_COMMAND;
                    parseMessage();
                    if (_flow == 2) setRts(true);       // release
                    break;
                default:
                    _state.buf->queue(c);
                    break;
            }
            break;

        case MODE_DATA_RX:
            
            switch(sub)
            {
                case 0:
                // cid
                if( (c >= '0') && (c <= '9') )
                {
                    cid = x2i(c);
                }
                else if ( c == ',' )
                {
                    sub++;
                    count = 0;
                    len = 0;
                }
                //daniel add for mqtt
                else if ( c == 'Q' )
                {
                    cid = 0;
                    is_mqtt_data = 1;
                }
                //
                else
                {
                    _state.mode = MODE_COMMAND;
                }
                break;

            case 1:
                // ip                
//                if ((c >= '0' && c <= '9') || c == '.')
                if (((c >= '0' && c <= '9') || c == '.') && is_mqtt_data == 0 )
                {
                    _con[cid].ip[count] = c;
                    count++;
                }
                else if( c == ',' )
                {
                    _con[cid].ip[count] = '\0';
                    _con[cid].port = 0;
                    sub++;
                }
                //daniel for mqtt
                else if( is_mqtt_data == 1)
                {
                    rcvd_mqtt_topic[count] = c;
                    count++;
                }
//              else 
                else if( is_mqtt_data == 0 )
                {
                    _state.mode = MODE_COMMAND;
                }
                break;

            case 2:
                // port
                if ( c >= '0' && c <= '9' )
                {
                    _con[cid].port = (_con[cid].port * 10) + ( c - '0' );
                }
                else if( c == ',')
                {
                    sub++;
                    count = 0;
                }
                else
                {
                    _state.mode = MODE_COMMAND;
                }
                break;

            case 3:
                // data length
                if ( c >= '0' && c <= '9' )
                {
                    //_con[cid].recv_length = (_con[cid].recv_length * 10) + (c - '0');
                    len = (len * 10) + (c - '0');
                }
                else if( c == '}' )
                {
                    sub++;
                    count = 0;
                    _con[cid].recv_length = len;
                }
                else
                {
                    _state.mode = MODE_COMMAND;
                }
                break;

            default:

                if(_con[cid].buf != NULL)
                {
                    _con[cid].buf->queue(c);
                    if(_con[cid].buf->available() > CFG_DATA_SIZE - 16 )
                    {
                        setRts(false);     // blcok
                        _con[cid].received = true;
                        WIZ_WARN("buf full");
                    }
                }
                _con[cid].recv_length--;
                if(_con[cid].recv_length == 0)
                {
                     //WIZ_DBG("recv cid: %d, count : %d, len : %d",cid, count, len);
                     //sprintf(tbf, "recv cid: %d, count : %d, len : %d",cid, count, len);
                     //strcat(g_asyncbuf, tbf);
                    _con[cid].received = true;
                    _state.mode = MODE_COMMAND;
                }
                break;
            }
            break;
    }
}


//#define MSG_TABLE_NUM 6
//daniel
#define MSG_TABLE_NUM 8
#define RES_TABLE_NUM 7
int WizFi310::parseMessage () {
    int i;
    char buf[128];

    static const struct MSG_TABLE {
        const char msg[24];
        void (WizFi310::*func)(const char *);
    } msg_table[MSG_TABLE_NUM] = {
        {"[OK]",                    &WizFi310::msgOk},
        {"[ERROR]",                 &WizFi310::msgError},
        {"[ERROR:INVALIDINPUT]",    &WizFi310::msgError},
        {"[CONNECT ",               &WizFi310::msgConnect},
        {"[DISCONNECT ",            &WizFi310::msgDisconnect},
        {"[LISTEN ",                &WizFi310::msgListen},
        //daniel
        {"[MQTT CONNECT]",               &WizFi310::msgMQTTConnect},
        {"[MQTT DISCONNECT]",               &WizFi310::msgMQTTDisconnect},
    };
    static const struct RES_TABLE{
        const Response res;
        void (WizFi310::*func)(const char *);
    }res_table[RES_TABLE_NUM]={
        {RES_NULL,          NULL},
        {RES_MACADDRESS,    &WizFi310::resMacAddress},
//      {RES_WJOIN,         &WizFi310::resWJOIN},
        {RES_CONNECT,       &WizFi310::resConnect},
        {RES_SSEND,         &WizFi310::resSSEND},
        {RES_FDNS,          &WizFi310::resFDNS},
        {RES_SMGMT,         &WizFi310::resSMGMT},
        {RES_WSTATUS,       &WizFi310::resWSTATUS},
    };


    for( i=0; i<sizeof(buf); i++ )
    {
        if( _state.buf->dequeue(&buf[i]) == false ) break;
    }

    buf[i] = '\0';
    //strncpy(_state.dbgRespBuf, buf, sizeof(buf) );
    //WIZ_DBG("%s\r\n",_state.dbgRespBuf);

    if(_state.res != RES_NULL)
    {
        for( i=0; i<RES_TABLE_NUM; i++)
        {
            if(res_table[i].res == _state.res)
            {
                //WIZ_DBG("parse res %d '%s'\r\n", i, buf);
                if(res_table[i].func != NULL)
                {
                    (this->*(res_table[i].func))(buf);
                }

                if(res_table[i].res == RES_CONNECT && _state.n < 2)
                    return -1;
            }
        }
    }

    for( i=0; i<MSG_TABLE_NUM; i++)
    {
        if( strncmp(buf, msg_table[i].msg, strlen(msg_table[i].msg)) == 0 )
        {
            //WIZ_DBG("parse msg '%s'\r\n", buf);
            if(msg_table[i].func != NULL)
            {
                (this->*(msg_table[i].func))(buf);
            }
            return 0;
        }
    }

    return -1;
}


void WizFi310::msgOk (const char *buf)
{
    _state.ok = true;
}

void WizFi310::msgError (const char *buf)
{
    _state.failure = true;
}

void WizFi310::msgConnect (const char *buf)
{
    int cid;

    if (buf[9] < '0' || buf[9] > '8' || buf[10] != ']') return;

    cid = x2i(buf[9]);

    initCon(cid, true);
    _state.cid = cid;
    _con[cid].accept = true;
    _con[cid].parent = cid;
}

void WizFi310::msgDisconnect (const char *buf)
{
    int cid;

    if(buf[12] < '0' || buf[12] > '8' || buf[13] != ']')    return;

    cid = x2i(buf[12]);
    _con[cid].connected = false;
}


void WizFi310::msgMQTTConnect (const char *buf)
{
    int cid = 0;

    //if (buf[9] < '0' || buf[9] > '8' || buf[10] != ']') return;

    //cid = x2i(buf[9]);
    initCon(cid, true);
    _state.cid = cid;
    _con[cid].accept = true;
    _con[cid].parent = cid;
    
    _con[cid].connected = true;
    _state.res = RES_NULL;
    _state.ok = true;
}

void WizFi310::msgMQTTDisconnect (const char *buf)
{
    int cid = 0;

    //if(buf[12] < '0' || buf[12] > '8' || buf[13] != ']')    return;

    //cid = x2i(buf[12]);
    _con[cid].connected = false;
}


void WizFi310::msgListen (const char *buf)
{
    int cid;

    if(buf[8] < '0' || buf[8] > '8' || buf[9] != ']')   return;

    cid = x2i(buf[8]);
    _state.cid = cid;
}

void WizFi310::resMacAddress (const char *buf)
{
    if( buf[2] == ':' && buf[5] == ':')
    {
        strncpy(_state.mac, buf, sizeof(_state.mac));
        _state.mac[17] = 0;
        _state.res = RES_NULL;

        if(strncmp(_state.mac,CFG_DEFAULT_MAC,sizeof(CFG_DEFAULT_MAC)) == 0)
            _state.ok = false;
        _state.ok = true;
    }
}

void WizFi310::resConnect (const char *buf)
{
    int cid;

    if (buf[0] == '[' && buf[1] == 'O' && buf[2] == 'K' && buf[3] == ']')
    {
        _state.n++;
    }
    else if( buf[0] == '[' && buf[1] == 'C' && buf[2] == 'O' && buf[3] == 'N' &&
              buf[4] == 'N' && buf[5] == 'E' && buf[6] == 'C' && buf[7] == 'T')
    {
        cid = x2i(buf[9]);
        _state.cid = cid;
        _state.n++;
    }

    if(_state.n >= 2)
    {
        _state.res = RES_NULL;
        _state.ok = true;
    }
}

void WizFi310::resSSEND (const char *buf)
{
    if(_state.cid != -1)
    {
        _state.res = RES_NULL;
        _state.ok = true;
    }
}

void WizFi310::resFDNS (const char *buf)
{
    int i;

    for(i=0; i<strlen(buf); i++)
    {
        if( (buf[i] < '0' || buf[i] > '9') && buf[i] != '.' )
        {
            return;
        }
    }

    strncpy(_state.resolv, buf, sizeof(_state.resolv));
    _state.res = RES_NULL;
}

void WizFi310::resSMGMT (const char *buf)
{
    int cid, i;
    char *c;

    if( (buf[0] < '0' || buf[0] > '8') )    return;

    cid = x2i(buf[0]);
    if( cid != _state.cid )                 return;

    // IP
    c = (char*)(buf+6);
    for( i=0; i<16; i++ )
    {
        if( *(c+i) == ':')
        {
            _con[cid].ip[i] = '\0';
            i++;
            break;
        }
        if( ( *(c+i) < '0' || *(c+i) > '9') && *(c+i) != '.' )  return;
        _con[cid].ip[i] = *(c+i);
    }

    // Port
    c = (c+i);
    _con[cid].port = 0;
    for( i=0; i<5; i++ )
    {
        if( *(c+i) == '/')                  break;
        if( *(c+i) < '0' || *(c+i) > '9' )  return;

        _con[cid].port = (_con[cid].port * 10) + ( *(c+i) - '0' );
    }

    _state.res = RES_NULL;
}

void WizFi310::resWSTATUS (const char *buf)
{
    int idx=0,sep_cnt=0;
    int ip_idx=0,gw_idx=0;

    if(_state.n == 0)
    {
        _state.n++;
    }
    else if(_state.n == 1)
    {
        for(idx=0;buf[idx]!='\r';idx++)
        {
            if(buf[idx] =='/')
            {
                sep_cnt++;
                continue;
            }

            if( sep_cnt == 2)    // IP Address
            {
                _state.ip[ip_idx++] = buf[idx];
            }
            else if(sep_cnt == 3)
            {
                _state.gateway[gw_idx++] = buf[idx];
            }
        }
        _state.ip[ip_idx] = '\0';
        _state.gateway[gw_idx] = '\0';
        _state.res = RES_NULL;
    }
}
