/*
created March 2017
by Leandro Späth
*/

#ifndef M590_h //handle including library twice
#define M590_h

#include <Arduino.h>
#include <SoftwareSerial.h>

#define COMMAND_TIMEOUT 1000
#define LONG_TIMEOUT 10000
#define ASYNC_TIMEOUT   60000
#define STATUS_POLLING_RATE 250

#define DEBUG

#ifdef DEBUG
#define MONITOR(a) _debugSerial->print(a);
#define MONITOR_NL() _debugSerial->println();
#else
#define MONITOR(a)
#define MONITOR_NL()
#endif

enum m590ResponseCode {
    M590_SUCCESS,
    M590_FAILURE,
    M590_TIMEOUT,
    M590_LENGTH_EXCEEDED,
    M590_ASYNC_RUNNING,
    M590_NO_PARAMETERS,
    M590_UNDEFINED
};

enum m590States {
    M590_STATE_SHUTDOWN,                        //0
    M590_STATE_STARTUP,
    M590_STATE_STARTUP_DONE,
    M590_STATE_PIN_REQUIRED,
    M590_STATE_PIN_ENTRY_DONE,
    M590_STATE_PIN_VALIDATION,                  //5
    M590_STATE_PIN_VALIDATION_DONE,
    M590_STATE_CELLULAR_CONNECTING,
    M590_STATE_CELLULAR_CONNECTED,
    M590_STATE_FATAL_ERROR                      //9
};

enum m590CommandStates {
    M590_STATE_IDLE,
    M590_STATE_ussdCbk_START,
    M590_STATE_ussdCbk_RUN,
    M590_STATE_CMGR_START,
    M590_STATE_CMGR_RUN,
    M590_STATE_CMGL_START,
    M590_STATE_CMGL_RUN
};

enum m590NetworkStates {
    M590_NET_NOT_REGISTERED_NOT_SEARCHING,
    M590_NET_REGISTERED,
    M590_NET_REGISTRATION_REFUSED,
    M590_NET_SEARCHING_NOT_REGISTERED,
    M590_NET_UNKNOWN,
    M590_NET_REGISTERED_ROAMING,
    M590_NET_PARSE_ERROR, //not actually part of response, used to determine function failure
};

enum m590SMSdeleteFlags{
    M590_SMS_DEL_REC_NR,
    M590_SMS_DEL_ALL_READ,
    M590_SMS_DEL_ALL_READ_SENT,
    M590_SMS_DEL_ALL_READ_SENT_UNSENT,
    M590_SMS_DEL_ALL
};

enum m590SMSreadFlags{
    M590_SMS_REC_UNREAD,
    M590_SMS_REC_READ,
    M590_SMS_STO_UNSENT,
    M590_SMS_STO_SENT,
    M590_SMS_ALL
};

typedef enum {
    M590_SMS_CMGR,
    M590_SMS_STATUS,
    M590_SMS_STATUS_QUOTE,
    M590_SMS_SENDER,
    M590_SMS_SENDER_COMMA,
    M590_SMS_SENDER_2ND_COMMA,
    M590_SMS_SENDER_QUOTE,
    M590_SMS_DATE,
    M590_SMS_DATE_NL,
    M590_SMS_TEXT,
    M590_SMS_OK,
    M590_SMS_COMPLETED
}m590SMSParseState;


class M590 {
public:
     M590();

     bool begin(unsigned long baudRate = 115200,HardwareSerial *gsmSerial = NULL);

     void enableDebugSerial(HardwareSerial *debugSerial = NULL);

    int available();

     bool connected();

    char read();

    void write(const char c);

    void print(String s);

     bool initialize(String pin = "");

     void loop();

     bool checkAlive(void(*callback)(void) = NULL);

     bool isAlive();

     bool checkPinRequired();

     bool sendPinEntry(String pin, void(*callback)(void) = NULL);

     m590NetworkStates checkNetworkState();


    /** Synchronous intefaces **/

     bool getSignalStrength(byte* rssi, byte* ber);

     bool setSMSTextModeCharSetGSM();

     bool sendSMS(const char * phoneNumberString, const char * messageString);

     bool sendussdCbk(const char * ussdCbkString);

     bool readSMS(byte indexString, char * smsBuffer, byte smsBufferSize);

     bool deleteSMS(byte index, byte deleteFlag);

    /** Asynchronous interfaces **/

     bool SendussdCbk_Async(const char * ussdCbkString, void(*notificationCbk)());

     bool readSMS_Async(byte indexString, char * smsBuffer, unsigned int smsBufferSize, void(*notificationCbk)());
     bool readParsedSMS_Async(byte indexString, char * smsText, unsigned int smsTextSize, char * smsStatus, unsigned int smsStatusSize,
                       char * smsSender, unsigned int smsSenderSize, char * smsDate, unsigned int smsDateSize, void(*notificationCbk)());

     bool readSMSList_Async(m590SMSreadFlags readFlag, char * smsBuffer, unsigned int smsBufferSize, void(*notificationCbk)());
    
