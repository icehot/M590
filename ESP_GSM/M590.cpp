/*
created March 2017
by Leandro Späth
*/

#include <Arduino.h>
#include "M590.h"

const char
        M590_COMMAND_GET_SIM_IDENTIFICATION[]   PROGMEM = "CCID",
        M590_COMMAND_CHECK_STATUS[]             PROGMEM = "CPAS",
        M590_COMMAND_CHECK_NETWORK_STATUS[]     PROGMEM = "CREG?",
        M590_COMMAND_CHECK_PIN[]                PROGMEM = "CPIN?",
        M590_COMMAND_INPUT_PIN[]                PROGMEM = "CPIN=",
        M590_COMMAND_SHUTDOWN[]                 PROGMEM = "CPWROFF",
        M590_COMMAND_GET_NATIVE_NUMBER[]        PROGMEM = "CNUM",
        M590_COMMAND_GET_SIGNAL_STRENGTH[]      PROGMEM = "CSQ",
        M590_COMMAND_SMS_PHONE_NR_START[]       PROGMEM = "CMGS=\"",
        M590_COMMAND_SMS_PHONE_NR_END[]         PROGMEM = "\"",
        M590_COMMAND_SMS_READ[]                 PROGMEM = "CMGR=",
        M590_COMMAND_SMS_READ_LIST[]            PROGMEM = "CMGL=",
        M590_COMMAND_SMS_DELETE[]               PROGMEM = "CMGD=",
        M590_COMMAND_SET_MODEM_GSM_CHARSET[]    PROGMEM = "CSCS=\"GSM\"",
        M590_COMMAND_SET_MODEM_TEXT_MODE[]      PROGMEM = "CMGF=1",
        M590_COMMAND_USSD_START[]               PROGMEM = "CUSD=1,\"",
        M590_COMMAND_USSD_END[]                 PROGMEM = "\",15",
        M590_COMMAND_SMS_NOTIFICATION[]         PROGMEM = "CNMI=2,1,0,0,0";


const char
M590_RESPONSE_PREFIX[]          PROGMEM = "+",//"\r\n+",
M590_RESPONSE_COLON[]           PROGMEM = ": ",
M590_RESPONSE_COMMA             PROGMEM = ',',
M590_RESPONSE_QUOTE             PROGMEM = '\"',
M590_RESPONSE_NL                PROGMEM = '\n',
M590_RESPONSE_TEXT_NPUT[]       PROGMEM = ">",
M590_RESPONSE_OK[]              PROGMEM = "OK\r\n",
M590_RESPONSE_ERROR[]           PROGMEM = "ERROR\r\n",
M590_RESPONSE_FAIL[]            PROGMEM = "FAIL\r\n",
M590_RESPONSE_PIN_REQUIRED[]    PROGMEM = " SIM PIN",
M590_RESPONSE_PIN_DONE[]        PROGMEM = " READY",
M590_RESPONSE_PIN_VAL_DONE[]    PROGMEM = "+PBREADY",
M590_RESPONSE_CMTI[]            PROGMEM = "+CMTI: \"SM\",",
M590_RESPONSE_SMS_END[]         PROGMEM = "\r\n\r\nOK\r\n",
M590_RESPONSE_CMGR[]            PROGMEM = "+CMGR: \"";

const char
        M590_AT[]                       PROGMEM = "AT",
        M590_CRLF[]                     PROGMEM = "\r\n",
        M590_COMMAND_PREFIX[]           PROGMEM = "AT+",
        M590_CONTENT_LENGTH_HEADER[]    PROGMEM = "Content-Length: ";

const char
        M590_ERROR_NOT_RESPONDING[]         PROGMEM = "\nThe M590 did not respond to an \"AT\". Please check serial connection, power supply and ONOFF pin.",
        M590_ERROR_NO_PIN[]                 PROGMEM = "\nNo pin was specified, but the module requests one",
        M590_ERROR_WRONG_PIN[]              PROGMEM = "\nWrong PIN was entered, down one try.",
        M590_ERROR_OTHER_PIN_ERR[]          PROGMEM = "\nError during PIN check, maybe a PUK is required, please check SIM card in a phone",
        M590_ERROR_PINVAL_TIMEOUT[]         PROGMEM = "\nTimeout during pin validation, please check module and try again",
        M590_ERROR_UNHANDLED_NET_STATE[]    PROGMEM = "\nNetwork status returned unhandled state: ";
        


const char
        M590_LOG_NO_PIN_REQUIRED[]      PROGMEM = "No PIN was required";

const char
        M590_LOG_00[]   PROGMEM = "Shutdown",
        M590_LOG_01[]   PROGMEM = "In Startup",
        M590_LOG_02[]   PROGMEM = "Module is active.",
        M590_LOG_03[]   PROGMEM = "Pin entry is required",
        M590_LOG_04[]   PROGMEM = "Pin entry successful",
        M590_LOG_05[]   PROGMEM = "Pin is being validated",
        M590_LOG_06[]   PROGMEM = "Pin validation successful",
        M590_LOG_07[]   PROGMEM = "Registering on cellular network",
        M590_LOG_08[]   PROGMEM = "Connected to cellular network",
        M590_LOG_09[]   PROGMEM = "A fatal error occured, library can not continue";

//crude method of accessing multiple progmem Strings easily // index corresponds to m590States
const char *M590_LOG[] = {
        M590_LOG_00,
        M590_LOG_01,
        M590_LOG_02,
        M590_LOG_03,
        M590_LOG_04,
        M590_LOG_05,
        M590_LOG_06,
        M590_LOG_07,
        M590_LOG_08,
        M590_LOG_09,
};


M590::M590() {
    _gsmSerial = NULL;
}

bool M590::begin(unsigned long baudRate, HardwareSerial *gsmSerial){
    if (!_gsmSerial) {
        _gsmSerial = gsmSerial;
    }

    _gsmSerial->begin(baudRate);
}

void M590::enableDebugSerial(HardwareSerial *debugSerial) {
    if (debugSerial)
        _debugSerial = debugSerial;
}

// Serial passthrough operations
int M590::available() {
    return _gsmSerial->available();
}

char M590::read() {
    return (char) _gsmSerial->read();
}

void M590::write(const char c) {
    _gsmSerial->write(c);
}

void M590::print(const String s) {
    _gsmSerial->print(s);
}


