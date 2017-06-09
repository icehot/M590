#include "M590.h"

/* M590 class initialisation related functions */

/* The initialisation uses synchronous (blocking) modem access based on https://github.com/LeoDJ/M590 work */

/* Synchronous initialisation */

bool M590::init(unsigned long baudRate, HardwareSerial *gsmSerial, char* pin)
{
    bool retVal = false;
    bool continueInit = true;

    u32 startTime = millis();

    while ((millis() < startTime + INIT_TIMEOUT) && (continueInit == true))
    {
        switch (_initState)
        {
            case M590_UNINITIALIZED:
                if (gsmSerial)
                {/* Serial port for gsm is not NULL */
                    _gsmSerial = gsmSerial;
                    _gsmSerial->begin(baudRate);
                    _initState = M590_INIT_CHECK_ALIVE;
                }
                else
                {/* Serial port for gsm is NULL */
                    _gsmSerial = NULL;
                    _initState = M590_INIT_ERROR;
                    retVal = false;
                }
                break;

            case M590_INIT_CHECK_ALIVE:
                if (isAlive())
                {
                    _initState = M590_INIT_PIN_CHECK;
                }
                else
                {
                    printDebug(M590_ERROR_NOT_RESPONDING);
                    _initState = M590_INIT_ERROR;
                }

                break;

            case M590_INIT_PIN_CHECK:

                switch (checkPinRequired())
                {
                    case M590_PIN_NOT_REQUIRED:
                        _initState = M590_INIT_DONE;
                        break;

                    case M590_PIN_REQUIRED:
                        _initState = M590_INIT_INPUT_PIN;
                        break;

                    case M590_PIN_ERROR:
                        _initState = M590_INIT_ERROR;
                        break;

                    default:
                        /* Should never reach here */
                        printDebug(M590_ERROR_INVALID_STATE_STATE);
                        break;
                }
                
                break;

                //@Todo: Need to be tested
            case M590_INIT_INPUT_PIN:
               
                if (pin && pin != "")
                {
                    if (sendPinEntry(pin))
                    {
                        _initState = M590_INIT_PIN_VALIDATION;
                    }
                    else
                    {
                        printDebug(M590_ERROR_WRONG_PIN);
                        _initState = M590_INIT_ERROR;
                    }
                }
                else 
                {
                    printDebug(M590_ERROR_NO_PIN);
                    _initState = M590_INIT_ERROR;
                }
                break;

            case M590_INIT_PIN_VALIDATION:
                if (pinValidation())
                {
                    _initState = M590_INIT_DONE;
                }
                else
                {
                    printDebug(M590_ERROR_PINVAL_TIMEOUT);
                    _initState = M590_INIT_ERROR;
                }
                break;

            case M590_INIT_DONE:
                /* Successfull initialization*/
                continueInit = false;
                retVal = true;
                break;

            case M590_INIT_ERROR:
                continueInit = false;
                retVal = false;
                break;

            default:
                /* Should never reach here */
                printDebug(M590_ERROR_INVALID_STATE_STATE);
                break;
        }
    }

    if (continueInit == true)
    {
        printDebug(M590_ERROR_INIT_TIMEOUT);
        retVal = false;
    }

    return retVal;
}

bool M590::waitForNetWork(const unsigned int timeout)
{
    unsigned long startTime = millis();

    while (millis() < startTime + timeout)
    {
        unsigned long curMillis = millis();

        if (_asyncStartTime == 0)
        {
            _asyncStartTime = curMillis; //repurpose asyncStartTime variable
        }
        else if (curMillis >= _asyncStartTime + STATUS_POLLING_RATE)
        {
            _networkState = checkNetworkState();

            if (_networkState == M590_NET_REGISTERED)
            {
                resetAsyncVariables();
                return true;
            }
            else
            {
                if (_networkState == M590_NET_SEARCHING_NOT_REGISTERED)
                {
                    printDebug(".",false); //print dots to show wait for registration
                }
                else
                {
                    printDebug(M590_ERROR_UNHANDLED_NET_STATE);
                    printDebug(String(_networkState), true);
                    resetAsyncVariables();
                    return false;
                }
            }
            _asyncStartTime = curMillis;
        }
    }

    resetAsyncVariables();
    printDebug(M590_ERROR_NETWORK_REG_TIMEOUT);
    return false;
}


/* Synchronous functionalities */

bool M590::isAlive()
{
    ResponseStateType resp;
    sendCommand(M590_COMMAND_AT, "");

    if (readForResponseSync(M590_RESPONSE_OK, M590_RESPONSE_ERROR) == M590_RESPONSE_SUCCESS)
        return true;
    else
        return false;
}

