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
#include <WString.h>

#include <include/tl/optional.hpp>

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

namespace {
//! Helper utility to convert a ReadyState into a string
//! String library agnostic, can be used with std::string or Arduino's WString
template<typename T>
T toString(ReadyState state)
{
    switch (state)
    {
    case ReadyState::Idle:      return "Idle";
    case ReadyState::Unsent:    return "Unsent";
    case ReadyState::Opened:    return "Opened";
    case ReadyState::HdrsRecvd: return "HdrsRecvd";
    case ReadyState::Loading:   return "Loading";
    case ReadyState::Done:      return "Done";
    }

    return T{"Unknown ReadyState("} + int(state) + ')';
}
}

enum class HTTPmethod
{
    GET,
    POST
};

namespace {
//! Helper utility to convert a HTTPmethod into a string
//! String library agnostic, can be used with std::string or Arduino's WString
template<typename T>
T toString(HTTPmethod method)
{
    switch (method)
    {
    case HTTPmethod::GET:  return "GET";
    case HTTPmethod::POST: return "POST";
    }

    return T{"Unknown HTTPmethod("} + int(method) + ')';
}
}

struct  URL
{
  String   scheme;
  String   user;
  String   pwd;
  String   host;
  int     port;
  String   path;
  String   query;
  String   fragment;

  String toString() const
  {
      return scheme + user + pwd + host + ':' + port + path + query + fragment;
  }
};

tl::optional<URL> parseURL(const String &url);

class AsyncHTTPRequest
{
    using callback_arg_t = void*;

    struct header
    {
      header*   next{};
      String     name;
      String     value;
      
      ~header() 
      {
        delete next;
      }
    };

    using readyStateChangeCB = std::function<void(callback_arg_t, AsyncHTTPRequest*, ReadyState readyState)>;
    using onDataCB = std::function<void(void*, AsyncHTTPRequest*, size_t available)>;    

  public:
    ~AsyncHTTPRequest();


    //External functions in typical order of use:
    //__________________________________________________________________________________________________________*/
    void        setDebug(bool);                                         // Turn debug message on/off
    bool        debug() const;                                          // is debug on or off?

    bool        open(const URL &url, HTTPmethod method = HTTPmethod::GET); // Initiate a request
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
    String      respHeaderName(int index);                              // Return header name by index
    String      respHeaderValue(int index);                             // Return header value by index
    String      respHeaderValue(const String &name);                    // Return header value by name
    
    bool        respHeaderExists(const String &name);                   // Does header exist by name?
    
#if (ESP32 || ESP8266)
    String      respHeaderValue(const __FlashStringHelper *name);
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
    HTTPmethod _HTTPmethod;

    ReadyState _readyState{ReadyState::Idle};

    int16_t         _HTTPcode{0};                 // HTTP response code or (negative) exception code
    bool            _chunked{false};              // Processing chunked response
    bool            _debug{DEBUG_IOTA_HTTP_SET};  // Debug state
    uint32_t        _timeout{DEFAULT_RX_TIMEOUT}; // Default or user overide RxTimeout in seconds
    uint32_t        _lastActivity{0};             // Time of last activity
    uint32_t        _requestStartTime{0};         // Time last open() issued
    uint32_t        _requestEndTime{0};           // Time of last disconnect
    URL             _URL{};                       // -> URL data structure
    String          _connectedHost;               // Host when connected
    int             _connectedPort{-1};           // Port when connected
    AsyncClient*    _client{nullptr};             // ESPAsyncTCP AsyncClient instance
    size_t          _contentLength{0};            // content-length header value or sum of chunk headers
    size_t          _contentRead{0};              // number of bytes retrieved by user since last open()
    readyStateChangeCB _readyStateChangeCB{};     // optional callback for readyState change
    callback_arg_t  _readyStateChangeCBarg{};     // associated user argument
    onDataCB        _onDataCB{nullptr};           // optional callback when data received
    void*           _onDataCBarg{nullptr};        // associated user argument

#ifdef ESP32
    SemaphoreHandle_t threadLock{xSemaphoreCreateRecursiveMutex()};
#endif

    // request and response String buffers and header list (same queue for request and response).

    xbuf*       _request{nullptr};              // Tx data buffer
    xbuf*       _response{nullptr};             // Rx data buffer for headers
    xbuf*       _chunks{nullptr};               // First stage for chunked response
    header*     _headers{nullptr};              // request or (readyState > readyStateHdrsRcvd) response headers

    // Protected functions

    header*     _addHeader(const String &name, const String &value);
    header*     _getHeader(const String &name);
    header*     _getHeader(int idx);
    bool        _buildRequest();
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