bool M590::initialize(String pin) {
    
    _asyncNewSMString = M590_RESPONSE_CMTI;
    _commandState = M590_STATE_IDLE;

    _IdleTask = &handlerGateway;
    _handlerCbk = _IdleTask;

    if (!checkAlive()) {//checkAlive still gets executed
        printDebug(M590_ERROR_NOT_RESPONDING); //TODO: better error handling
        return false;
    }

    checkPinRequired();
    if (_currentState == M590_STATE_PIN_REQUIRED) {
        if (pin && pin != "")
            sendPinEntry(pin); //sets state to pin_entry_done, when successful
        else {
            printDebug(M590_ERROR_NO_PIN);
            return false;
        }
    } else if (_currentState == M590_STATE_FATAL_ERROR)
        return false;

    if (_currentState == M590_STATE_PIN_ENTRY_DONE) {
        _currentState = M590_STATE_PIN_VALIDATION;
        readForAsyncResponse(M590_RESPONSE_PIN_VAL_DONE); //start asnyc reading (execution continued in loop())
    } else if (_currentState == M590_STATE_PIN_VALIDATION_DONE) {
        printDebug(M590_LOG_NO_PIN_REQUIRED);
    } else {
        _currentState = M590_STATE_FATAL_ERROR;
        printDebug(M590_ERROR_WRONG_PIN);
    }
}


//thee loop gets called every arduino code to handle "async" responses
void M590::loop() {
    switch (_currentState) {
        case M590_STATE_STARTUP_DONE:
            break;

        case M590_STATE_PIN_VALIDATION: {
            m590ResponseCode status = readForAsyncResponse(); //call function with last entered parameters
            if (status == M590_SUCCESS)
                _currentState = M590_STATE_PIN_VALIDATION_DONE;
            else if (status == M590_TIMEOUT) {
                _currentState = M590_STATE_FATAL_ERROR;
                printDebug(M590_ERROR_PINVAL_TIMEOUT);
            }
            break;
        }

        case M590_STATE_PIN_VALIDATION_DONE: {
            _currentState = M590_STATE_CELLULAR_CONNECTING;
            break;
        }

        case M590_STATE_CELLULAR_CONNECTING: {
            unsigned long curMillis = millis();
            if (_asyncStartTime == 0) _asyncStartTime = curMillis; //repurpose asyncStartTime variable
            else if (curMillis >= _asyncStartTime + STATUS_POLLING_RATE) {
                m590NetworkStates netState = checkNetworkState();
                if (netState == M590_NET_REGISTERED)
                    _currentState = M590_STATE_CELLULAR_CONNECTED;
                else if (netState == M590_NET_SEARCHING_NOT_REGISTERED) {
                    printDebug("."); //print dots to show wait for registration
                } else {
                    _currentState = M590_STATE_FATAL_ERROR;
                    printDebug(M590_ERROR_UNHANDLED_NET_STATE);
                    printDebug(String(netState), true);
                }
                _asyncStartTime = curMillis;
            }
            break;
        }

        case M590_STATE_FATAL_ERROR: {
            //reset the library and try again
            break;

        default:
            break;
        }
        
    }
    if (_previousState != _currentState) {
        printDebug(M590_LOG[_currentState]);
    }
    _previousState = _currentState;

    (this->*_handlerCbk)();
}

bool M590::checkAlive(void(*callback)(void)) {
    if (_currentState == M590_STATE_SHUTDOWN) {
        sendCommandWithoutPrefix(M590_AT);
        if (readForResponse(M590_RESPONSE_OK) == M590_SUCCESS) {
            _currentState = M590_STATE_STARTUP_DONE;
            if (callback) callback();
            return true;
        } else
            return false;
    } else return false;
}

bool M590::checkPinRequired() {
    if (_currentState == M590_STATE_STARTUP_DONE) {
        sendCommand(M590_COMMAND_CHECK_PIN);
        memset(_responseBuffer, 0, sizeof(_responseBuffer));
        readForResponse(M590_RESPONSE_OK, _responseBuffer, sizeof(_responseBuffer));
        bool required = bufferStartsWithProgmem(_responseBuffer, M590_RESPONSE_PIN_REQUIRED);
        //check if module does not need pin entry
        bool alreadyReady = bufferStartsWithProgmem(_responseBuffer, M590_RESPONSE_PIN_DONE);
        _currentState = required ? M590_STATE_PIN_REQUIRED : M590_STATE_PIN_VALIDATION_DONE;
        if (!required && !alreadyReady) {
            _currentState = M590_STATE_FATAL_ERROR;
            printDebug(M590_ERROR_OTHER_PIN_ERR);
        }
        return required; //returns true, if pin is required
    } else return false;
}

bool M590::sendPinEntry(String pin, void (*callback)(void)) {
    if (_currentState == M590_STATE_PIN_REQUIRED) {
        _gsmSerial->print((__FlashStringHelper *) M590_COMMAND_PREFIX);
        _gsmSerial->print((__FlashStringHelper *) M590_COMMAND_INPUT_PIN);
        _gsmSerial->print('"');
        _gsmSerial->print(pin);
        _gsmSerial->print('"');
        _gsmSerial->println();
        bool success = readForResponses(M590_RESPONSE_OK, M590_RESPONSE_FAIL) == M590_SUCCESS;
        if (success) _currentState = M590_STATE_PIN_ENTRY_DONE;
        return success;
    }
    return false;
}

m590NetworkStates M590::checkNetworkState() {
    sendCommand(M590_COMMAND_CHECK_NETWORK_STATUS);
    memset(_responseBuffer, 0, sizeof(_responseBuffer));
    m590ResponseCode r = readForResponse(M590_RESPONSE_OK, _responseBuffer, sizeof(_responseBuffer));
    //the fourth char in the response (e.g. " 0,3") will be the registration state (e.g. 3)
    if (r == M590_SUCCESS)
        return (m590NetworkStates) (_responseBuffer[3] - '0'); //convert to integer, maps to m590NetworkStates
    else return M590_NET_PARSE_ERROR;
}

bool M590::getSignalStrength(byte* rssi, byte* ber) 
{
    int i;
    String tempString = "";

    if (_currentState == M590_STATE_CELLULAR_CONNECTED)
    {
        sendCommand(M590_COMMAND_GET_SIGNAL_STRENGTH);
        memset(_responseBuffer, 0, sizeof(_responseBuffer));
        if (M590_SUCCESS == readForResponse(M590_RESPONSE_OK, _responseBuffer, sizeof(_responseBuffer)))
        {
            for (i = 0; (i < sizeof(_responseBuffer)) && (_responseBuffer[i] != ','); i++)
            {
                tempString += _responseBuffer[i];
            }
            *rssi = tempString.toInt();
            tempString = "";
            
            if (i < sizeof(_responseBuffer))
            {
                for (++i/*continue after the separator*/; (i < sizeof(_responseBuffer)) && (_responseBuffer[i] != '\0'); i++)
                {
                    tempString += _responseBuffer[i];
                }
            }
            else
            {
                printDebug(F("Incomplete response"));
                return false;
            }

            *ber = tempString.toInt(); 

            return true;
        }
        else
        {
            printDebug(F("CSQ Error"));
            return false;
        }

    }
    else
    {
        printDebug(F("NOT connected"));
        return false;
    }
}

