#include "M590.h"

/* M590 class core functionality for asynchronous modem access */

M590::M590()
{
    _gsmSerial = NULL;
    _debugSerial = NULL;
}

void M590::process()
{
    if (_gsmState == M590_MODEM_IDLE)
    {/* Check for new job */

        if (_commandFifo.getItemCount())
        {/* There is job in the fifo, process it */

            CommandType c;

            /** Set the gsm state to busy **/
            _gsmState = M590_MODEM_BUSY;

            /** Get the next element from FIFO **/
            _commandFifo.remove(&c);

            /** Send out the command **/
            sendCommand(c.command, c.parameter);

            /** Initialize the asynchronous response reading **/

                /* Start the timeout timer for async reading */
                _asyncStartTime = millis();
                /* save responseString pointer to look for in private variable (for responsehandler) */
                _asyncProgmemResponseString = c.response;
                _asyncResponseLength = strlen_P(c.response);
                /* Assign the timeout value */
                _asyncTimeout = c.timeout;
                /* Assign the command callback */
                _asyncCommandCbk = c.commandCbk;
                /* Assign the responsebuffer and size */
                _asyncResponseBuffer = c.responebuffer;
                _asyncResponseBufferSize = c.responsebufferSize;
                /* Assign the resultType and result pointer */
                _asyncResultType = c.resultType;
                _asyncResultptr = c.resultptr;

            /** Start the response reading by setting the M590_RESPONSE_RUNNING state for responshandler() **/
            _responseState = M590_RESPONSE_RUNNING;

            MONITOR("<< ");
        }
        else
        {/* There is no job in the fifo, idle task can run */

        }
    }
    else
    {/* Process ongoing job*/
        responseHandler();
    }
}

void M590::resetAsyncVariables() {
    _asyncStartTime = 0;
    _asyncTimeout = 0;
    _asyncProgmemResponseString = NULL;
    _asyncResponseLength = 0;
    _asyncBytesMatched = 0;
    _asyncSeparator = ':'; //@Todo: assign it dinamically
    _asyncSeparatorFound = false;
    _asyncResponseIndex = 0;
    _asyncResponseBuffer = NULL;
    _asyncResponseBufferSize = 0;
    _asyncResultType = M590_RES_NULL;
    _asyncResultptr = NULL;
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
            if (millis() > _asyncStartTime + _asyncTimeout)
            {/* Action for timeout */
                _responseState = M590_RESPONSE_TIMEOUT;
                break;
            }

            while (_gsmSerial->available())
            {
                /* Get response from Serial port*/
                char c = (char)_gsmSerial->read();
                MONITOR(c);

                /* Save the Response if it is requested*/
                if (_asyncResponseBuffer != NULL)
                {
                    if (c == _asyncSeparator)
                        _asyncSeparatorFound = true;
                    else
                    {
                        if ((_asyncResponseBufferSize - 1) <_asyncResponseIndex)
                        {
                            _responseState = M590_RESPONSE_LENGTH_EXCEEDED;
                        }

                        if (_asyncSeparatorFound)
                            _asyncResponseBuffer[_asyncResponseIndex++] = c;
                    }
                }

                /* Check for the requested response String */
                if (c == pgm_read_byte_near(_asyncProgmemResponseString + _asyncBytesMatched))
                {
                    _asyncBytesMatched++;

                    if (_asyncBytesMatched == _asyncResponseLength)
                    {
                        /* Action for success */
                        _responseState = M590_RESPONSE_SUCCESS;
                    }
                }
                else
                {
                    _asyncBytesMatched = 0;
                }
            }

            break;

        case M590_RESPONSE_TIMEOUT:
            printDebug(M590_ERROR_RESPONSE_TIMEOUT);
            if (_asyncCommandCbk != NULL)
            {
                _asyncCommandCbk(M590_RESPONSE_TIMEOUT);
            }
            _responseState = M590_RESPONSE_PREPARE_IDLE;
            break;

        case M590_RESPONSE_LENGTH_EXCEEDED:
            printDebug(M590_ERROR_RESPONSE_LENGTH_EXCEEDED);
            if (_asyncCommandCbk != NULL)
            {
                _asyncCommandCbk(M590_RESPONSE_LENGTH_EXCEEDED);
            }
            _responseState = M590_RESPONSE_PREPARE_IDLE;
            break;

        case M590_RESPONSE_SUCCESS:
            MONITOR("<< END");
            MONITOR_NL();

            /* Process the result if applicable */
            if (_asyncResultType != M590_RES_NULL)
            {
                processResult();
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

void M590::processResult()
{
    switch (_asyncResultType)
    {
        case M590_RES_CSQ:
            processCSQ();
        break;

        default:
            /* Invalid Result Type */
            printDebug(M590_ERROR_INVALID_STATE);
            break;
    }
}

void M590::processCSQ()
{
    int i;
    String tempString = "";

    //@Todo: waits for generalization
    for (i = 0; (i < _asyncResponseBufferSize) && (_asyncResponseBuffer[i] != ','); i++)
    {
        tempString += _asyncResponseBuffer[i];
    }

    ((M590_CSQResultType *)_asyncResultptr)->rssi = tempString.toInt();
    tempString = "";
    
    if (i < _asyncResponseBufferSize)
    {
        for (++i/*continue after the separator*/; (i < sizeof(_asyncResponseBuffer)) && (_asyncResponseBuffer[i] != '\r'); i++)
        {
            tempString += _asyncResponseBuffer[i];
        }

        ((M590_CSQResultType *)_asyncResultptr)->ber = tempString.toInt();
    }
    else
    {
        printDebug(M590_ERROR_RESPONSE_INCOMPLETE);
        ((M590_CSQResultType *)_asyncResultptr)->rssi = 0;
        ((M590_CSQResultType *)_asyncResultptr)->ber = 0;
    }
}