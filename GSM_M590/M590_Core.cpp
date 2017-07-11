#include "M590.h"

/* M590 class core functionality for asynchronous modem access */

M590::M590()
{
    _gsmSerial = NULL;
    _debugSerial = NULL;
}



void M590::resetAsyncVariables() {
    _asyncStartTime = 0;
    _asyncTimeout = 0;
    _asyncProgmemResponseString = NULL;
    _asyncResponseLength = 0;
    _asyncBytesMatched = 0;
    _asyncFailMatched = 0;
    _asyncResultType = M590_RES_NULL;
    _asyncResultptr = NULL;
    _asyncProcessState = 0;
    _asyncTempString = "";
    _asyncCommandCbk = NULL;
    _asyncInternalCbk = NULL;
}

void M590::sendCommand(const char *progmemCommand, String params) 
{
    if (_gsmSerial)
    {
        _gsmSerial->print((__FlashStringHelper *)progmemCommand);

        MONITOR(">> ");
        MONITOR((__FlashStringHelper *)progmemCommand);

        if (params && params.length())
        {
            _gsmSerial->print(params);
            MONITOR(params);
        }
        _gsmSerial->println(); //terminate with CLRF
         MONITOR_NL();
    }
    else
    {
        printDebug(M590_ERROR_NO_GSM_SERIAL);
    }
}

void M590::responseHandler()
{/* Cyclic function to read for responses asynchronously */
    switch (_responseState)
    {
        case M590_RESPONSE_RUNNING:
            _responseState = readForResponseAsync();
            break;

        case M590_RESPONSE_FAILURE:
            printDebug(M590_ERROR_RESPONSE_ERROR);
            if (_asyncInternalCbk != NULL)
            {
                (this->*_asyncInternalCbk)(M590_RESPONSE_FAILURE);
            }
            if (_asyncCommandCbk != NULL)
            {
                _asyncCommandCbk(M590_RESPONSE_FAILURE);
            }
            _responseState = M590_RESPONSE_PREPARE_IDLE;
            break;

        case M590_RESPONSE_TIMEOUT:
            printDebug(M590_ERROR_RESPONSE_TIMEOUT);
            if (_asyncInternalCbk != NULL)
            {
                (this->*_asyncInternalCbk)(M590_RESPONSE_TIMEOUT);
            }
            if (_asyncCommandCbk != NULL)
            {
                _asyncCommandCbk(M590_RESPONSE_TIMEOUT);
            }
            _responseState = M590_RESPONSE_PREPARE_IDLE;
            break;

        case M590_RESPONSE_LENGTH_EXCEEDED:
            printDebug(M590_ERROR_RESPONSE_LENGTH_EXCEEDED);
            if (_asyncInternalCbk != NULL)
            {
                (this->*_asyncInternalCbk)(M590_RESPONSE_LENGTH_EXCEEDED);
            }
            if (_asyncCommandCbk != NULL)
            {
                _asyncCommandCbk(M590_RESPONSE_LENGTH_EXCEEDED);
            }
            _responseState = M590_RESPONSE_PREPARE_IDLE;
            break;

        case M590_RESPONSE_SUCCESS:
            MONITOR("<< END");
            MONITOR_NL();

            /* Post handling for some requests */
            if (_asyncResultType == M590_RES_TCPSETUP0)
            {
                _socket[M590_GPRS_LINK_0].connected = true;
            }
            else if ((_asyncResultType == M590_RES_TCPSETUP1))
            {
                _socket[M590_GPRS_LINK_1].connected = true;
            }
            else
            {
                /* Do nothing, other case */
            }

            if (_asyncInternalCbk != NULL)
            {
                (this->*_asyncInternalCbk)(M590_RESPONSE_SUCCESS);
            }
            if (_asyncCommandCbk != NULL)
            {
                _asyncCommandCbk(M590_RESPONSE_SUCCESS);
            }
            _responseState = M590_RESPONSE_PREPARE_IDLE;
            break;
        
        case M590_RESPONSE_PREPARE_IDLE:
            resetAsyncVariables();
            _responseState = M590_RESPONSE_IDLE;
            _gsmState = M590_MODEM_IDLE;
            break;

        case M590_RESPONSE_IDLE:
            /* Do nothing */
            break;

        default:
            //you should never reach here
            printDebug(M590_ERROR_INVALID_STATE);
            break;
    }
}