bool M590::sendSMS(const char * phoneNumberString, const char * messageString)
{
    /* Check the connected state */
    if (_currentState == M590_STATE_CELLULAR_CONNECTED)
    {
        /** Step 1: Set SMS mode to text mode with AT+CMGF=1 expect OK**/
        sendCommand(M590_COMMAND_SET_MODEM_TEXT_MODE);
        if (M590_SUCCESS != readForResponses(M590_RESPONSE_OK, M590_RESPONSE_ERROR))
        {
            printDebug(F("CMGF Error"));
            return false;
        }

        /** Step 2: Set modem character set to ‘GSM’ with AT+CSCS="GSM" expect OK **/
        sendCommand(M590_COMMAND_SET_MODEM_GSM_CHARSET);
        if (M590_SUCCESS != readForResponses(M590_RESPONSE_OK, M590_RESPONSE_ERROR))
        {
            printDebug(F("CSCS Error"));
            return false;
        }

        /** Step 3: Set the target phone number, wait for the ">" symbol to appear **/
        /*Build up the CMGS  command */

        _gsmSerial->print((__FlashStringHelper *)M590_COMMAND_PREFIX);
        _gsmSerial->print((__FlashStringHelper *)M590_COMMAND_SMS_PHONE_NR_START);
        _gsmSerial->print(phoneNumberString);
        _gsmSerial->print((__FlashStringHelper *)M590_COMMAND_SMS_PHONE_NR_END);
        _gsmSerial->println();

        if (M590_SUCCESS != readForResponses(M590_RESPONSE_TEXT_NPUT, M590_RESPONSE_ERROR))
        {
            printDebug(F("CMGS Error"));
            return false;
        }

        /** Step 4: Send the message, write terminating character "Ctrl+Z" (0x1A) to finalize the message **/
        _gsmSerial->print(messageString);
        _gsmSerial->write(0x1A);

        /** Step 5: Wait for confirmation , asynchronously? **/
        if (M590_SUCCESS != readForResponses(M590_RESPONSE_OK, M590_RESPONSE_ERROR, 9000))
        {
            printDebug(F("CMGW Error"));
            return false;
        }

    }
    else
    {
        printDebug(F("NOT connected"));
        return false;
    }

    return true;
}


bool M590::readSMS(byte index, char * smsBuffer, byte smsBufferSize)
{
    m590ResponseCode resp;
    char indexString[3];
    /* Convert index to string */
    sprintf(indexString, "%d", index);

    /* Check the connected state */
    if (_currentState == M590_STATE_CELLULAR_CONNECTED)
    {
        /** Send the CMGR message to read, expect OK **/
        sendCommand(M590_COMMAND_SMS_READ, indexString);
        if (M590_SUCCESS != readForSMS(M590_RESPONSE_OK, M590_RESPONSE_ERROR, smsBuffer, smsBufferSize))
        {
            printDebug(F("CMGR Error"));
            return false;
        }
    }
    else
    {
        printDebug(F("NOT connected"));
        return false;
    }

    return true;
}

bool M590::readParsedSMS_Async(byte index, char * smsText, unsigned int smsTextSize, char * smsStatus, unsigned int smsStatusSize,
    char * smsSender, unsigned int smsSenderSize, char * smsDate, unsigned int smsDateSize, void(*notificationCbk)())
{
    m590ResponseCode resp;
    char indexString[3];
    /* Convert index to string */
    sprintf(indexString, "%d", index);

    /* Check the connected state */
    if (_currentState == M590_STATE_CELLULAR_CONNECTED)
    {
        if (_commandState == M590_STATE_IDLE)
        {
            /* Parameter check */
            if ((index == 0) || (notificationCbk == NULL) || (smsText == NULL) || (smsStatus == NULL) || (smsSender == NULL) || (smsDate == NULL) )
            {
                printDebug(F("Invalid parameters"));
                return false;
            }

            /* Prepare asynchronous handler */
            _commandState = M590_STATE_CMGR_START;
            _smsParseState = M590_SMS_CMGR;
            _handlerCbk = &handlerParsedCMGR;

            /* Assign SMS related buffers and sizes */
            _smsStatus = smsStatus;
            _smsStatusSize = smsStatusSize;
            _smsSender = smsSender;
            _smsSenderSize = smsSenderSize;
            _smsDate = smsDate;
            _smsDateSize = smsDateSize;
            _smsBuffer = smsText;
            _smsBufferSize = smsTextSize;

            /* Assign notification Callback */
            _notificationCbk = notificationCbk;

            /** Send the CMGR message to read, expect OK **/
            sendCommand(M590_COMMAND_SMS_READ, indexString);
            //printDebug(F("CMGR sent"));
        }
        else
        {
            printDebug(F("Busy"));
            return false;
        }

    }
    else
    {
        printDebug(F("NOT connected"));
        return false;
    }

    return true;
}

bool M590::readSMS_Async(byte index, char * smsBuffer, unsigned int smsBufferSize, void(*notificationCbk)())
{
    m590ResponseCode resp;
    char indexString[3];
    /* Convert index to string */
    sprintf(indexString, "%d", index);

    /* Check the connected state */
    if (_currentState == M590_STATE_CELLULAR_CONNECTED)
    {
        if (_commandState == M590_STATE_IDLE)
        {
            /* Parameter check */
            if ((index == 0) || (notificationCbk == NULL) || (smsBuffer == NULL) || (smsBufferSize == 0))
            {
                printDebug(F("Invalid parameters"));
                return false;
            }

            /* Prepare asynchronous handler */
            _commandState = M590_STATE_CMGR_START;
            _handlerCbk = &handlerCMGR;

            /* Assign SMS buffer and size */
            _smsBuffer = smsBuffer;
            _smsBufferSize = smsBufferSize;

            /* Assign notification Callback */
            _notificationCbk = notificationCbk;

            /** Send the CMGR message to read, expect OK **/
            sendCommand(M590_COMMAND_SMS_READ, indexString);
            //printDebug(F("CMGR sent"));
        }
        else
        {
            printDebug(F("Busy"));
            return false;
        }

    }
    else
    {
        printDebug(F("NOT connected"));
        return false;
    }

    return true;
}

bool M590::readSMSList_Async(m590SMSreadFlags readFlag, char * smsBuffer, unsigned int smsBufferSize, void(*notificationCbk)())
{
    m590ResponseCode resp;
    char readflagString[2];
    /* Convert index to string */
    sprintf(readflagString, "%d", readFlag);

    /* Check the connected state */
    if (_currentState == M590_STATE_CELLULAR_CONNECTED)
    {
        if (_commandState == M590_STATE_IDLE)
        {
            /* Parameter check */
            if ((readFlag > 4) || (notificationCbk == NULL) || (smsBuffer == NULL) || (smsBufferSize == 0))
            {
                printDebug(F("Invalid parameters"));
                return false;
            }

            /* Prepare asynchronous handler */
            _commandState = M590_STATE_CMGL_START;
            _handlerCbk = &handlerCMGL;

            /* Assign SMS buffer and size */
            _smsBuffer = smsBuffer;
            _smsBufferSize = smsBufferSize;

            /* Assign notification Callback */
            _notificationCbk = notificationCbk;

            /** Send the CMGR message to read, expect OK **/
            sendCommand(M590_COMMAND_SMS_READ_LIST, readflagString);
            //printDebug(F("CMGR sent"));
        }
        else
        {
            printDebug(F("Busy"));
            return false;
        }

    }
    else
    {
        printDebug(F("NOT connected"));
        return false;
    }

    return true;
}

