/*
 * SIM908Client.cpp
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
 * Copyright (c) 2015 Arthur Jahn
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

#include "SIM908Client.h"
#include <avr/pgmspace.h>

#define STATE_INACTIVE 0
#define STATE_ACTIVE 1
#define STATE_SETUP 2
#define STATE_IDLE 3
#define STATE_CONNECTED 4
#define STATE_CLOSED 5
#define STATE_EOT 6

SIM908Client::SIM908Client(uint8_t receivePin, uint8_t transmitPin, uint8_t powerPin, uint8_t gpsPin, uint8_t gsmPin) : _modem(receivePin, transmitPin), _pwrPin(powerPin), _gpsPin(gpsPin), _gsmPin(gsmPin), _state(STATE_INACTIVE)
{
    // set power pin to output to be able to set LOW and HIGH
    pinMode(powerPin, OUTPUT);
    pinMode(gpsPin, OUTPUT);
    pinMode(gsmPin, OUTPUT);
    _bufindex = _buflen = 0;
    _flowctrl = 1;
}

void SIM908Client::begin(int speed)
{
    uint8_t tries, res;
    unsigned long start;

    // begin software serial
    _modem.begin(speed);

    digitalWrite(_pwrPin, HIGH);
    delay(1200);
    digitalWrite(_pwrPin, LOW);
    delay(6000);
    //start shield in gsm mode.
    enableGsm();

    tries = 3;
    while ((_state == STATE_INACTIVE) && (tries-- > 0)) {
        if (sendAndAssert(F("AT"), F("OK"), 500, 10) == _S908_RECV_OK) {
            _state = STATE_SETUP;
        }
    }
    if (_state < STATE_ACTIVE)
        return;

    // baud rate setting
    tries = 3;
    do {
      //Set baud rate of the SoftwareSerial
      _modem.print(F("AT+IPR="));
      _modem.println(speed);
      _modem.flush();
      res = recvExpected(F("OK"), 1000);
    } while ((res != _S908_RECV_OK) && (--tries > 0));

    if (res != _S908_RECV_OK)
        return;

    // factory settings
    if (sendAndAssert(F("AT&F"), F("OK"), 1000, 3) != _S908_RECV_OK)
        return;

    // flow control
    if (sendAndAssert(F("AT+IFC=1,1"), F("OK"), 1000, 3) != _S908_RECV_OK)
        return;

    _state = STATE_SETUP;
}

uint8_t SIM908Client::pin(const char *pin)
{
    uint8_t res, tries;

    if (_state != STATE_SETUP)
        return 0;

    tries = 3;
    do {
        voidReadBuffer();

        _modem.print(F("AT+CPIN="));
        _modem.println(pin);
        _modem.flush();
        res = recvExpected(F("OK"), 5000);
    } while ((res != _S908_RECV_OK) && (--tries > 0));

    if (res != _S908_RECV_OK)
        return 0;
    return 1;
}

uint8_t SIM908Client::attach(const char *apn, const char *user, const char *pass)
{
  uint8_t tries, res;

  if (_state != STATE_SETUP)
      return 0;
  //verify if sim pin number is ok
  if (sendAndAssert(F("AT+CPIN?"), F("+CPIN: READY"), 1000, 3, 2000) != _S908_RECV_OK)
      return 0;
  //Close GPRS connection
  if (sendAndAssert(F("AT+CIPSHUT"), F("SHUT OK"), 1000, 3) != _S908_RECV_OK)
      return 0;
  //Start up Single IP connection mode
  if (sendAndAssert(F("AT+CIPMUX=0"), F("OK"), 1000, 3) != _S908_RECV_OK)
      return 0;
  //TCP IP non Transparent mode
  if (sendAndAssert(F("AT+CIPMODE=1"), F("OK"), 1000, 3, 5000) != _S908_RECV_OK)
      return 0;
  //Check network registration status
  if (sendAndAssert(F("AT+CREG=1"), F("OK"), 2000, 3, 5000) != _S908_RECV_OK)
      return 0;
  //Create GPRS PDP context
  if (sendAndAssert(F("AT+CGDCONT=1,\"IP\",\"zap.vivo.com.br\""), F("OK"), 1000, 3, 5000) != _S908_RECV_OK)
      return 0;
  //activate GPRS PDP context
  if (sendAndAssert(F("AT+CGACT=1,1"), F("OK"), 7000, 3, 1000) != _S908_RECV_OK)
      return 0;
  //attach to the GPRS service
  if (sendAndAssert(F("AT+CGATT=1"), F("OK"), 1000, 3, 5000) != _S908_RECV_OK)
      return 0;

  tries = 3;
  //Start task and set APN user name and password
  do {
      delay(5000);
      voidReadBuffer();
      _modem.print(F("AT+CSTT=\""));
      _modem.print(apn);
      _modem.print(F("\",\""));
      _modem.print(user);
      _modem.print(F("\",\""));
      _modem.print(pass);
      _modem.println(F("\""));
      _modem.flush();
      res = recvExpected(F("OK"), 1000);
  } while ((res != _S908_RECV_OK) && (--tries > 0));

  if (res != _S908_RECV_OK)
      return 0;

  //Bring up the GPRS connection
  if (sendAndAssert(F("AT+CIICR"), F("OK"), 10000, 1, 5000) != _S908_RECV_OK)
      return 0;
  delay(1000);
  //Verify if it has the local IP address
  if (sendAndAssert(F("AT+CIFSR"), F("ERROR"), 2000, 1, 1000) == _S908_RECV_OK)
      return 1;

  _state = STATE_IDLE;
  return 1;
}

int SIM908Client::connect(IPAddress ip, uint16_t port)
{
    uint8_t res, tries;

    if (_state != STATE_IDLE)
        return 0;

    tries = 1  ;
    do {
        voidReadBuffer();
        _modem.print(F("AT+CIPSTART=\"TCP\",\""));
        _modem.print(ip);
        _modem.print(F("\",\""));
        _modem.print(port);
        _modem.println(F("\""));
        _modem.flush();
        res = recvExpected(F("OK"), 1000);
        if (res != _S908_RECV_OK)
            continue;
        res = recvExpected(F("CONNECT"), 6000);
        delay(500);
    } while ((res != _S908_RECV_OK) && (--tries > 0));
      delay(1000);
    if (res == _S908_RECV_OK) {
        _state = STATE_CONNECTED;
        return 1;
    }
    return 0;
}

int SIM908Client::connect(const char *host, uint16_t port)
{
    uint8_t res, tries;

    if (_state != STATE_IDLE){
      return 0;
    }

    tries = 3;
    do {
        voidReadBuffer();
        _modem.print(F("AT+CIPSTART=\"TCP\",\""));
        _modem.print(host);
        _modem.print(F("\",\""));
        _modem.print(port);
        _modem.println(F("\""));
        _modem.flush();
        res = recvExpected(F("OK"), 1000);
        if (res != _S908_RECV_OK)
            continue;
        res = recvExpected(F("CONNECT"), 6000);
        delay(500);
    } while ((res != _S908_RECV_OK) && (--tries > 0));
    if (res == _S908_RECV_OK) {
        _state = STATE_CONNECTED;
        return 1;
    }
    return 0;
}

size_t SIM908Client::write(uint8_t w)
{
    if (_state != STATE_CONNECTED)
        return 0;
    return _modem.write(w);
}

size_t SIM908Client::write(const uint8_t *buf, size_t size)
{
    if (_state != STATE_CONNECTED)
        return 0;
    return _modem.write(buf,size);
}

void SIM908Client::flush()
{
    if (_state != STATE_CONNECTED)
        return;
    _modem.write((byte)0x1a);
    _modem.flush();
}

void SIM908Client::stop()
{
    if (_flowctrl == 0) {
        _flowctrl = 1;
        _modem.write(0x11);//XON
        _modem.flush();
    }
    if (_state == STATE_CONNECTED) {
        do {
            _buflen = 0;
        } while (fillBuffer());
        //if (_state == STATE_CONNECTED) {
          delay(1000);
          _modem.print(F("+++"));
          delay(500);
          sendAndAssert(F("AT+CIPCLOSE"), F("OK"), 1000, 3);
        //}
    }
    _buflen = _bufindex = 0;
    _flowctrl = 1;
    _state = STATE_IDLE;
}

void SIM908Client::enableGps(){
  digitalWrite(_gpsPin,LOW);//Enable GPS mode
  digitalWrite(_gsmPin,HIGH);//Disable GSM mode
}

void SIM908Client::enableGsm(){
  digitalWrite(_gsmPin,LOW);//Enable GSM mode
  digitalWrite(_gpsPin,HIGH);//Disable GPS mode
}

int SIM908Client::available()
{
    if ((_state < STATE_CONNECTED) || (_state == STATE_EOT))
        return -1;
    if (_buflen < 10)
        fillBuffer();
        return _buflen;
}

int SIM908Client::read() {
    int p = peek();
    if (p < 0)
        return p;

    --_buflen;
    return p;
}

int SIM908Client::read(uint8_t *buf, size_t size)
{
    int ret = 0, r;
    while ((r = read()) >= 0)
        buf[ret++] = r;
    return ret;
}

int SIM908Client::peek()
{
    size_t i;

    if ((_state < STATE_CONNECTED) || (_state == STATE_EOT))
        return -1;
    if ((_buflen < 10) && (fillBuffer() == 0))
        return -1;
    i = _S908_READ_BUFFER_SIZE + _bufindex - _buflen;

    if (i >= _S908_READ_BUFFER_SIZE)
        i -= _S908_READ_BUFFER_SIZE;

    return _buf[i];
}

uint8_t SIM908Client::connected()
{
    return ((_state >= STATE_CONNECTED) && (_state < STATE_EOT));
}

SIM908Client::operator bool()
{
    return _state >= STATE_SETUP;
}

void SIM908Client::voidReadBuffer()
{
    while (_modem.available())
        _modem.read();
}

uint8_t SIM908Client::sendAndAssert(const __FlashStringHelper* cmd, const __FlashStringHelper* exp, uint16_t timeout, uint8_t tries, uint16_t failDelay)
{
    uint8_t res;
    do {
        voidReadBuffer();
        _modem.println(cmd);
        _modem.flush();
        res = recvExpected(exp, timeout);
        if (res != _S908_RECV_OK)
            delay(failDelay);
    } while ((res != _S908_RECV_OK) && (--tries > 0));
    return res;
}

uint8_t SIM908Client::recvExpected(const __FlashStringHelper *exp, uint16_t timeout)
{
    uint8_t ret = _S908_RECV_RETURN;
    unsigned long start;
    const char PROGMEM *ptr = (const char PROGMEM*)exp;
    char c = pgm_read_byte(ptr), r;

    while ((ret != _S908_RECV_OK) && (ret != _S908_RECV_INVALID)) {
        start = millis();
        while ((!_modem.available()) && (millis()-start < timeout));
        if (!_modem.available()){
            return _S908_RECV_TIMEOUT;
        }
        r = _modem.read();

        switch (ret) {
        case _S908_RECV_RETURN:
            if (r == '\n')
                ret = _S908_RECV_READING;
            else if (r != '\r')
                ret = _S908_RECV_ECHO;
            break;
        case _S908_RECV_ECHO:
            if (r == '\n')
                ret = _S908_RECV_RETURN;
            break;
        case _S908_RECV_READING:
            if (r == c) {
                c = pgm_read_byte(++ptr);
                if (c == 0) {
                    ret = _S908_RECV_READ;
                }
            } else {
                ret = _S908_RECV_NO_MATCH;
            }
            break;
        case _S908_RECV_READ:
            if (r == '\n')
              ret = _S908_RECV_OK;
            break;
        case _S908_RECV_NO_MATCH:
            if (r == '\n')
              ret = _S908_RECV_INVALID;
            break;
        }
    }
    return ret;
}

size_t SIM908Client::fillBuffer()
{
    unsigned long start;

    if ((_buflen == 0) && (_state == STATE_CLOSED))
        _state = STATE_EOT;

    if (_state != STATE_CONNECTED)
        return _buflen;

    while (_buflen < _S908_READ_BUFFER_SIZE) {
        if (_modem.available() > _SS_MAX_RX_BUFF-16) {
            if (_buflen+8 == _S908_READ_BUFFER_SIZE) {
                _modem.write(0x13);
                _modem.flush();
                _flowctrl = 0;
            }
        } else if (_modem.available() < 8) {
            if (_flowctrl == 0) {
                _flowctrl = 1;
                _modem.write(0x11);//XON
                _modem.flush();
            }

            start = millis();
            while ((!_modem.available()) && (millis() - start < 50));
        }

        if (!_modem.available())
            break;

        _buf[_bufindex++] = _modem.read();
        if (_bufindex == _S908_READ_BUFFER_SIZE)
            _bufindex = 0;
        _buflen++;
    }

    if (detectClosed()) {
        if (_buflen)
            _state = STATE_CLOSED;
        else
            _state = STATE_EOT;
    }

    return _buflen;
}

uint8_t SIM908Client::detectClosed()
{
    const char PROGMEM *find = PSTR("\r\nCLOSED\r\n");
    const char PROGMEM *ptr;
    size_t len, i; char c;

    for (len = _buflen; len >= 10; --len) {
        i = _S908_READ_BUFFER_SIZE + _bufindex - len;
        if (i >= _S908_READ_BUFFER_SIZE)
            i -= _S908_READ_BUFFER_SIZE;
        ptr = find;
        while ((c = pgm_read_byte(ptr++)) > 0) {
            if (_buf[i++] != c)
                break;
            if (i == _S908_READ_BUFFER_SIZE)
                i = 0;
        }
        if (c > 0)
            continue;

        // cut the CLOSED off
        _buflen -= len;
        _bufindex += _S908_READ_BUFFER_SIZE - len;
        if (_bufindex >= _S908_READ_BUFFER_SIZE)
            _bufindex -= _S908_READ_BUFFER_SIZE;

        return 1;
    }
    return 0;
}
