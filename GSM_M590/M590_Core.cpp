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

            _gsmState = M590_MODEM_BUSY;

            CommandType c;

            _commandFifo.remove(&c);

            sendCommand(c.command, c.parameter);
            readForResponse(c.response, c.timeout, c.commandCbk);

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
    _asyncBytesMatched = 0;
    _asyncResponseLength = 0;
    _asyncProgmemResponseString = NULL;
    //_asyncResponseIndex = 0;
    _responseState = M590_RESPONSE_RUNNING;
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


void M590::readForResponse(const char *progmemResponseString, const unsigned int timeout, void(*commandCbk)(ResponseStateType response))
{/* Initialize the asynchronous response reading */
    /* Start the timeout timer for async reading */
    _asyncStartTime = millis();
    /* save responseString pointer to look for in private variable (for responsehandler) */
    _asyncProgmemResponseString = progmemResponseString;
    _asyncResponseLength = strlen_P(progmemResponseString);

    _asyncTimeout = timeout;

    _commandCbk = commandCbk;
}


void M590::responseHandler()
{/* Cyclic function to read for responses asynchronously */
    switch (_responseState)
    {
    case M590_RESPONSE_RUNNING:
        if (millis() > _asyncStartTime + _asyncTimeout)
        {
            resetAsyncVariables();

            /* Action for timeout */
            _responseState = M590_RESPONSE_TIMEOUT;
            break;
        }

        while (_gsmSerial->available())
        {
            char c = (char)_gsmSerial->read();
            MONITOR(c);

            if (c == pgm_read_byte_near(_asyncProgmemResponseString + _asyncBytesMatched))
            {
                _asyncBytesMatched++;

                if (_asyncBytesMatched == _asyncResponseLength)
                {
                    resetAsyncVariables();
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
        if (_commandCbk != NULL)
        {
            _commandCbk(M590_RESPONSE_TIMEOUT);
        }
        _responseState = M590_RESPONSE_IDLE;
        _gsmState = M590_MODEM_IDLE;
        break;

    case M590_RESPONSE_SUCCESS:
        MONITOR("<< END");
        MONITOR_NL();
        if (_commandCbk != NULL)
        {
            _commandCbk(M590_RESPONSE_SUCCESS);
        }
        _responseState = M590_RESPONSE_IDLE;
        _gsmState = M590_MODEM_IDLE;
        break;

    case M590_RESPONSE_IDLE:
        break;
    default:
        //you should never reach here
        break;
    }
}