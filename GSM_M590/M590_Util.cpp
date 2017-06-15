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