bool M590::enableNewSMSNotification(void(*newSMSnotificationCbk)(byte index))
{

    m590ResponseCode resp;

    /* Check the connected state */
    if (_currentState == M590_STATE_CELLULAR_CONNECTED)
    {
        if (_commandState == M590_STATE_IDLE)
        {
            /* Parameter check */
            if ((newSMSnotificationCbk == NULL))
            {
                printDebug(F("Invalid parameters"));
                return false;
            }

            /* Enable SMS Notification by sending AT+CNMI=2,1,0,0,0 */
            sendCommand(M590_COMMAND_SMS_NOTIFICATION);
            resp = readForResponses(M590_RESPONSE_OK, M590_RESPONSE_ERROR);

            Serial.println(resp);

            if (M590_SUCCESS != resp)
            {
                printDebug(F("CNMI Error"));
                return false;
            }

            /* Assign new SMS notification Callback */
            _newSMSnotificationCbk = newSMSnotificationCbk;

            /* Set the Idle Task to watch for new SMS */
            _IdleTask = &handlerSMSNotification;

            printDebug(F("SMS Notification enabled"));
        }
        else
        {
            printDebug(F("Busy"));
            return false;
        }

    }
    else
    {
        printDebug(F("NOT connected"));
        return false;
    }

    return true;
}

bool M590::disableNewSMSNotification()
{

    /* Check the connected state */
    if (_currentState == M590_STATE_CELLULAR_CONNECTED)
    {
        if (_commandState == M590_STATE_IDLE)
        {

            /* Assign new SMS notification Callback */
            _newSMSnotificationCbk = NULL;

            /* Set the Idle Task to handle gateway functionality */
            _IdleTask = &handlerGateway;
            returnToIdle();

            printDebug(F("SMS Notification disabled"));
        }
        else
        {
            printDebug(F("Busy"));
            return false;
        }

    }
    else
    {
        printDebug(F("NOT connected"));
        return false;
    }

    return true;
}

bool M590::setSMSTextModeCharSetGSM()
{
    /* Check the connected state */
    if (_currentState == M590_STATE_CELLULAR_CONNECTED)
    {
        /** Step 1: Set SMS mode to text mode with AT+CMGF=1 expect OK**/

        sendCommand(M590_COMMAND_SET_MODEM_TEXT_MODE);
        if (M590_SUCCESS != readForResponses(M590_RESPONSE_OK, M590_RESPONSE_ERROR))
        {
            printDebug(F("CMGF Error"));
            return false;
        }

        /** Step 2: Set modem character set to ‘GSM’ with AT+CSCS="GSM" expect OK **/

        sendCommand(M590_COMMAND_SET_MODEM_GSM_CHARSET);
        if (M590_SUCCESS != readForResponses(M590_RESPONSE_OK, M590_RESPONSE_ERROR))
        {
            printDebug(F("CSCS Error"));
            return false;
        }
    }
    else
    {
        printDebug(F("NOT connected"));
        return false;
    }

    return true;
}


bool M590::deleteSMS(byte index, byte deleteFlag)
{
    char parameter[5];

    /* Check the connected state */
    if (_currentState == M590_STATE_CELLULAR_CONNECTED)
    {   
        /** Delete the requested entry, expect OK **/

        /* Build up the parameter list */
        sprintf(parameter, "%d", index);
        strcpy(parameter+strlen(parameter), ",");
        sprintf(parameter + strlen(parameter), "%d", deleteFlag);

        /* Send the AT command for deletion */
        sendCommand(M590_COMMAND_SMS_DELETE, parameter);
        if (M590_SUCCESS != readForResponses(M590_RESPONSE_OK, M590_RESPONSE_ERROR))
        {
            printDebug(F("CMGD Error"));
            return false;
        }
    }
    else
    {
        printDebug(F("NOT connected"));
        return false;
    }
    return true;
}


m590ResponseCode M590::readForSMS(const char *progmemResponseString, const char *progmemFailString, char *buffer, const unsigned int max_bytes,
    const unsigned int timeout) {
    byte passMatched = 0, failMatched = 0, dataRead = 0;
    byte passResponseLength = strlen_P(progmemResponseString);
    byte failResponseLength = strlen_P(progmemFailString);
    unsigned long startTime = millis();

    while (millis() < (startTime + timeout)) {
        if (_gsmSerial->available()) {
            char c = (char)_gsmSerial->read();

            buffer[dataRead] = c;
            dataRead++;

            if (dataRead >= (max_bytes - 1))
            { //if read more than buffer size
                buffer[dataRead] = 0;
                return M590_LENGTH_EXCEEDED;
            }

            //check for pass
            if (c == pgm_read_byte_near(progmemResponseString + passMatched)) {
                passMatched++;
                if (passMatched == passResponseLength) {
                    return M590_SUCCESS;
                }
            }
            else
                passMatched = 0;

            //check for fail
            if (c == pgm_read_byte_near(progmemResponseString + failMatched)) {
                failMatched++;
                if (failMatched == failResponseLength) {
                    buffer = NULL;
                    return M590_FAILURE;
                }
            }
            else
                failMatched = 0;
        }
    }
    //timeout reached
    return M590_TIMEOUT;
}

bool M590::sendUSSD(const char * ussdString)
{
    /* Check the connected state */
    if (_currentState == M590_STATE_CELLULAR_CONNECTED)
    {
        /** Step 1: Set SMS mode to text mode with AT+CMGF=1 expect OK**/
        sendCommand(M590_COMMAND_SET_MODEM_TEXT_MODE);
        if (M590_SUCCESS != readForResponses(M590_RESPONSE_OK, M590_RESPONSE_ERROR))
        {
            printDebug(F("CMGF Error"));
            return false;
        }

        /** Step 2: Set modem character set to ‘GSM’ with AT+CSCS="GSM" expect OK **/
        sendCommand(M590_COMMAND_SET_MODEM_GSM_CHARSET);
        if (M590_SUCCESS != readForResponses(M590_RESPONSE_OK, M590_RESPONSE_ERROR))
        {
            printDebug(F("CSCS Error"));
            return false;
        }

        /** Step 3: Send the USSD message, expect OK **/
        /*Build up the CUSD  command */

        _gsmSerial->print((__FlashStringHelper *)M590_COMMAND_PREFIX);
        _gsmSerial->print((__FlashStringHelper *)M590_COMMAND_USSD_START);
        _gsmSerial->print(ussdString);
        _gsmSerial->print((__FlashStringHelper *)M590_COMMAND_USSD_END);
        _gsmSerial->println();

        if (M590_SUCCESS != readForResponses(M590_RESPONSE_OK, M590_RESPONSE_ERROR, 9000))
        {
            printDebug(F("CUSD Error"));
            return false;
        }

    }
    else
    {
        printDebug(F("NOT connected"));
        return false;
    }

    return true;
}