ResponseStateType M590::readForResponseAsync()
{
    if (millis() > _asyncStartTime + _asyncTimeout)
    {/* Action for timeout */
        return M590_RESPONSE_TIMEOUT;
    }

    while (_gsmSerial->available())
    {
        /* Get response from Serial port*/
        char c = (char)_gsmSerial->read();
        MONITOR(c);

        /* Process the result if applicable */
        processResult(c);

        /* Check for the requested response String */
        if (c == pgm_read_byte_near(_asyncProgmemResponseString + _asyncBytesMatched))
        {
            _asyncBytesMatched++;

            if (_asyncBytesMatched == _asyncResponseLength)
            {
                /* Action for success */
                return M590_RESPONSE_SUCCESS;
            }
        }
        else
        {
            _asyncBytesMatched = 0;
        }
        
        /* Check for ERROR response*/
        if (c == pgm_read_byte_near(M590_RESPONSE_ERROR + _asyncFailMatched))
        {
            _asyncFailMatched++;

            if (_asyncFailMatched == strlen(M590_RESPONSE_ERROR))
            {
                return M590_RESPONSE_FAILURE;
            }
        }
        else
        {
            _asyncFailMatched = 0;
        }

    }

    return M590_RESPONSE_RUNNING;
}

void M590::processResult(char c)
{
    switch (_asyncResultType)
    {
        case M590_RES_CUSD:
            processCUSD(c);
        break;

        case M590_RES_CMGR:
            processCMGR(c);
        break;

        case M590_RES_CMGL:
            processCMGL(c);
        break;

        case M590_RES_CMGS:
            processCMGS(c);
        break;

        case M590_RES_CSQ:
            processCSQ(c);
        break;

        case M590_RES_XIIC:
            processXIIC(c);
        break;

        case M590_RES_DNS:
            processDNS(c);
        break;

        case M590_RES_TCPSETUP0:
        case M590_RES_TCPSETUP1:
            /* Handling done in responseHandler()*/
        break;

        case M590_RES_TCPSEND:
            processTCPSend(c);
        break;
        case M590_RES_NULL:
            /* Nothing to process */
            break;

        default:
            /* Invalid Result Type */
            printDebug(M590_ERROR_INVALID_STATE);
            break;
    }
}

void M590::processCMGS(char c)
{
    switch (_asyncProcessState)
    {
        case 0: /* Wait for '>' */
            if (c == '>')
            {/* Text input character match */

                /* Send the message */
                _gsmSerial->print((const char *)(_asyncResultptr));
                
                /* write terminating character "Ctrl+Z" (0x1A) to finalize the message */
                _gsmSerial->write(0x1A);

                /* Finish the process*/
                _asyncProcessState++;
            }
            break;

        case 1: /* Process completed */
            /* Do nothing */
            break;

        default:
            printDebug(M590_ERROR_INVALID_STATE);
            break;
    }
}

void M590::processCMGL(char c)
{
    switch (_asyncProcessState)
    {
    case 0: /* Wait for separator */
        if (c == ':')
        {/* separator and new sms found increase smsCount and go to next phase*/
            ((M590_SMSList_ResultType *)_asyncResultptr)->smsCount++;
            _asyncProcessState++;
        }
        break;

    case 1: /* Read until '"'  */
        if (c == '"')
        {/* Found, go to next phase */
            _asyncProcessState++;
        }
        break;

    case 2: /* Read status until '"' */
        if (c == '"')
        {/* process and go to next phase*/
            _asyncProcessState++;
        }
        else
        {/* Add the character to the temp string */
            ((M590_SMSList_ResultType *)_asyncResultptr)->smsArray[((M590_SMSList_ResultType *)_asyncResultptr)->smsCount - 1].status += c;
        }
        break;

    case 3: /* Read until '"'  */
        if (c == '"')
        {/* Found, go to next phase */
            _asyncProcessState++;
        }
        break;

    case 4: /* Read sender until '"' */
        if (c == '"')
        {/* process and go to next phase*/
            _asyncProcessState++;
        }
        else
        {/* Add the character to the temp string */
            ((M590_SMSList_ResultType *)_asyncResultptr)->smsArray[((M590_SMSList_ResultType *)_asyncResultptr)->smsCount - 1].sender += c;
        }
        break;

    case 5: /* Read until '"'  */
        if (c == '"')
        {/* Found, go to next phase */
            _asyncProcessState++;
        }
        break;

    case 6: /* Read Empty field until '"'  */
        if (c == '"')
        {/* Found, go to next phase*/
            _asyncProcessState++;
        }
        break;

    case 7: /* Read until '"'  */
        if (c == '"')
        {/* Found, go to next phase */
            _asyncProcessState++;
        }
        break;

    case 8: /* Read Date until '"' */
        if (c == '"')
        {/* process and go to next phase*/
            _asyncProcessState++;
        }
        else
        {/* Add the character to the temp string */
            ((M590_SMSList_ResultType *)_asyncResultptr)->smsArray[((M590_SMSList_ResultType *)_asyncResultptr)->smsCount - 1].date += c;
        }
        break;

    case 9: /* Read until '\n'  */
        if (c == '\n')
        {/* Found, go to next phase */
            _asyncProcessState++;
        }
        break;

    case 10: /* Read Date until '\r' */ //@Todo: Better limitation needed
        if (c == '\r')
        {/* process and wait for next SMS by jumping back to beginning */
            _asyncProcessState = 0;
        }
        else
        {/* Add the character to the temp string */
            ((M590_SMSList_ResultType *)_asyncResultptr)->smsArray[((M590_SMSList_ResultType *)_asyncResultptr)->smsCount - 1].text += c;
        }
        break;

    case 11: /* Process completed */
        /* Do nothing */
        break;

    default:
        printDebug(M590_ERROR_INVALID_STATE);
        break;
    }
}

