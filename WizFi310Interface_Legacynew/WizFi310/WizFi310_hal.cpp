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

void WizFi310::setReset(bool flg)
{
    if( flg )
    {
        // low
        _reset.output();
        _reset = 0;
    }
    else
    {
        // high z
        _reset.input();
        _reset.mode(PullNone);
    }
}

void WizFi310::isrUart()
{
    char c;

    c = getUart();

    recvData(c);
    //S_UartPutc(c);
}

int WizFi310::getUart()
{
    return _wizfi.getc();
}

void WizFi310::putUart (char c)
{
    _wizfi.putc(c);
}

void WizFi310::setRts (bool flg)
{
    if (flg)
    {
        if(_flow == 2)
        {
            if(_rts)
            {
                _rts->write(0); // low
            }
        }
    }
    else
    {
        if(_flow == 2)
        {
            if(_rts)
            {
                _rts->write(1); // high
            }
        }
    }
}

int WizFi310::lockUart (int ms)
{
    Timer t;

    if(_state.mode != MODE_COMMAND)
    {
        t.start();
        while(_state.mode != MODE_COMMAND)
        {
            if(t.read_ms() >= ms)
            {
                  WIZ_WARN("lock timeout (%d)\r\n", _state.mode);
                return -1;
            }
        }
    }

#ifdef CFG_ENABLE_RTOS
    if (_mutexUart.lock(ms) != osOK) return -1;
#endif

    if(_flow == 2)
    {
        if(_cts && _cts->read())
        {
            // CTS check
            t.start();
            while (_cts->read())
            {
                if(t.read_ms() >= ms)
                {
                    WIZ_DBG("cts timeout\r\n");
                    return -1;
                }
            }
        }
    }

    setRts(false);      // blcok
    return 0;
}

void WizFi310::unlockUart()
{
    setRts(true);       // release
#ifdef CFG_ENABLE_RTOS
    _mutexUart.unlock();
#endif
}

void WizFi310::initUart (PinName cts, PinName rts, PinName alarm, int baud)
{
    _baud = baud;
    if (_baud) _wizfi.baud(_baud);

    _wizfi.attach(this, &WizFi310::isrUart, Serial::RxIrq);

    _cts = NULL;
    _rts = NULL;
    _flow = 0;

    if(cts != NC)
    {
        _cts = new DigitalIn(cts);
    }
    if(rts != NC)
    {
        _rts = new DigitalOut(rts);
        _flow = 2;
    }
}