bool M590::SendUSSD_Async(const char * ussdString, void (*notificationCbk)())
{
    /* Check the connected state */
    if (_currentState == M590_STATE_CELLULAR_CONNECTED )
    {
        if (_commandState == M590_STATE_IDLE)
        {
            /* Prepare asynchronous handler */
            _commandState = M590_STATE_USSD_START;
            _handlerCbk = &handlerCUSD;

            /* Assign notification Callback */
            _notificationCbk = notificationCbk;

            /** Send the USSD message, expect OK in handlerCUSD() **/
            /*Build up the CUSD  command */
            //@Todo: improve complex command sending
            _gsmSerial->print((__FlashStringHelper *)M590_COMMAND_PREFIX);
            _gsmSerial->print((__FlashStringHelper *)M590_COMMAND_USSD_START);
            _gsmSerial->print(ussdString);
            _gsmSerial->print((__FlashStringHelper *)M590_COMMAND_USSD_END);
            _gsmSerial->println();
        }
        else
        {
            printDebug(F("Busy"));
            return false;
        }

    }
    else
    {
        printDebug(F("NOT connected"));
        return false;
    }

    return true;
}

void M590::sendCommand(const char *progmemCommand, const char *params) {
    //need to cast to FlashStringHelper for it to correctly read from progmem
    _gsmSerial->print((__FlashStringHelper *) M590_COMMAND_PREFIX);
    MONITOR((__FlashStringHelper *)M590_COMMAND_PREFIX);

    sendCommandWithoutPrefix(progmemCommand, params);
}

void M590::sendCommandWithoutPrefix(const char *progmemCommand, const char *params) {
    _gsmSerial->print((__FlashStringHelper *) progmemCommand);
    MONITOR((__FlashStringHelper *)progmemCommand);

    if (params && strlen(params))
    {
        _gsmSerial->print(params);
        MONITOR(params);
    }
    _gsmSerial->println(); //terminate with CLRF
    MONITOR_NL(); //terminate with CLRF
}

void M590::resetAsyncVariables() {
    _asyncStartTime = 0;
    _asyncBytesMatched = 0;
    _asyncResponseLength = 0;
    _asyncProgmemResponseString = NULL;
    _asyncResponseIndex = 0;
}

m590ResponseCode M590::readForAsyncResponse(const char *progmemResponseString, const unsigned int timeout) {
    if (_asyncStartTime == 0)
        _asyncStartTime = millis();
    if (!_asyncProgmemResponseString && progmemResponseString) 
    {
        //save responseString pointer to look for in private variable (so async function can be called without parameters)
        _asyncProgmemResponseString = progmemResponseString;
        _asyncResponseLength = strlen_P(progmemResponseString);
    } else if (!_asyncProgmemResponseString && !progmemResponseString)
    {
        //return when function is called for the first time without any parameters
        return M590_NO_PARAMETERS;
    }

    if (millis() > _asyncStartTime + timeout)
    {
        resetAsyncVariables();
        return M590_TIMEOUT;
    }

    while (_gsmSerial->available()) {
        char c = (char) _gsmSerial->read();
        if (c == pgm_read_byte_near(_asyncProgmemResponseString + _asyncBytesMatched)) {
            _asyncBytesMatched++;
            if (_asyncBytesMatched == _asyncResponseLength) {
                resetAsyncVariables();
                return M590_SUCCESS;
            }
        } else
            _asyncBytesMatched = 0;
    }
    return M590_ASYNC_RUNNING;
}

m590ResponseCode M590::checkForNewSMS() {

    if (_newSMSArrived)
    {/*Get the index number */
        while (_gsmSerial->available())
        {
            char c = (char)_gsmSerial->read();

            if (c == '\r')
            {
                _responseBuffer[_asyncResponseIndex] = 0;
                resetAsyncVariables();
                _newSMSArrived = false;
                return M590_SUCCESS;
            }
            else
            {
                _responseBuffer[_asyncResponseIndex++] = c;
                return M590_ASYNC_RUNNING;
            }
        }
        return M590_ASYNC_RUNNING;
    }
    else
    {/* Wait for new SMS  */
        _asyncResponseLength = strlen_P(M590_RESPONSE_CMTI);

        while (_gsmSerial->available()) {
            char c = (char)_gsmSerial->read();
            if (c == pgm_read_byte_near(_asyncNewSMString + _asyncBytesMatched)) {
                _asyncBytesMatched++;
                if (_asyncBytesMatched == _asyncResponseLength) {
                    resetAsyncVariables();
                    _newSMSArrived = true;
                    return M590_ASYNC_RUNNING;
                }
            }
            else
                _asyncBytesMatched = 0;
        }
    }

    return M590_ASYNC_RUNNING;
}


m590ResponseCode M590::readForSMS_Async(const char *progmemResponseString, const unsigned int timeout) {

    if (_asyncStartTime == 0)
        _asyncStartTime = millis();
    if (!_asyncProgmemResponseString && progmemResponseString)
    {
        //save responseString pointer to look for in private variable (so async function can be called without parameters)
        _asyncProgmemResponseString = progmemResponseString;
        _asyncResponseLength = strlen_P(progmemResponseString);
    }
    else if (!_asyncProgmemResponseString && !progmemResponseString)
    {
        //return when function is called for the first time without any parameters
        return M590_NO_PARAMETERS;
    }

    if (millis() > _asyncStartTime + timeout)
    {
        resetAsyncVariables();
        return M590_TIMEOUT;
    }

    while (_gsmSerial->available()) {
        char c = (char)_gsmSerial->read();

        _smsBuffer[_asyncResponseIndex++] = c;

        if (_asyncResponseIndex >= (_smsBufferSize - 1))
        { //if read more than buffehr size
            _smsBuffer[_asyncResponseIndex] = 0;
            resetAsyncVariables();
            return M590_LENGTH_EXCEEDED;
        }

        if (c == pgm_read_byte_near(_asyncProgmemResponseString + _asyncBytesMatched)) {
            _asyncBytesMatched++;
            if (_asyncBytesMatched == _asyncResponseLength) {

                /* Remove the:
                \r\n\r\nOK\r\n from the end of the buffer */
              
                for (int i = 0; i <= strlen_P(_asyncProgmemResponseString); i++)
                {
                    _smsBuffer[_asyncResponseIndex-i] = 0;
                }

                resetAsyncVariables();
                return M590_SUCCESS;
            }
        }
        else
            _asyncBytesMatched = 0;
    }
    return M590_ASYNC_RUNNING;
}