void M590::processCMGR(char c)
{
    switch (_asyncProcessState)
    {
    case 0: /* Wait for separator */
        if (c == ':')
        {/* separator found go to next phase*/

            _asyncProcessState++;
        }
        break;

    case 1: /* Read until '"'  */
        if (c == '"')
        {/* Found, go to next phase */
            _asyncProcessState++;
        }
        break;

    case 2: /* Read status until '"' */
        if (c == '"')
        {/* process and go to next phase*/
            _asyncProcessState++;
        }
        else
        {/* Add the character to the temp string */
            ((M590_SMS_ResultType *)_asyncResultptr)->status += c;
        }
        break;

    case 3: /* Read until '"'  */
        if (c == '"')
        {/* Found, go to next phase */
            _asyncProcessState++;
        }
        break;

    case 4: /* Read sender until '"' */
        if (c == '"')
        {/* process and go to next phase*/
            _asyncProcessState++;
        }
        else
        {/* Add the character to the temp string */
            ((M590_SMS_ResultType *)_asyncResultptr)->sender += c;
        }
        break;

    case 5: /* Read until '"'  */
        if (c == '"')
        {/* Found, go to next phase */
            _asyncProcessState++;
        }
        break;

    case 6: /* Read Empty field until '"'  */
        if (c == '"')
        {/* Found, go to next phase*/
            _asyncProcessState++;
        }
        break;

    case 7: /* Read until '"'  */
        if (c == '"')
        {/* Found, go to next phase */
            _asyncProcessState++;
        }
        break;

    case 8: /* Read Date until '"' */
        if (c == '"')
        {/* process and go to next phase*/
            _asyncProcessState++;
        }
        else
        {/* Add the character to the temp string */
            ((M590_SMS_ResultType *)_asyncResultptr)->date += c;
        }
        break;

    case 9: /* Read until '\n'  */
        if (c == '\n')
        {/* Found, go to next phase */
            _asyncProcessState++;
        }
        break;

    case 10: /* Read Date until '\r' */ //@Todo: Better limitation needed
        if (c == '\r')
        {/* process and go to next phase*/
            _asyncProcessState++;
        }
        else
        {/* Add the character to the temp string */
            ((M590_SMS_ResultType *)_asyncResultptr)->text += c;
        }
        break;

    case 11: /* Process completed */
        /* Do nothing */
        break;

    default:
        printDebug(M590_ERROR_INVALID_STATE);
        break;
    }
}

void M590::processCUSD(char c)
{
    switch (_asyncProcessState)
    {
        case 0: /* Wait for separator */
            if (c == ':')
            {/* separator found go to next phase*/

                _asyncProcessState = 1;
            }
        break;

        case 1: /* Wait until '"'  */
            if (c == '"')
            {/* separator found go to next phase*/
                _asyncProcessState = 2;
            }
        break;

        case 2: /* Read until '"' */
            if (c == '"')
            {/* process and go to next phase*/
                _asyncProcessState = 3;
            }
            else
            {/* Add the character to the temp string */
                *((M590_USSDResponse_ResultType *)_asyncResultptr) += c;
            }
        break;

        case 3: /* Process completed */
            /* Do nothing */
            break;

        default:
            printDebug(M590_ERROR_INVALID_STATE);
        break;
    }
}

