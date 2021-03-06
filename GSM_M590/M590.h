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
#define ATTACH_GPRS_TIMEOUT        10000
#define SEARCH_FOR_NETWORK_TIMEOUT 10000
#define DNS_TIMEOUT                10000
#define TCP_TIMEOUT                10000
#define STATUS_POLLING_RATE        250

typedef enum {
    M590_RESPONSE_IDLE = 0,
    M590_RESPONSE_RUNNING = 1,
    M590_RESPONSE_SUCCESS = 2,
    M590_RESPONSE_TIMEOUT = 3,
    M590_RESPONSE_LENGTH_EXCEEDED = 4,
    M590_RESPONSE_FAILURE = 5,
    M590_RESPONSE_PREPARE_IDLE
}ResponseStateType;

typedef enum {
    M590_RES_NULL,
    M590_RES_CUSD,
    M590_RES_CMGR,
    M590_RES_CMGL,
    M590_RES_CMGS,
    M590_RES_CSQ,
    M590_RES_XIIC,
    M590_RES_DNS,
    M590_RES_TCPSETUP0,
    M590_RES_TCPSETUP1,
    M590_RES_TCPSEND
}ResultType;



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
    M590_GPRS_DISCONNECTED = 0,
    M590_GPRS_INTERNAL_STACK,
    M590_GPRS_SET_APN,
    M590_GPRS_AUTH_APN,
    M590_GPRS_CONNECT_APN,
    M590_GPRS_CHECK_IP,
    M590_GPRS_ERROR,
    M590_GPRS_CONNECTED,
}GprsStateType;

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

typedef enum {
    M590_SMS_DEL_REC_NR,
    M590_SMS_DEL_ALL_READ,
    M590_SMS_DEL_ALL_READ_SENT,
    M590_SMS_DEL_ALL_READ_SENT_UNSENT,
    M590_SMS_DEL_ALL
}DeleteFlagType;

typedef enum {
    M590_SMS_REC_UNREAD,
    M590_SMS_REC_READ,
    M590_SMS_STO_UNSENT,
    M590_SMS_STO_SENT,
    M590_SMS_ALL
}M590_SMSReadFlagType;

typedef enum{
    M590_GPRS_LINK_0 = 0,
    M590_GPRS_LINK_1 = 1
}M590_GPRSLinkType;

typedef struct{
    byte rssi;
    byte ber;
}M590_SignalQuality_ResultType;

typedef String M590_USSDResponse_ResultType;

typedef struct{
    String text;
    String status;
    String sender;
    String date;
}M590_SMS_ResultType;

typedef struct{
    char smsCount;
    M590_SMS_ResultType smsArray[10];
}M590_SMSList_ResultType;

typedef String M590_IP_ResultType;

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
M590_COMMAND_SMS_START[]                PROGMEM = "AT+CMGS=\"",
M590_COMMAND_SMS_END[]                  PROGMEM = "\"",
M590_COMMAND_SMS_DELETE[]               PROGMEM = "AT+CMGD=",
M590_COMMAND_SMS_READ[]                 PROGMEM = "AT+CMGR=",
M590_COMMAND_SMS_LIST_READ[]            PROGMEM = "AT+CMGL=",
M590_COMMAND_SMS_NOTIFICATION_ENABLE[]  PROGMEM = "AT+CNMI=2,1,0,0,0",
M590_COMMAND_SMS_NOTIFICATION_DISABLE[] PROGMEM = "AT+CNMI=1,0,0,0,0",
M590_COMMAND_GPRS_CONNECT[]             PROGMEM = "AT+XIIC=1",
M590_COMMAND_GPRS_DISCONNECT[]          PROGMEM = "AT+XIIC=0",
M590_COMMAND_GPRS_CHECK_CONNECTION[]    PROGMEM = "AT+XIIC?",
M590_COMMAND_GPRS_INTERNAL_STACK[]      PROGMEM = "AT+XISP=0",
M590_COMMAND_GPRS_SET_APN[]             PROGMEM = "AT+CGDCONT=1,\"IP\",\"",
M590_COMMAND_GPRS_AUTHENTICATE[]        PROGMEM = "AT+XGAUTH=1,1,\"",
M590_COMMAND_GPRS_DNS[]                 PROGMEM = "AT+DNS=\"",
M590_COMMAND_GPRS_TCPSETUP[]            PROGMEM = "AT+TCPSETUP=",
M590_COMMAND_GPRS_TCPCLOSE[]            PROGMEM = "AT+TCPCLOSE=",
M590_COMMAND_GPRS_TCPSEND[]             PROGMEM = "AT+TCPSEND=",