m590ResponseCode M590::parseSMS() 
{
    m590ResponseCode response;
    switch (_smsParseState)
    {
        case M590_SMS_CMGR:
            /* Read until +CMGR: " */
            response = readForAsyncResponse(M590_RESPONSE_CMGR);
            if (M590_SUCCESS == response)
            {
                _smsParseState = M590_SMS_STATUS;
            }
            else if (M590_ASYNC_RUNNING != response)
            {
                _smsParseState = M590_SMS_COMPLETED;
                printDebug(F("SMS parse fail at CMGR"));
                return M590_FAILURE;
            }
            break;

        case M590_SMS_STATUS:

            response = serialToBuffer_Async(_smsStatus, M590_RESPONSE_QUOTE, _smsStatusSize);

            if (M590_SUCCESS == response)
            {
                _smsParseState = M590_SMS_STATUS_QUOTE;

            }
            else if (M590_ASYNC_RUNNING != response)
            {
                _smsParseState = M590_SMS_COMPLETED;
                printDebug(F("SMS parse fail at STATUS"));
                return M590_FAILURE;
            }
            break;

        case M590_SMS_STATUS_QUOTE:

            response = readUntil_Async(M590_RESPONSE_QUOTE);

            if (M590_SUCCESS == response)
            {
                _smsParseState = M590_SMS_SENDER;
            }
            else if (M590_ASYNC_RUNNING != response)
            {
                _smsParseState = M590_SMS_COMPLETED;
                printDebug(F("SMS parse fail at STATUS_QUOTE"));
                return M590_FAILURE;
            }
            break;

        case M590_SMS_SENDER:

            response = serialToBuffer_Async(_smsSender, M590_RESPONSE_QUOTE, _smsSenderSize);

            if (M590_SUCCESS == response)
            {
                _smsParseState = M590_SMS_SENDER_COMMA;
            }
            else if (M590_ASYNC_RUNNING != response)
            {
                _smsParseState = M590_SMS_COMPLETED;
                printDebug(F("SMS parse fail at SENDER"));
                return M590_FAILURE;
            }
            break;

        case M590_SMS_SENDER_COMMA:

            response = readUntil_Async(M590_RESPONSE_COMMA);

            if (M590_SUCCESS == response)
            {
                _smsParseState = M590_SMS_SENDER_2ND_COMMA;
            }
            else if (M590_ASYNC_RUNNING != response)
            {
                _smsParseState = M590_SMS_COMPLETED;
                printDebug(F("SMS parse fail at SENDER_COMMA"));
                return M590_FAILURE;
            }
            break;

        case M590_SMS_SENDER_2ND_COMMA:

            response = readUntil_Async(M590_RESPONSE_COMMA);

            if (M590_SUCCESS == response)
            {
                _smsParseState = M590_SMS_SENDER_QUOTE;
            }
            else if (M590_ASYNC_RUNNING != response)
            {
                _smsParseState = M590_SMS_COMPLETED;
                printDebug(F("SMS parse fail at SENDER_2ND_COMMA "));
                return M590_FAILURE;
            }
            break;

        case M590_SMS_SENDER_QUOTE:

            response = readUntil_Async(M590_RESPONSE_QUOTE);

            if (M590_SUCCESS == response)
            {
                _smsParseState = M590_SMS_DATE;
            }
            else if (M590_ASYNC_RUNNING != response)
            {
                _smsParseState = M590_SMS_COMPLETED;
                printDebug(F("SMS parse fail at SENDER_QUOTE "));
                return M590_FAILURE;
            }
            break;

        case M590_SMS_DATE:

            response = serialToBuffer_Async(_smsDate, M590_RESPONSE_QUOTE, _smsDateSize);

            if (M590_SUCCESS == response)
            {
                _smsParseState = M590_SMS_DATE_NL;
            }
            else if (M590_ASYNC_RUNNING != response)
            {
                _smsParseState = M590_SMS_COMPLETED;
                printDebug(F("SMS parse fail at DATE"));
                return M590_FAILURE;
            }
            break; 

        case M590_SMS_DATE_NL:

            response = readUntil_Async(M590_RESPONSE_NL);

            if (M590_SUCCESS == response)
            {
                _smsParseState = M590_SMS_TEXT;
            }
            else if (M590_ASYNC_RUNNING != response)
            {
                _smsParseState = M590_SMS_COMPLETED;
                printDebug(F("SMS parse fail at DATE_NL"));
                return M590_FAILURE;
            }
            break;

        case M590_SMS_TEXT:

            //response = readForSMS_Async(M590_RESPONSE_SMS_END);
            response = serialToBuffer_Async(_smsBuffer, M590_RESPONSE_SMS_END, _smsBufferSize);
            if (M590_SUCCESS == response)
            {
                _smsParseState = M590_SMS_COMPLETED;
            }
            else if (M590_ASYNC_RUNNING != response)
            {
                _smsParseState = M590_SMS_COMPLETED;
                printDebug(F("SMS parse fail at TEXT"));
                return M590_FAILURE;
            }
            break;

        case M590_SMS_COMPLETED:
            return M590_SUCCESS;
            break;

        default:
            printDebug(F("Invalid SMS parse state"));
            return M590_UNDEFINED;
            break;
    }

    return M590_ASYNC_RUNNING;
}

m590ResponseCode M590::readForResponse(const char *progmemResponseString, char *buffer, const unsigned int max_bytes,
                                       const unsigned int timeout) {
    byte dataMatched = 0, dataRead = 0;
    byte readingData = 0; //state
    byte dataLength = strlen_P(M590_RESPONSE_PREFIX);
    byte matched = 0;
    byte responseLength = strlen_P(progmemResponseString);

    unsigned long startTime = millis();

    while (millis() < (startTime + timeout)) 
    {
        if (_gsmSerial->available()) 
        {
            char c = (char) _gsmSerial->read();
            
            if (c == pgm_read_byte_near(M590_RESPONSE_PREFIX + dataMatched)) 
            { //check if a return data message begins
                dataMatched++;
                if (dataMatched == dataLength) 
                { //when return data begins
                    readingData = 1;
                    dataMatched = 0;
                }
            }
            else 
            {
                dataMatched = 0;
            }

            if (readingData) 
            { //if in reading return data mode

                if (readingData == 1 && c == ':') //before colon, there is only the command echoed back
                { 
                    readingData = 2;
                }
                else if (readingData == 2)
                { //if at actual data
                    //_debugSerial->print(c);
                    if (c == '\r')
                    { //if reached end of return data
                        readingData = 0;
                    }
                    else
                    {
                        buffer[dataRead] = c;
                        dataRead++;
                        if (dataRead >= (max_bytes - 1))
                        { //if read more than buffer size
                            buffer[dataRead] = 0;
                            return M590_LENGTH_EXCEEDED;
                        }
                    }
                 }
            } 
            else 
            {
                if (c == pgm_read_byte_near(progmemResponseString + matched)) 
                {
                    matched++;
                    if (matched == responseLength) 
                    {
                        return M590_SUCCESS;
                    }
                } 
                else
                { 
                    matched = 0;
                }

            }
        }
    }
    //timeout reached
    return M590_TIMEOUT;
}

