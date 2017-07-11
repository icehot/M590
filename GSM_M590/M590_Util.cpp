#include "M590.h"

/* M590 class utility functions */

bool M590::bufferStartsWithProgmem(char *buffer, const char *progmemString) {
    bool matches = true;
    for (int i = 0; i < strlen_P(progmemString); i++) {
        matches = buffer[i] == pgm_read_byte_near(progmemString + i);
    }
    return matches;
}

bool M590::enableDebugSerial(HardwareSerial *debugSerial) {
    if (debugSerial)
    {
        _debugSerial = debugSerial;
        return true;
    }
    else
    {
        _debugSerial = NULL;
        return false;
    }
}

void M590::printDebug(const char *progmemString, bool withNewline) {
    if (_debugSerial) {
        _debugSerial->print((__FlashStringHelper *)progmemString);
        if (withNewline) _debugSerial->println();
    }
}

void M590::printDebug(const String s, bool withNewline) {
    if (_debugSerial) {
        _debugSerial->print(s);
        if (withNewline) _debugSerial->println();
    }
}

void M590::checkForNewSMS(char c)
{
    byte index;

    if (_newSMSArrived)
    {/*Get the index number */
        if (c == '\r')
        {
            index = (byte)_asyncTempString.toInt();

            /* Reset SMS notification related variables */
            _asyncSMSMatched = 0;
            _newSMSArrived = false;
            _asyncTempString = "";

            if (_newSMSnotificationCbk != NULL)
            {
                _newSMSnotificationCbk(index);
            }
        }
        else
        {
            _asyncTempString += c;
        }
    }
    else
    {/* Wait for new SMS  */
        if (c == pgm_read_byte_near(M590_RESPONSE_CMTI + _asyncSMSMatched))
        {
            _asyncSMSMatched++;

            if (_asyncSMSMatched == strlen(M590_RESPONSE_CMTI))
            {
                /* Set the new SMS Arrived flag */
                _newSMSArrived = true;
                /* Prepare the buffer for getting the index */
                _asyncTempString = "";
            }
        }
        else
        {
            _asyncSMSMatched = 0;
        }
    }
}

void M590::checkForTCPClose(char c)
{
    byte index;

    if (_tcpCloseDetected)
    {/*Get the index number */
        if (c == ',')
        {
            index = (byte)_asyncTempString.toInt();

            /* Reset SMS notification related variables */
            _tcpCloseDetected = false;
            _asyncTempString = "";

            /* TCP Close Actions*/
            if (index == 0)
            {
                _socket[M590_GPRS_LINK_0].connected = false;
                if (_socket[M590_GPRS_LINK_0]._disconnectCbk)
                {
                    _socket[M590_GPRS_LINK_0]._disconnectCbk(M590_RESPONSE_SUCCESS);
                }
            }
            else if (index == 1)
            {
                _socket[M590_GPRS_LINK_1].connected = false;
                if (_socket[M590_GPRS_LINK_1]._disconnectCbk)
                {
                    _socket[M590_GPRS_LINK_1]._disconnectCbk(M590_RESPONSE_SUCCESS);
                }
            }
            else
            {
                printDebug(M590_ERROR_GPRS_INVALID_LINK);
            }
        }
        else
        {
            _asyncTempString += c;
        }
    }
    else
    {/* Wait for TCP Close */
        if (c == pgm_read_byte_near(M590_RESPONSE_TCPCLOSE + _asyncTCPCloseMatched))
        {
            _asyncTCPCloseMatched++;

            if (_asyncTCPCloseMatched == strlen(M590_RESPONSE_TCPCLOSE))
            {
                /* Set the TCP Close flag */
                _tcpCloseDetected = true;
                /* Prepare the buffer for getting the index */
                _asyncTempString = "";
            }
        }
        else
        {
            _asyncTCPCloseMatched = 0;
        }
    }
}

void M590::checkForTCPReceive(char c)
{
    if (_tcpReceiveDetected)
    {/*Get the index number */

        switch (_asyncReceiveState)
        {
            case 0: /* Get link number 0/1 */
                if (c == ',')
                {
                    _asyncLink = (byte)_asyncTempString.toInt();

                    if (_asyncLink >= M590_GPRS_LINK_0 && _asyncLink <= M590_GPRS_LINK_1)
                    {
                        /* Go to next phase */
                        _asyncReceiveState++;
                        _asyncTempString = "";
                    }
                    else
                    {/* Invalid link */
                        _asyncReceiveState = 0;
                        _tcpReceiveDetected = false;
                        printDebug(M590_ERROR_GPRS_INVALID_LINK);
                    }
                }
                else
                {
                    _asyncTempString += c;
                }
            break;

            case 1: /* Get number of bytes */
                if (c == ',')
                {
                    _asyncNrOfRecvBytes = _asyncTempString.toInt();

                    /* Go to next phase */
                    _asyncReceiveState++;
                    _asyncTempString = "";
                }
                else
                {
                    _asyncTempString += c;
                }
            break;

            case 2: /* Wait for number of bytes to be received */
                if (_asyncNrOfRecvBytes > 0)
                {
                    _socket[_asyncLink].receiveFifo.add(c);
                    _asyncNrOfRecvBytes--;
                }
                else
                {
                    /* Go to next phase */
                    _asyncReceiveState++;

                }
                break;

            case 3: /* Process completed */
                _tcpReceiveDetected = false;
                /* Do nothing */
                break;

            default:
                printDebug(M590_ERROR_INVALID_STATE);
                break;
        }
    }
    else
    {/* Wait for TCP Receive */
        if (c == pgm_read_byte_near(M590_RESPONSE_TCPRECV + _asyncTCPReceiveMatched))
        {
            _asyncTCPReceiveMatched++;

            if (_asyncTCPReceiveMatched == strlen(M590_RESPONSE_TCPRECV))
            {
                /* Set the TCP Close flag */
                _tcpReceiveDetected = true;
                _asyncReceiveState = 0;
                /* Prepare the buffer for getting the index */
                _asyncTempString = "";
            }
        }
        else
        {
            _asyncTCPReceiveMatched = 0;
        }
    }
}

void M590::gatewayHandler()
{
    char c;

    if (_debugSerial && _gsmSerial)
    {
        /* GSM to DEBUG */
        while (_gsmSerial->available() > 0) 
        {
            c = _gsmSerial->read();
            _debugSerial->write(c);

            if (_smsNotificationEnabled)
            {
                checkForNewSMS(c);
            }
        }

        /* DEBUG to GSM */
        while (_debugSerial->available() > 0) {
            _gsmSerial->write(_debugSerial->read());
        }
    }
}

void M590::idleHandler()
{
    char c;

    if (_gsmSerial)
    {
        while (_gsmSerial->available() > 0)
        {
            c = _gsmSerial->read();

            /* GSM to DEBUG */
            //if (_debugSerial)
            //    _debugSerial->write(c);
            
            /* SMS notification check */
            if (_smsNotificationEnabled)
            {
                checkForNewSMS(c);
            }

            /* Check for incoming bytes */

            checkForTCPReceive(c);

            checkForTCPClose(c);
        }

        /* DEBUG to GSM */
        if (_debugSerial)
        {
            while (_debugSerial->available() > 0) 
            {
                _gsmSerial->write(_debugSerial->read());
            }
        }

    }
}