PinStateType M590::checkPinRequired()
{
    ResponseStateType response;

    PinStateType retVal = M590_PIN_ERROR;

    sendCommand(M590_COMMAND_CHECK_PIN, "");
    memset(_responseBuffer, 0, sizeof(_responseBuffer));
    response = readForResponseBufferedSync(M590_RESPONSE_OK, _responseBuffer, sizeof(_responseBuffer));

    if (response != M590_RESPONSE_SUCCESS)
    {
        retVal = M590_PIN_ERROR;
        return retVal;
    }

    bool required = bufferStartsWithProgmem(_responseBuffer, M590_RESPONSE_PIN_REQUIRED);

    if (required)
    {
        retVal = M590_PIN_REQUIRED;
    }

    bool alreadyReady = bufferStartsWithProgmem(_responseBuffer, M590_RESPONSE_PIN_NOT_REQUIRED);

    if (alreadyReady)
    {//check if module does not need pin entry
        retVal = M590_PIN_NOT_REQUIRED;
    }

    if (!required && !alreadyReady) {
        retVal = M590_PIN_ERROR;
        printDebug(M590_ERROR_OTHER_PIN_ERR);
    }

    return retVal;
}


bool M590::sendPinEntry(char* pin)
{
    char parameter[8];

    /* Build up the parameter list */
    strcpy(parameter, "\"");
    strcpy(parameter + 1, pin);
    strcpy(parameter + strlen(pin) + 1, "\"");

    sendCommand(M590_COMMAND_INPUT_PIN, parameter);

    if (readForResponseSync(M590_RESPONSE_OK, M590_RESPONSE_FAIL) == M590_RESPONSE_SUCCESS)
        return true;
    else
        return false;
}

bool M590::pinValidation()
{
    if (readForResponseSync(M590_RESPONSE_PIN_VAL_DONE, M590_RESPONSE_FAIL, PIN_VALIDATION_TIMEOUT) == M590_RESPONSE_SUCCESS)
        return true;
    else
        return false;
}

NetworkStateType M590::checkNetworkState()
{
    sendCommand(M590_COMMAND_CHECK_NETWORK_STATUS, "");
    memset(_responseBuffer, 0, sizeof(_responseBuffer));

    ResponseStateType response = readForResponseBufferedSync(M590_RESPONSE_OK, _responseBuffer, sizeof(_responseBuffer));

    /* the fourth char in the response (e.g. " 0,3") will be the registration state (e.g. 3) */
    if (response == M590_RESPONSE_SUCCESS)
        return (NetworkStateType)(_responseBuffer[3] - '0'); /* convert to integer, maps to NetworkStateType */
    else
        return M590_NET_PARSE_ERROR;
}


/* Synchronous Core functionality */


ResponseStateType M590::readForResponseSync(const char *progmemResponseString, const char *progmemFailString, const unsigned int timeout)
{
    byte passMatched = 0, failMatched = 0;
    byte passResponseLength = strlen_P(progmemResponseString);
    byte failResponseLength = strlen_P(progmemFailString);
    unsigned long startTime = millis();

    MONITOR("<< ");
    while (millis() < (startTime + timeout)) {
        if (_gsmSerial->available()) {
            char c = (char)_gsmSerial->read();
            MONITOR(c);

            //check for pass
            if (c == pgm_read_byte_near(progmemResponseString + passMatched)) {
                passMatched++;
                if (passMatched == passResponseLength) {
                    MONITOR("<< END");
                    MONITOR_NL();
                    return M590_RESPONSE_SUCCESS;
                }
            }
            else
                passMatched = 0;

            //check for fail
            if (c == pgm_read_byte_near(progmemFailString + failMatched)) {
                failMatched++;
                if (failMatched == failResponseLength) {
                    return M590_RESPONSE_FAILURE;
                }
            }
            else
                failMatched = 0;
        }
    }
    //timeout reached
    printDebug(M590_ERROR_RESPONSE_TIMEOUT);
    return M590_RESPONSE_TIMEOUT;
}

ResponseStateType M590::readForResponseBufferedSync(const char *progmemResponseString, char *buffer, const unsigned int max_bytes, const unsigned int timeout)
{
    byte dataMatched = 0, dataRead = 0;
    byte readingData = 0; //state
    byte dataLength = strlen_P(M590_RESPONSE_PREFIX);
    byte matched = 0;
    byte responseLength = strlen_P(progmemResponseString);

    unsigned long startTime = millis();

    MONITOR("<< ")

        while (millis() < (startTime + timeout))
        {
            if (_gsmSerial->available())
            {
                char c = (char)_gsmSerial->read();
                MONITOR(c);
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
                        readingData = 2;
                    else if (readingData == 2)
                    { //if at actual data
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
                                printDebug(M590_ERROR_RESPONSE_LENGTH_EXCEEDED);
                                return M590_RESPONSE_LENGTH_EXCEEDED;
                            }
                        }
                    }
                }
                else
                {
                    if (c == pgm_read_byte_near(progmemResponseString + matched))
                    {
                        matched++;
                        if (matched == responseLength) {
                            MONITOR("<< END");
                            MONITOR_NL();
                            return M590_RESPONSE_SUCCESS;
                        }
                    }
                    else
                        matched = 0;
                }
            }
        }
    //timeout reached
    printDebug(M590_ERROR_RESPONSE_TIMEOUT);
    return M590_RESPONSE_TIMEOUT;
}