M590_RESPONSE_PREFIX[]                  PROGMEM = "+",
M590_RESPONSE_TEXT_INPUT[]              PROGMEM = ">",
M590_RESPONSE_CMTI[]                    PROGMEM = "+CMTI: \"SM\",",
M590_RESPONSE_TCPCLOSE[]                PROGMEM = "+TCPCLOSE:",
M590_RESPONSE_TCPRECV[]                 PROGMEM = "+TCPRECV:",
M590_RESPONSE_PIN_REQUIRED[]            PROGMEM = " SIM PIN",
M590_RESPONSE_PIN_NOT_REQUIRED[]        PROGMEM = " READY",
M590_RESPONSE_PIN_VAL_DONE[]            PROGMEM = "+PBREADY",
M590_RESPONSE_DNS_OK[]                  PROGMEM = "+DNS:OK",
M590_RESPONSE_DNS_ERROR[]               PROGMEM = "+DNS:Error",
M590_RESPONSE_TCP0_OK[]                 PROGMEM = "+TCPSETUP:0,OK",
M590_RESPONSE_TCP1_OK[]                 PROGMEM = "+TCPSETUP:1,OK",
M590_RESPONSE_TCP0_CLOSE[]              PROGMEM = "+TCPCLOSE:0,OK",
M590_RESPONSE_TCP1_CLOSE[]              PROGMEM = "+TCPCLOSE:1,OK",
M590_RESPONSE_TCP0_SEND[]               PROGMEM = "+TCPSEND:0,",
M590_RESPONSE_TCP1_SEND[]               PROGMEM = "+TCPSEND:1,",
M590_RESPONSE_FAIL[]                    PROGMEM = "FAIL\r\n",
M590_RESPONSE_ERROR[]                   PROGMEM = "ERROR\r\n",
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
M590_ERROR_RESPONSE_ERROR[]                  PROGMEM = "\n#M590: Response Error",
M590_ERROR_RESPONSE_LENGTH_EXCEEDED[]        PROGMEM = "\n#M590: Response Lenght exceeded",
M590_ERROR_RESPONSE_INCOMPLETE[]             PROGMEM = "\n#M590: Incomplete Response",
M590_ERROR_BUFFER_OVERFLOW[]                 PROGMEM = "\n#M590: Buffer Overflow",
M590_ERROR_FIFO_FULL[]                       PROGMEM = "\n#M590: FIFO Full",
M590_ERROR_GPRS_DISCONNECT[]                 PROGMEM = "\n#M590: GPRS Disconnection Error",
M590_ERROR_GPRS_STACK[]                      PROGMEM = "\n#M590: GPRS Stack Error",
M590_ERROR_GPRS_APN[]                        PROGMEM = "\n#M590: GPRS Set APN Error",
M590_ERROR_GPRS_AUTH[]                       PROGMEM = "\n#M590: GPRS Authentication Error",
M590_ERROR_GPRS_CONNECTION[]                 PROGMEM = "\n#M590: GPRS Connection Error",
M590_ERROR_GPRS_IP_CHECK[]                   PROGMEM = "\n#M590: GPRS IP Check Error",
M590_ERROR_GPRS_DNS_ERROR[]                  PROGMEM = "\n#M590: GPRS Unknown Host",
M590_ERROR_GPRS_LINK0_CLOSED[]               PROGMEM = "\n#M590: Link0 Closed",
M590_ERROR_GPRS_LINK1_CLOSED[]               PROGMEM = "\n#M590: Link1 Closed",
M590_ERROR_GPRS_INVALID_LINK[]               PROGMEM = "\n#M590: Invalid Link",
M590_ERROR_INVALID_STATE[]                   PROGMEM = "\n#M590: Invalid State";

