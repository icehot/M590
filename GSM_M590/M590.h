// M590.h

#ifndef _M590_h
#define _M590_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "Fifo.h"
#define MONITOR_M590

#ifdef MONITOR_M590
#define MONITOR(a) if(_debugSerial) _debugSerial->print(a);
#define MONITOR_NL() if(_debugSerial) _debugSerial->println();
#else
#define MONITOR(a)
#define MONITOR_NL()
#endif

/* Timeout values in milliseconds */
#define COMMAND_TIMEOUT            1000
#define ASYNC_TIMEOUT              2000
#define PIN_VALIDATION_TIMEOUT     20000
#define INIT_TIMEOUT               30000
#define SEARCH_FOR_NETWORK_TIMEOUT 10000
#define STATUS_POLLING_RATE        250

typedef enum {
    M590_RESPONSE_IDLE = 0,
    M590_RESPONSE_RUNNING = 1,
    M590_RESPONSE_SUCCESS = 2,
    M590_RESPONSE_TIMEOUT = 3,
    M590_RESPONSE_LENGTH_EXCEEDED = 4,
    M590_RESPONSE_FAILURE = 5
}ResponseStateType;

typedef struct {
    const char* command;
    String parameter;
    const char* response;
    char* responebuffer;
    void(*commandCbk)(ResponseStateType response) = NULL;
    unsigned long timeout = ASYNC_TIMEOUT;
}CommandType;

typedef enum {
    M590_MODEM_IDLE = 0,
    M590_MODEM_BUSY = 1
}ModemStateType;

typedef enum {
    M590_UNINITIALIZED = 0,
    M590_INIT_CHECK_ALIVE,
    M590_INIT_PIN_CHECK,
    M590_INIT_INPUT_PIN,
    M590_INIT_PIN_VALIDATION,
    M590_INIT_DONE,
    M590_INIT_ERROR,
}InitStateType;

typedef enum {
    M590_PIN_NOT_REQUIRED = 0,
    M590_PIN_REQUIRED = 1,
    M590_PIN_ERROR = 2
}PinStateType;

typedef enum  {
    M590_NET_NOT_REGISTERED_NOT_SEARCHING,
    M590_NET_REGISTERED,
    M590_NET_REGISTRATION_REFUSED,
    M590_NET_SEARCHING_NOT_REGISTERED,
    M590_NET_UNKNOWN,
    M590_NET_REGISTERED_ROAMING,
    M590_NET_PARSE_ERROR, //not actually part of response, used to determine function failure
}NetworkStateType;



const char
M590_COMMAND_AT[]                       PROGMEM = "AT",
M590_COMMAND_CHECK_PIN[]                PROGMEM = "AT+CPIN?",
M590_COMMAND_INPUT_PIN[]                PROGMEM = "AT+CPIN=",
M590_COMMAND_CHECK_NETWORK_STATUS[]     PROGMEM = "AT+CREG?",
M590_COMMAND_GET_SIGNAL_STRENGTH[]      PROGMEM = "AT+CSQ",
M590_COMMAND_SET_MODEM_GSM_CHARSET[]    PROGMEM = "AT+CSCS=\"GSM\"",
M590_COMMAND_SET_MODEM_TEXT_MODE[]      PROGMEM = "AT+CMGF=1",
M590_COMMAND_USSD_START[]               PROGMEM = "AT+CUSD=1,\"",
M590_COMMAND_USSD_END[]                 PROGMEM = "\",15", 

M590_RESPONSE_PREFIX[]                  PROGMEM = "+",
M590_RESPONSE_PIN_REQUIRED[]            PROGMEM = " SIM PIN",
M590_RESPONSE_PIN_NOT_REQUIRED[]        PROGMEM = " READY",
M590_RESPONSE_PIN_VAL_DONE[]            PROGMEM = "+PBREADY",
M590_RESPONSE_ERROR[]                   PROGMEM = "ERROR",
M590_RESPONSE_FAIL[]                    PROGMEM = "FAIL\r\n",
M590_RESPONSE_OK[]                      PROGMEM = "OK\r\n";

