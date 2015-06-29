/*
 * SIM908Client.h
 *
 * Created on: Mai 25, 2015
 * Author: Arthur Jahn
 * E-Mail: stutrzbecher@gmail.com
 *
 * Description:
 * This file contains the definitions of the SIM908Client, based
 * on arduino Client.h interface, by using AT commands suppported
 * by SIM908 SIMCOM chip, used for handling GPS/GSM connections.
 *
 * This library is based on Vincent Wochnik SIM900Client
 * available at: https://github.com/vwochnik/sim900client
 * E-Mail: v.wochnik@gmail.com
 * Copyright (c) 2013 Vincent Wochnik
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Vincent Wochnik  & Arthur Jahn
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef SIM908CLIENT_H
#define SIM908CLIENT_H

#include "Arduino.h"
#include "Client.h"
#include "WString.h"
#include <SoftwareSerial.h>

#define _S908_READ_BUFFER_SIZE 128

#define _S908_RECV_OK 0
#define _S908_RECV_TIMEOUT 1
#define _S908_RECV_RETURN 2
#define _S908_RECV_ECHO 3
#define _S908_RECV_READING 4
#define _S908_RECV_READ 5
#define _S908_RECV_NO_MATCH 6
#define _S908_RECV_INVALID 7

class SIM908Client : public Client
{
public:
    

    SIM908Client(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
    void begin(int);
    uint8_t pin(const char*);
    uint8_t attach(const char*, const char*, const char*);

    virtual int connect(IPAddress ip, uint16_t port);
    virtual int connect(const char *host, uint16_t port);
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *buf, size_t size);
    virtual int available();
    virtual int read();
    virtual int read(uint8_t *buf, size_t size);
    virtual int peek();
    virtual uint8_t connected();
    virtual void flush();
    virtual void stop();
    virtual void enableGsm();

    virtual void enableGps();
    virtual int8_t startGPS();
    virtual char* getGPS();
    virtual int8_t convert2Degrees(char* input);
    
    virtual operator bool();

protected:
    void voidReadBuffer();
    //sendAndAssert sends a AT command (first param), waits a response (second param), time to wait a response (third param)
    //  sets a number of tries (fourth param) and a time to wait before another try (fifth param)
    uint8_t sendAndAssert(const __FlashStringHelper*, const __FlashStringHelper*, uint16_t, uint8_t,uint16_t=0);
    uint8_t recvExpected(const __FlashStringHelper*, uint16_t);
    size_t fillBuffer();
    uint8_t detectClosed();

private:
    // serial connection to sim908 gprs
    SoftwareSerial _modem;
    // power up pin
    uint8_t _pwrPin;
    // gps module pin
    uint8_t _gpsPin;
    // gsm module pin
    uint8_t _gsmPin;
    // class state variable
    uint8_t _state;
    // software flow control state
    uint8_t _flowctrl;
    // circular buffer
    uint8_t _buf[_S908_READ_BUFFER_SIZE];
    // buffer index and length
    uint16_t _bufindex, _buflen;
};

#endif