void M590::processCSQ(char c)
{
    switch (_asyncProcessState)
    {
        case 0: /* Wait for separator */
            if (c == ':')
            {/* separator found go to next phase*/

                _asyncProcessState = 1;
            }
        break;

        case 1: /* Read until ',' */
            if (c == ',')
            {/* process and go to next phase*/

                ((M590_SignalQuality_ResultType *)_asyncResultptr)->rssi = _asyncTempString.toInt();
                _asyncTempString = "";

                _asyncProcessState = 2;
            }
            else
            {/* Add the character to the temp string */
                _asyncTempString += c;
            }
        break;

        case 2: /* Read until '\r' */
            if (c == '\r')
            {/* process and go to next phase*/

                ((M590_SignalQuality_ResultType *)_asyncResultptr)->ber = _asyncTempString.toInt();;
                _asyncTempString = "";

                _asyncProcessState = 3;
            }
            else
            {/* Add the character to the temp string */
                _asyncTempString += c;
            }
        break;


        case 3: /* Process completed */
            /* Do nothing */
        break;

        default:
            printDebug(M590_ERROR_INVALID_STATE);
        break;
    }
}

void M590::processXIIC(char c)
{
    switch (_asyncProcessState)
    {
    case 0: /* Wait for separator */
        if (c == ',')
        {/* separator found go to next phase*/

            _asyncProcessState = 1;
        }
        break;

    case 1: /* Read until '\r' */
        if (c == '\r')
        {/* process and go to next phase*/

            *((M590_IP_ResultType *)_asyncResultptr) = _asyncTempString;
            _asyncTempString = "";

            _asyncProcessState = 2;
        }
        else
        {/* Add the character to the temp string */
            _asyncTempString += c;
        }

    case 2: /* Process completed */
        /* Do nothing */
        break;

    default:
        printDebug(M590_ERROR_INVALID_STATE);
        break;
    }
}

void M590::processDNS(char c)
{
    switch (_asyncProcessState)
    {
        case 0: /* Wait for separator */
            if (c == ':')
            {/* separator found go to next phase*/
                _asyncProcessState = 1;
            }
            break;

        case 1: /* Read until '\r' */
            if (c == '\r')
            {/* process and go to next phase*/
                _asyncProcessState = 2;
            }
            else
            {/* Add the character to the temp string */
                *((String *)_asyncResultptr) += c;
            }
            break;

        case 2: /* Process completed */
            /* Do nothing */
            break;

        default:
            printDebug(M590_ERROR_INVALID_STATE);
            break;
    }
}

void M590::processTCPSetup(char c){

    switch (_asyncProcessState)
    {
    case 0: /* Wait for separator */
        if (c == ':')
        {/* separator found go to next phase*/
            _asyncProcessState = 1;
            _internalString = "";
        }
        break;

    case 1: /* Read until ',' */
        if (c == ',')
        {/* process and go to next phase*/
            _asyncProcessState = 2;

            if (_internalString.toInt() == 0)
            {
                _socket[M590_GPRS_LINK_0].connected = true;
            }
            else if (_internalString.toInt() == 1)
            {
                _socket[M590_GPRS_LINK_1].connected = true;
            }
            else
            {
                printDebug(M590_ERROR_GPRS_INVALID_LINK);
            }
        }
        else
        {/* Add the character to the temp string */
            _internalString += c;
        }
        break;

    case 2: /* Process completed */
        /* Do nothing */
        break;

    default:
        printDebug(M590_ERROR_INVALID_STATE);
        break;
    }

}

void M590::processTCPSend(char c)
{
    switch (_asyncProcessState)
    {
    case 0: /* Wait for '>' */
        if (c == '>')
        {/* Text input character match */

            /* Send the request */
            _gsmSerial->print((const char *)(_asyncResultptr));
            MONITOR((const char *)(_asyncResultptr));

            /* write terminating character (0x0D) to finalize the message */
            _gsmSerial->write(0x0D);

            /* Finish the process*/
            _asyncProcessState++;
        }
        break;

    case 1: /* Process completed */
        /* Do nothing */
        break;

    default:
        printDebug(M590_ERROR_INVALID_STATE);
        break;
    }
}