    bool enableNewSMSNotification(void(*newSMSnotificationCbk)(byte index));

    bool disableNewSMSNotification();

    m590ResponseCode checkForNewSMS();

    

private:
    HardwareSerial *_gsmSerial;
    HardwareSerial *_debugSerial;
    m590States _currentState = M590_STATE_SHUTDOWN;
    m590States _previousState = M590_STATE_SHUTDOWN;
    unsigned long _asyncStartTime = 0;
    byte _asyncBytesMatched = 0;
    byte _asyncResponseLength = 0;
    const char *_asyncProgmemResponseString = NULL;
    const char *_asyncNewSMString = NULL;
    char _responseBuffer[16];
    char * _smsStatus = NULL;
    byte _smsStatusSize = 0;
    char * _smsSender = NULL;
    byte _smsSenderSize = 0;
    char * _smsDate = NULL;
    byte  _smsDateSize = 0;
    char * _smsBuffer = NULL;
    byte _smsBufferSize = 0;

    byte asyncMatchedChars = 0;
    

    /** Async related variables **/
    m590CommandStates _commandState = M590_STATE_IDLE;
    m590SMSParseState _smsParseState = M590_SMS_CMGR;

    unsigned int _asyncResponseIndex = 0;
    bool _newSMSArrived = false;


    bool restart();

    void sendCommandWithoutPrefix(const char *progmemCommand,
                                  const char *params = NULL);

    void sendCommand(const char *progmemCommand,
                     const char *params = NULL);

    //if given a buffer pointer, the buffer will contain the response data after the colon
    m590ResponseCode readForResponse(const char *progmemResponseString,
                                     char *buffer = NULL,
                                     const unsigned int max_bytes = 0,
                                     const unsigned int timeout = COMMAND_TIMEOUT);

    m590ResponseCode readForResponses(const char *progmemResponseString,
                                      const char *progmemFailString,
                                      const unsigned int timeout = COMMAND_TIMEOUT);

    m590ResponseCode serialToBuffer(char *buffer,
                                    const char readUntil,
                                    const unsigned int max_bytes,
                                    const unsigned int timeout = COMMAND_TIMEOUT);

    m590ResponseCode serialToBuffer_Async(char *buffer,
                                          const char readUntil,
                                          const unsigned int maxBytes,
                                          const unsigned int timeout = ASYNC_TIMEOUT);

    m590ResponseCode serialToBuffer_Async(char *buffer,
                                          const char *progmemResponseString,
                                          const unsigned int maxBytes,
                                          const unsigned int timeout = ASYNC_TIMEOUT);


    m590ResponseCode readUntil(const char readUntil,
                               const unsigned int timeout = COMMAND_TIMEOUT);

    m590ResponseCode readUntil_Async(const char readUntil,
                                     const unsigned int timeout = ASYNC_TIMEOUT);

    bool bufferStartsWithProgmem(char *buffer,
                                 const char *progmemString);

    void printDebug(const char *progmemString, bool withNewline = true);

    void printDebug(const String s, bool withNewline = true);

    /** Synchronous interfaces **/

    m590ResponseCode readForSMS(const char *progmemResponseString, const char *progmemFailString, char *buffer, const unsigned int max_bytes,
        const unsigned int timeout = COMMAND_TIMEOUT);

    /** Asynchronous interfaces **/

    void resetAsyncVariables();

    m590ResponseCode readForAsyncResponse(const char *progmemResponseString = NULL,
        const unsigned int timeout = ASYNC_TIMEOUT);

    m590ResponseCode readForSMS_Async(const char *progmemResponseString = NULL,
        const unsigned int timeout = ASYNC_TIMEOUT);

    m590ResponseCode parseSMS(/*const char *progmemResponseString, const unsigned int timeout*/);

    void returnToIdle();

    void (M590::*_handlerCbk)() = NULL;

    void(*_notificationCbk)() = NULL;

    void(*_newSMSnotificationCbk)(byte index) = NULL;

    void(M590::*_IdleTask)() = NULL;

    void handlerGateway();

    void handlerSMSNotification();

    void handlerCUSD();

    void handlerCMGR();

    void handlerParsedCMGR();

    void handlerCMGL();
};

#endif


/* void _handleReturn(String retStr); //handle an async answer from the module
     void _parseReturn(char c);

     void _parseSerial();*/


/*void loop(); //function called on every loop
//void begin(String pin); //connect to cellular network with pin
byte getSignalStrength(); //return current signal strength
void beginGPRS(String apn); //connect data link with APN
void beginGPRS(String apn, String user, String pass); //connect data link with APN login credentials
bool connectTCP(String ip, String port); //initialize TCP connection, returns success
bool disconnectTCP(); //disconnect TCP connection
String doRequest(String req); //perform an HTTP request
String dns(String host); //perform a DNS request
bool sendAT(String cmd); //send an AT command directly to the M590 module*/
