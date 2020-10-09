/****************************************************************************************************************************
  AsyncHTTPRequest_Generic.h - Dead simple AsyncHTTPRequest for ESP8266, ESP32 and currently STM32 with built-in LAN8742A Ethernet
  
  For ESP8266, ESP32 and STM32 with built-in LAN8742A Ethernet (Nucleo-144, DISCOVERY, etc)
  
  AsyncHTTPRequest_STM32 is a library for the ESP8266, ESP32 and currently STM32 run built-in Ethernet WebServer
  
  Based on and modified from asyncHTTPrequest Library (https://github.com/boblemaire/asyncHTTPrequest)
  
  Built by Khoi Hoang https://github.com/khoih-prog/AsyncHTTPRequest_Generic
  Licensed under MIT license
  
  Copyright (C) <2018>  <Bob Lemaire, IoTaWatt, Inc.>
  This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License 
  as published bythe Free Software Foundation, either version 3 of the License, or (at your option) any later version.
  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
  You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.  
 
  Version: 1.0.0
  
  Version Modified By   Date      Comments
  ------- -----------  ---------- -----------
  1.0.0    K Hoang     14/09/2020 Initial coding to add support to STM32 using built-in Ethernet (Nucleo-144, DISCOVERY, etc).
 *****************************************************************************************************************************/
 
#pragma once

#define AsyncHTTPRequest_Generic_version   "1.0.0"

#include <Arduino.h>

#include "AsyncHTTPRequest_Debug.h"

#ifndef DEBUG_IOTA_PORT
  #define DEBUG_IOTA_PORT Serial
#endif

#ifdef DEBUG_IOTA_HTTP
  #define DEBUG_IOTA_HTTP_SET     true
#else
  #define DEBUG_IOTA_HTTP_SET     false
#endif

#if ESP32

  #include <AsyncTCP.h>
  
#elif ESP8266

  #include <ESPAsyncTCP.h>
  
#elif ( defined(STM32F0) || defined(STM32F1) || defined(STM32F2) || defined(STM32F3)  ||defined(STM32F4) || defined(STM32F7) || \
       defined(STM32L0) || defined(STM32L1) || defined(STM32L4) || defined(STM32H7)  ||defined(STM32G0) || defined(STM32G4) || \
       defined(STM32WB) || defined(STM32MP1) )
       
  #include "STM32AsyncTCP.h"
  
#endif

#include <pgmspace.h>
#include <utility/xbuf.h>