//two counters/indexes
m590ResponseCode
M590::readForResponses(const char *progmemResponseString, const char *progmemFailString, const unsigned int timeout) {
    byte passMatched = 0, failMatched = 0;
    byte passResponseLength = strlen_P(progmemResponseString);
    byte failResponseLength = strlen_P(progmemFailString);
    unsigned long startTime = millis();

    while (millis() < (startTime + timeout)) {
        if (_gsmSerial->available()) {
            char c = (char) _gsmSerial->read();
            //_debugSerial->print(c);

            //check for pass
            if (c == pgm_read_byte_near(progmemResponseString + passMatched)) {
                passMatched++;
                if (passMatched == passResponseLength) {
                    return M590_SUCCESS;
                }
            } else
                passMatched = 0;

            //check for fail
            if (c == pgm_read_byte_near(progmemResponseString + failMatched)) {
                failMatched++;
                if (failMatched == failResponseLength) {
                    return M590_FAILURE;
                }
            } else
                failMatched = 0;
        }
    }
    //timeout reached
    return M590_TIMEOUT;
}

m590ResponseCode
M590::serialToBuffer(char *buffer, const char readUntil, const unsigned int maxBytes, const unsigned int timeout) {
    unsigned long startTime = millis();
    unsigned int bytesRead = 0;

    while (millis() < (startTime + timeout)) {
        if (_gsmSerial->available()) {
            buffer[bytesRead] = (char) _gsmSerial->read();

            //check for readUntil character and replace with 0 (null termination)
            if (buffer[bytesRead] == readUntil) {
                buffer[bytesRead] = 0;
                return M590_SUCCESS;
            }

            bytesRead++;

            if (bytesRead >= (maxBytes - 1)) {
                buffer[bytesRead] = 0;
                return M590_LENGTH_EXCEEDED;
            }
        }
    }
    return M590_TIMEOUT;
}

m590ResponseCode
M590::serialToBuffer_Async(char *buffer, const char readUntil, const unsigned int maxBytes, const unsigned int timeout)
{
    if (_asyncStartTime == 0)
    {
        _asyncStartTime = millis();
    }

    if (millis() > _asyncStartTime + timeout)
    {
        resetAsyncVariables();
        return M590_TIMEOUT;
    }

    while (_gsmSerial->available()) {
        buffer[_asyncResponseIndex] = (char)_gsmSerial->read();

        //check for readUntil character and replace with 0 (null termination)
        if (buffer[_asyncResponseIndex] == readUntil) {
            buffer[_asyncResponseIndex] = 0;
            resetAsyncVariables();
            return M590_SUCCESS;
        }

        _asyncResponseIndex++;

        if (_asyncResponseIndex >= (maxBytes - 1)) {
            buffer[_asyncResponseIndex] = 0;
            return M590_LENGTH_EXCEEDED;
        }
    }
    return M590_ASYNC_RUNNING;
}

m590ResponseCode
M590::serialToBuffer_Async(char *buffer, const char *progmemResponseString, const unsigned int maxBytes, const unsigned int timeout)
{
    if (_asyncStartTime == 0)
    {
        _asyncStartTime = millis();
    }

    if (!_asyncProgmemResponseString && progmemResponseString)
    {
        //save responseString pointer to look for in private variable (so async function can be called without parameters)
        _asyncProgmemResponseString = progmemResponseString;
        _asyncResponseLength = strlen_P(progmemResponseString);
    }
    else if (!_asyncProgmemResponseString && !progmemResponseString)
    {
        //return when function is called for the first time without any parameters
        return M590_NO_PARAMETERS;
    }

    if (millis() > _asyncStartTime + timeout)
    {
        resetAsyncVariables();
        return M590_TIMEOUT;
    }

    while (_gsmSerial->available()) {
        char c = (char)_gsmSerial->read();

        buffer[_asyncResponseIndex++] = c;

        if (_asyncResponseIndex >= (maxBytes - 1))
        { //if read more than buffehr size
            buffer[_asyncResponseIndex] = 0;
            resetAsyncVariables();
            return M590_LENGTH_EXCEEDED;
        }

        if (c == pgm_read_byte_near(_asyncProgmemResponseString + _asyncBytesMatched)) {
            _asyncBytesMatched++;
            if (_asyncBytesMatched == _asyncResponseLength) {

                /* Remove the end char sequence from the end of the buffer , for example the SMS end:
                \r\n\r\nOK\r\n */

                for (int i = 0; i <= strlen_P(_asyncProgmemResponseString); i++)
                {
                    buffer[_asyncResponseIndex - i] = 0;
                }

                resetAsyncVariables();
                return M590_SUCCESS;
            }
        }
        else
            _asyncBytesMatched = 0;
    }
    return M590_ASYNC_RUNNING;
}

m590ResponseCode M590::readUntil(const char readUntil, const unsigned int timeout) {
    unsigned long startTime = millis();

    while (millis() < (startTime + timeout)) {
        if (_gsmSerial->available()) {
            if (readUntil == _gsmSerial->read()) {
                return M590_SUCCESS;
            }
        }
    }
    return M590_TIMEOUT;
}

m590ResponseCode M590::readUntil_Async(const char readUntil, const unsigned int timeout)
{
    if (_asyncStartTime == 0)
    {
        _asyncStartTime = millis();
    }

    if (millis() > _asyncStartTime + timeout)
    {
        resetAsyncVariables();
        return M590_TIMEOUT;
    }

    while (_gsmSerial->available()) 
    {
        if (_gsmSerial->read() == readUntil)
        {
            resetAsyncVariables();
            return M590_SUCCESS;
        }
    }
    return M590_ASYNC_RUNNING;
}

bool M590::bufferStartsWithProgmem(char *buffer, const char *progmemString) {
    bool matches = true;
    for (int i = 0; i < strlen_P(progmemString); i++) {
        matches = buffer[i] == pgm_read_byte_near(progmemString + i);
    }
    return matches;
}

void M590::printDebug(const char *progmemString, bool withNewline) {
    if (_debugSerial) {
        _debugSerial->print((__FlashStringHelper *) progmemString);
        if (withNewline) _debugSerial->println();
    }
}

void M590::printDebug(const String s, bool withNewline) {
    if (_debugSerial) {
        _debugSerial->print(s);
        if (withNewline) _debugSerial->println();
    }
}