class M590
{
    typedef struct {
        const char* command;
        String parameter;
        const char* response;
        void(*commandCbk)(ResponseStateType response) = NULL;
        void(M590::*internalCbk)(ResponseStateType response) = NULL;
        unsigned long timeout = ASYNC_TIMEOUT;
        ResultType resultType = M590_RES_NULL;
        void* resultptr = NULL;
    }CommandType;

    typedef struct {
        Fifo<char> receiveFifo;
        bool connected = false;
        unsigned int port = 0;
        String ip = "";
        void(*_connectCbk)(ResponseStateType response) = NULL;
        void(*_disconnectCbk)(ResponseStateType response) = NULL;
    }SocketType;

    public:
        /* Constructor */
        M590();

        /* Initialization API-s*/
        bool enableDebugSerial(HardwareSerial *debugSerial);
        bool init(unsigned long baudRate, HardwareSerial *gsmSerial, char* pin = "");
        bool waitForNetWork(const unsigned int timeout = SEARCH_FOR_NETWORK_TIMEOUT);

        /* Public API-s */
        void process();
        bool checkAlive(void(*commandCbk)(ResponseStateType response) = NULL);
        bool getSignalStrength(M590_SignalQuality_ResultType * resultptr, void(*commandCbk)(ResponseStateType response) = NULL);
        bool setSMSTextModeCharSetGSM(void(*commandCbk)(ResponseStateType response) = NULL);
        bool sendSMS(const char *phoneNumber, const char* smsText, void(*commandCbk)(ResponseStateType response) = NULL);
        bool sendUSSD(char * ussdString, M590_USSDResponse_ResultType* resultptr, void(*commandCbk)(ResponseStateType response) = NULL);
        bool deleteSMS(byte index, DeleteFlagType deleteFlag, void(*commandCbk)(ResponseStateType response) = NULL);
        bool readSMS(byte index, M590_SMS_ResultType* resultptr, void(*commandCbk)(ResponseStateType response) = NULL);
        bool readSMSList(M590_SMSReadFlagType readFlag, M590_SMSList_ResultType* resultptr, void(*commandCbk)(ResponseStateType response) = NULL);
        bool enableNewSMSNotification(void(*newSMSnotificationCbk)(byte index), void(*commandCbk)(ResponseStateType response) = NULL);
        bool disableNewSMSNotification(void(*commandCbk)(ResponseStateType response) = NULL);

        /* GPRS initialization */
        bool detachGPRS();
        bool attachGPRS(const char* apn, const char* user, const char* pwd, M590_IP_ResultType* resultptr);
        GprsStateType M590::readyGPRS();
        /* Client */
        bool connect(M590_GPRSLinkType link, const char *host, unsigned int port, void(*connectCbk)(ResponseStateType response) = NULL, void(*disconnectCbk)(ResponseStateType response) = NULL);
        bool disconnect(M590_GPRSLinkType link, void(*commandCbk)(ResponseStateType response) = NULL);
        bool send(M590_GPRSLinkType link, const char * buffer, void(*commandCbk)(ResponseStateType response) = NULL);
        bool available(M590_GPRSLinkType link);
        char read(M590_GPRSLinkType link);

    private:
        /* Serial port handlers */
        HardwareSerial *_gsmSerial = NULL;
        HardwareSerial *_debugSerial = NULL;

        /* Internal storage variables */
        Fifo<CommandType> _commandFifo;