#define DEBUG_HTTP(format,...)  if(_debug){\
    DEBUG_IOTA_PORT.printf("Debug(%3ld): ", millis()-_requestStartTime);\
    DEBUG_IOTA_PORT.printf_P(PSTR(format),##__VA_ARGS__);}

#define DEFAULT_RX_TIMEOUT 3                    // Seconds for timeout

#define HTTPCODE_CONNECTION_REFUSED  (-1)
#define HTTPCODE_SEND_HEADER_FAILED  (-2)
#define HTTPCODE_SEND_PAYLOAD_FAILED (-3)
#define HTTPCODE_NOT_CONNECTED       (-4)
#define HTTPCODE_CONNECTION_LOST     (-5)
#define HTTPCODE_NO_STREAM           (-6)
#define HTTPCODE_NO_HTTP_SERVER      (-7)
#define HTTPCODE_TOO_LESS_RAM        (-8)
#define HTTPCODE_ENCODING            (-9)
#define HTTPCODE_STREAM_WRITE        (-10)
#define HTTPCODE_TIMEOUT             (-11)

enum class ReadyState
{
    Idle,              // Client created, open not yet called
    Unsent,            // open() has been called, not connected
    Opened,            // open() has been called, connected
    HdrsRecvd,         // send() called, response headers available
    Loading,           // receiving, partial data available
    Done               // Request complete, all data available.
};
    
class AsyncHTTPRequest
{
    using callback_arg_t = void*;

    struct header
    {
      header*   next;
      char*     name;
      char*     value;
      
      header(): next(nullptr), name(nullptr), value(nullptr)
      {};
      
      ~header() 
      {
        delete[] name;
        delete[] value;
        delete next;
      }
    };

    struct  URL 
    {
      char*   scheme;
      char*   user;
      char*   pwd;
      char*   host;
      int     port;
      char*   path;
      char*   query;
      char*   fragment;
      
      URL():  scheme(nullptr), user(nullptr), pwd(nullptr), host(nullptr),
              port(80), path(nullptr), query(nullptr), fragment(nullptr)
      {};
      
      ~URL() 
      {
        delete[] scheme;
        delete[] user;
        delete[] pwd;
        delete[] host;
        delete[] path;
        delete[] query;
        delete[] fragment;
      }
    };

    using readyStateChangeCB = std::function<void(callback_arg_t, AsyncHTTPRequest*, ReadyState readyState)>;
    using onDataCB = std::function<void(void*, AsyncHTTPRequest*, size_t available)>;

  public:
    AsyncHTTPRequest();
    ~AsyncHTTPRequest();


    //External functions in typical order of use:
    //__________________________________________________________________________________________________________*/
    void        setDebug(bool);                                         // Turn debug message on/off
    bool        debug() const;                                          // is debug on or off?

    bool        open(const char* method, const char* URL);              // Initiate a request
    void        onReadyStateChange(readyStateChangeCB, callback_arg_t arg = 0);  // Optional event handler for ready state change
    void        onReadyStateChangeArg(callback_arg_t arg = 0);                   // set event handlers arg
    // or you can simply poll readyState()
    void        setTimeout(int seconds);                                // overide default timeout (seconds)

    void        setReqHeader(const char* name, const char* value);      // add a request header
    void        setReqHeader(const char* name, int32_t value);          // overload to use integer value
    
#if (ESP32 || ESP8266)
    void        setReqHeader(const char* name, const __FlashStringHelper* value);
    void        setReqHeader(const __FlashStringHelper *name, const char* value);
    void        setReqHeader(const __FlashStringHelper *name, const __FlashStringHelper* value);
    void        setReqHeader(const __FlashStringHelper *name, int32_t value);
#endif

    bool        send();                                                 // Send the request (GET)
    bool        send(String body);                                      // Send the request (POST)
    bool        send(const char* body);                                 // Send the request (POST)
    bool        send(const uint8_t* buffer, size_t len);                // Send the request (POST) (binary data?)
    bool        send(xbuf* body, size_t len);                           // Send the request (POST) data in an xbuf
    void        abort();                                                // Abort the current operation

    ReadyState  readyState() const;                                     // Return the ready state

    int         respHeaderCount();                                      // Retrieve count of response headers
    char*       respHeaderName(int index);                              // Return header name by index
    char*       respHeaderValue(int index);                             // Return header value by index
    char*       respHeaderValue(const char* name);                      // Return header value by name
    
    bool        respHeaderExists(const char* name);                     // Does header exist by name?
    
#if (ESP32 || ESP8266)
    char*       respHeaderValue(const __FlashStringHelper *name);
    bool        respHeaderExists(const __FlashStringHelper *name);
#endif
    
    String      headers();                                              // Return all headers as String

    void        onData(onDataCB, void* arg = 0);                        // Notify when min data is available
    size_t      available() const;                                      // response available
    size_t      responseLength() const;                                 // indicated response length or sum of chunks to date
    int         responseHTTPcode() const;                               // HTTP response code or (negative) error code
    String      responseText();                                         // response (whole* or partial* as string)
    size_t      responseRead(uint8_t* buffer, size_t len);              // Read response into buffer
    uint32_t    elapsedTime() const;                                    // Elapsed time of in progress transaction or last completed (ms)
    String      version() const;                                        // Version of AsyncHTTPRequest
    //___________________________________________________________________________________________________________________________________

  private:

    enum    {HTTPmethodGET, HTTPmethodPOST} _HTTPmethod;

    ReadyState _readyState;

    int16_t         _HTTPcode;                  // HTTP response code or (negative) exception code
    bool            _chunked;                   // Processing chunked response
    bool            _debug;                     // Debug state
    uint32_t        _timeout;                   // Default or user overide RxTimeout in seconds
    uint32_t        _lastActivity;              // Time of last activity
    uint32_t        _requestStartTime;          // Time last open() issued
    uint32_t        _requestEndTime;            // Time of last disconnect
    URL*            _URL;                       // -> URL data structure
    char*           _connectedHost;             // Host when connected
    int             _connectedPort;             // Port when connected
    AsyncClient*    _client;                    // ESPAsyncTCP AsyncClient instance
    size_t          _contentLength;             // content-length header value or sum of chunk headers
    size_t          _contentRead;               // number of bytes retrieved by user since last open()
    readyStateChangeCB  _readyStateChangeCB;    // optional callback for readyState change
    callback_arg_t  _readyStateChangeCBarg;     // associated user argument
    onDataCB        _onDataCB;                  // optional callback when data received
    void*           _onDataCBarg;               // associated user argument

#ifdef ESP32
    SemaphoreHandle_t threadLock;
#endif

    // request and response String buffers and header list (same queue for request and response).

    xbuf*       _request;                       // Tx data buffer
    xbuf*       _response;                      // Rx data buffer for headers
    xbuf*       _chunks;                        // First stage for chunked response
    header*     _headers;                       // request or (readyState > readyStateHdrsRcvd) response headers

    // Protected functions

    header*     _addHeader(const char*, const char*);
    header*     _getHeader(const char*);
    header*     _getHeader(int);
    bool        _buildRequest();
    bool        _parseURL(const char*);
    bool        _parseURL(String);
    void        _processChunks();
    bool        _connect();
    size_t      _send();
    void        _setReadyState(ReadyState readyState);
    
#if (ESP32 || ESP8266)    
    char*       _charstar(const __FlashStringHelper *str);
#endif

    // callbacks

    void        _onConnect(AsyncClient*);
    void        _onDisconnect(AsyncClient*);
    void        _onData(void*, size_t);
    void        _onError(AsyncClient*, int8_t);
    void        _onPoll(AsyncClient*);
    bool        _collectHeaders();
};