const char
M590_ERROR_NO_GSM_SERIAL[]                   PROGMEM = "\n#M590: There is no Serial port assigned for GSM",
M590_ERROR_NOT_RESPONDING[]                  PROGMEM = "\n#M590: The M590 did not respond to an \"AT\". Please check serial connection, power supply and ONOFF pin.",
M590_ERROR_NO_PIN[]                          PROGMEM = "\n#M590: No pin was specified, but the module requests one",
M590_ERROR_WRONG_PIN[]                       PROGMEM = "\n#M590: Wrong PIN was entered, down one try.",
M590_ERROR_OTHER_PIN_ERR[]                   PROGMEM = "\n#M590: Error during PIN check, maybe a PUK is required, please check SIM card in a phone",
M590_ERROR_PINVAL_TIMEOUT[]                  PROGMEM = "\n#M590: Timeout during pin validation, please check module and try again",
M590_ERROR_NETWORK_REG_TIMEOUT[]             PROGMEM = "\n#M590: Timeout during network registration",
M590_ERROR_NETWORK_NOT_REG[]                 PROGMEM = "\n#M590: Not registered on network",
M590_ERROR_UNHANDLED_NET_STATE[]             PROGMEM = "\n#M590: Network status returned unhandled state: ",
M590_ERROR_INIT_TIMEOUT[]                    PROGMEM = "\n#M590: Initialization Timeout",
M590_ERROR_RESPONSE_TIMEOUT[]                PROGMEM = "\n#M590: Response Timeout",
M590_ERROR_RESPONSE_LENGTH_EXCEEDED[]        PROGMEM = "\n#M590: Response Lenght exceeded",
M590_ERROR_INVALID_STATE_STATE[]             PROGMEM = "\n#M590: Invalid State";

class M590
{
    public:
        M590();
        bool enableDebugSerial(HardwareSerial *debugSerial);
        bool init(unsigned long baudRate, HardwareSerial *gsmSerial, char* pin = "");
        bool waitForNetWork(const unsigned int timeout = SEARCH_FOR_NETWORK_TIMEOUT);

        void process();
        bool checkAlive(void(*commandCbk)(ResponseStateType response) = NULL);
        bool getSignalStrength(byte* rssi, byte* ber);
        bool setSMSTextModeCharSetGSM(void(*commandCbk)(ResponseStateType response) = NULL);
        bool sendUSSD(char * ussdString, void(*commandCbk)(ResponseStateType response) = NULL);

    private:
        HardwareSerial *_gsmSerial = NULL;
        HardwareSerial *_debugSerial = NULL;
        Fifo<CommandType> _commandFifo;
        ModemStateType _gsmState = M590_MODEM_IDLE;
        ResponseStateType _responseState = M590_RESPONSE_IDLE;
        NetworkStateType _networkState = M590_NET_NOT_REGISTERED_NOT_SEARCHING;

        InitStateType _initState = M590_UNINITIALIZED;
        unsigned long _asyncStartTime = 0;
        unsigned long _asyncTimeout = 0;
        const char *_asyncProgmemResponseString = NULL;
        byte _asyncResponseLength = 0;
        byte _asyncBytesMatched = 0;
        void(*_commandCbk)(ResponseStateType response) = NULL;
        char _responseBuffer[16];

        void resetAsyncVariables();
        void sendCommand(const char *progmemCommand, String params);
        void readForResponse(const char *progmemResponseString, const unsigned int timeout, void(*commandCbk)(ResponseStateType response));
        void responseHandler();

        void printDebug(const char *progmemString, bool withNewline = true);
        void printDebug(const String s, bool withNewline = true );

        /* Init related synchronous interfaces */
        bool isAlive();
        PinStateType checkPinRequired();
        bool M590::sendPinEntry(char* pin);
        bool M590::pinValidation();
        NetworkStateType checkNetworkState();
        ResponseStateType readForResponseBufferedSync(const char *progmemResponseString, char *buffer = NULL, const unsigned int max_bytes = 0, const unsigned int timeout = COMMAND_TIMEOUT);
        ResponseStateType readForResponseSync(const char *progmemResponseString, const char *progmemFailString, const unsigned int timeout = COMMAND_TIMEOUT);
        bool bufferStartsWithProgmem(char *buffer, const char *progmemString);
};

#endif