        char _internalBuffer[16];
        String _internalString = "";

        /* Internal state variables */
        ModemStateType _gsmState = M590_MODEM_IDLE;
        InitStateType _initState = M590_UNINITIALIZED;
        NetworkStateType _networkState = M590_NET_NOT_REGISTERED_NOT_SEARCHING;
        ResponseStateType _responseState = M590_RESPONSE_IDLE;
        bool _smsNotificationEnabled = false;
        bool _newSMSArrived = false;

        /* Internal variables for asynchronous response handling */
        unsigned long _asyncStartTime = 0;
        unsigned long _asyncTimeout = 0;
        const char *_asyncProgmemResponseString = NULL;
        byte _asyncResponseLength = 0;
        byte _asyncBytesMatched = 0;
        byte _asyncFailMatched = 0;
        byte _asyncSMSMatched = 0;
        byte _asyncProcessState = 0;
        String _asyncTempString = "";
        void (*_asyncCommandCbk)(ResponseStateType response) = NULL;
        void (*_newSMSnotificationCbk)(byte index) = NULL;
        ResultType _asyncResultType = M590_RES_NULL;
        void *_asyncResultptr = NULL;
        void (M590::*_asyncInternalCbk)(ResponseStateType response) = NULL;
        
        /* GPRS related variables*/
        SocketType _socket[2];
        GprsStateType _gprsState = M590_GPRS_DISCONNECTED;
        bool _tcpCloseDetected = false;
        bool _tcpReceiveDetected = false;
        byte _asyncTCPCloseMatched = 0;
        byte _asyncTCPReceiveMatched = 0;
        byte _asyncReceiveState = 0;
        unsigned int _asyncNrOfRecvBytes = 0;
        byte _asyncLink = 0;

        /* Internal core functions*/
        void resetAsyncVariables();
        void sendCommand(const char *progmemCommand, String params);
        void responseHandler();
        ResponseStateType readForResponseAsync();
        void processResult(char c);
        void idleHandler();

        /* GPRS related functions */
        void tcpSetup0(ResponseStateType response);
        void tcpSetup1(ResponseStateType response);
        void checkForTCPClose(char c);
        void checkForTCPReceive(char c);
        void processXIIC(char c);
        void processDNS(char c);
        void processTCPSetup(char c);
        void processTCPSend(char c);

        /* Internal  handler functions for different results */
        void processCSQ(char c);
        void processCUSD(char c);
        void processCMGR(char c);
        void processCMGL(char c);
        void processCMGS(char c);

        /* Internal utility functions */
        void printDebug(const char *progmemString, bool withNewline = true);
        void printDebug(const String s, bool withNewline = true );
        void gatewayHandler();
        void checkForNewSMS(char c);

        /* Init related synchronous interfaces */
        bool isAlive();
        PinStateType checkPinRequired();
        bool M590::sendPinEntry(char* pin);
        bool M590::pinValidation();
        NetworkStateType checkNetworkState();
        ResponseStateType readForResponseStringBufferedSync(const char *progmemResponseString, const char *progmemFailString, String* buffer, const unsigned int timeout = COMMAND_TIMEOUT);
        ResponseStateType readForResponseBufferedSync(const char *progmemResponseString, char *buffer = NULL, const unsigned int max_bytes = 0, const unsigned int timeout = COMMAND_TIMEOUT);
        ResponseStateType readForResponseSync(const char *progmemResponseString, const char *progmemFailString, const unsigned int timeout = COMMAND_TIMEOUT);
        bool bufferStartsWithProgmem(char *buffer, const char *progmemString);

        /* GPRS initialization related interfaces */
        bool setInternalStack();
        bool setAPN(const char * apn);
        bool authenticateAPN(const char * user, const char * pwd);
        bool connectToAPN();
        bool checkForIP(M590_IP_ResultType* resultptr);
        bool dnsIpQuery(const char* host, M590_IP_ResultType* resultptr);
};

#endif