void M590::returnToIdle()
{
    _commandState = M590_STATE_IDLE;
    _handlerCbk = _IdleTask;
    printDebug(F("#Return to Idle()"));
}


void M590::handlerGateway()
{
    if (_commandState == M590_STATE_IDLE)
    {
        while (Serial3.available() > 0) {
            Serial.write(Serial3.read());
        }
        while (Serial.available() > 0) {
            Serial3.write(Serial.read());
        }
    }
    else
    {
        printDebug(F("#Invalid state for Idle()"));
    }
}

void M590::handlerSMSNotification()
{
    m590ResponseCode response;
    int index;

    if (_commandState == M590_STATE_IDLE)
    {
        response = checkForNewSMS();
        
        switch (response)
        {
            case M590_SUCCESS:
                index = atoi(_responseBuffer);

                if (_newSMSnotificationCbk != NULL)
                {
                    _newSMSnotificationCbk(index);
                }

                break;

            case M590_ASYNC_RUNNING:
                // Do nothing 
                break;

            default:
                // should never reach here 
                printDebug(F("#CMTI: Invalid response"));
                break;
        }
    }
    else
    {
        printDebug(F("#Invalid state for Idle()"));
    }
}


void M590::handlerCUSD(){
    m590ResponseCode response;

    char command[15];

    switch (_commandState)
    {
        case M590_STATE_USSD_START:
        /** Set SMS mode to text mode with AT+CMGF=1 expect OK **/
        if (M590_SUCCESS == readForAsyncResponse(M590_RESPONSE_OK))
        {
            //printDebug(F("CUSD Completed instantly"));
            returnToIdle();
            
            /* Invoke notification callback */
            if (_notificationCbk != NULL)
            {
                _notificationCbk();
            }
        }
        else
        {
            _commandState = M590_STATE_USSD_RUN;
            //printDebug(F("USSD_RUN mode set"));
        }
        break;

    case M590_STATE_USSD_RUN:
        /**  Response waiting for CMGF=1 expect OK **/
        response = readForAsyncResponse();
        if (M590_SUCCESS == response)
        {
            //printDebug(F("CUSD Completed finally"));
            returnToIdle();
            /* Invoke notification callback */
            if (_notificationCbk != NULL)
            {
                _notificationCbk();
            }
        }
        else if (M590_ASYNC_RUNNING == response)
        {
            //printDebug(F("."),false);
        }
        else{
            printDebug("CUSD Error");
            returnToIdle();
        }
        break;

    default:
        /* Invalid command state */
        printDebug(F("#Invalid state for CUSD()"));
        returnToIdle();
        break;
    }
}

void M590::handlerCMGR(){
    m590ResponseCode response;
    char command[15];

    switch (_commandState)
    {
    case M590_STATE_CMGR_START:
        /** Set SMS mode to text mode with AT+CMGF=1 expect OK **/

        //printDebug(F("CMGR_Start state"));

        if (M590_SUCCESS == readForSMS_Async(M590_RESPONSE_OK))
        {
            //printDebug(F("CMGR Completed instantly"));
            returnToIdle();

            /* Invoke notification callback */
            if (_notificationCbk != NULL)
            {
                _notificationCbk();
            }
        }
        else
        {
            _commandState = M590_STATE_CMGR_RUN;
            //printDebug(F("CMGR_RUN mode set"));
        }
        break;

    case M590_STATE_CMGR_RUN:
        /**  Response waiting for CMGR=1 expect OK **/
        response = readForSMS_Async();
        if (M590_SUCCESS == response)
        {
            //printDebug(F("CMGR Completed finally"));
            returnToIdle();
            /* Invoke notification callback */
            if (_notificationCbk != NULL)
            {
                _notificationCbk();
            }
        }
        else if (M590_ASYNC_RUNNING == response)
        {
            //printDebug(F("."), false);
        }
        else{
            printDebug("CMGR Error");
            returnToIdle();
        }
        break;

    default:
        /* Invalid command state */
        printDebug(F("#Invalid state for CMGR()"));
        returnToIdle();
        break;
    }
}

void M590::handlerParsedCMGR(){
    m590ResponseCode response;
    char command[15];

    switch (_commandState)
    {
    case M590_STATE_CMGR_START:
        /** Set SMS mode to text mode with AT+CMGF=1 expect OK **/

        //printDebug(F("CMGR_Start state"));

        if (M590_SUCCESS == parseSMS())
        {
            //printDebug(F("CMGR Completed instantly"));
            returnToIdle();

            /* Invoke notification callback */
            if (_notificationCbk != NULL)
            {
                _notificationCbk();
            }
        }
        else
        {
            _commandState = M590_STATE_CMGR_RUN;
            //printDebug(F("CMGR_RUN mode set"));
        }
        break;

    case M590_STATE_CMGR_RUN:
        /**  Response waiting for CMGR=1 expect OK **/
        response = parseSMS();
        if (M590_SUCCESS == response)
        {
            //printDebug(F("CMGR Completed finally"));
            returnToIdle();
            /* Invoke notification callback */
            if (_notificationCbk != NULL)
            {
                _notificationCbk();
            }
        }
        else if (M590_ASYNC_RUNNING == response)
        {
            //printDebug(F("."), false);
        }
        else{
            printDebug("CMGR Error");
            returnToIdle();
        }
        break;

    default:
        /* Invalid command state */
        printDebug(F("#Invalid state for CMGR()"));
        returnToIdle();
        break;
    }
}

void M590::handlerCMGL(){
    m590ResponseCode response;
    char command[15];

    switch (_commandState)
    {
    case M590_STATE_CMGL_START:

        printDebug(F("CMGL_Start state"));

        if (M590_SUCCESS == readForSMS_Async(M590_RESPONSE_OK))
        {
            printDebug(F("CMGL Completed instantly"));
            returnToIdle();

            /* Invoke notification callback */
            if (_notificationCbk != NULL)
            {
                _notificationCbk();
            }
        }
        else
        {
            _commandState = M590_STATE_CMGL_RUN;
            printDebug(F("CMGL_RUN mode set"));
        }
        break;

    case M590_STATE_CMGL_RUN:
        /**  Response waiting for CMGL expect OK **/
        response = readForSMS_Async();
        if (M590_SUCCESS == response)
        {
            printDebug(F("CMGL Completed finally"));
            returnToIdle();
            /* Invoke notification callback */
            if (_notificationCbk != NULL)
            {
                _notificationCbk();
            }
        }
        else if (M590_ASYNC_RUNNING == response)
        {
            //printDebug(F("."), false);
        }
        else{
            printDebug("CMGL Error");
            returnToIdle();
        }
        break;

    default:
        /* Invalid command state */
        printDebug(F("#Invalid state for CMGL()"));
        returnToIdle();
        break;
    }
}
