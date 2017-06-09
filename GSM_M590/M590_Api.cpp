#include "M590.h"


/* M590 class public API functions */

bool M590::checkAlive(void(*commandCbk)(ResponseStateType response))
{/* Check if the modem is alive */

    CommandType c;
    c.command = M590_COMMAND_AT;
    c.parameter = "";
    c.responebuffer = NULL;
    c.response = M590_RESPONSE_OK;
    c.commandCbk = commandCbk;

    if (_commandFifo.add(c))
        return true;
    else
        return false;
}

bool M590::setSMSTextModeCharSetGSM(void(*commandCbk)(ResponseStateType response))
{
    CommandType c;

    /** Step 1: Set SMS mode to text mode with AT+CMGF=1 expect OK**/

    c.command = M590_COMMAND_SET_MODEM_TEXT_MODE;
    c.parameter = "";
    c.responebuffer = NULL;
    c.response = M590_RESPONSE_OK;
    c.commandCbk = commandCbk;

    if (!_commandFifo.add(c))
        return false;

    /** Step 2: Set modem character set to ‘GSM’ with AT+CSCS="GSM" expect OK **/

    c.command = M590_COMMAND_SET_MODEM_GSM_CHARSET;
    c.parameter = "";
    c.responebuffer = NULL;
    c.response = M590_RESPONSE_OK;
    c.commandCbk = NULL;

    if (!_commandFifo.add(c))
        return false;

    return true;
}

bool M590::sendUSSD(char * ussdString, void(*commandCbk)(ResponseStateType response))
{
    /* Check the connected state */
    if (_networkState == M590_NET_REGISTERED)
    {
        CommandType c;
        c.command = M590_COMMAND_USSD_START;
        c.parameter += ussdString;
        c.parameter += F("\",15"); //M590_COMMAND_USSD_END
        c.responebuffer = NULL;
        c.response = M590_RESPONSE_OK;
        c.commandCbk = commandCbk;
        c.timeout = 10 * ASYNC_TIMEOUT;

        if (_commandFifo.add(c))
            return true;
        else
            return false;
    }
    else
    {
        printDebug(M590_ERROR_NETWORK_NOT_REG);
        return false;
    }
}


//bool M590::getSignalStrength(void(*commandCbk)(ResponseStateType response, byte* rssi, byte* ber))
//{
//    CommandType c;
//    c.command = M590_COMMAND_GET_SIGNAL_STRENGTH;
//    c.parameter = "";
//    c.responebuffer = NULL;
//    c.response = M590_RESPONSE_OK;
//    c.commandCbk = commandCbk;
//
//    if (_commandFifo.add(c))
//        return true;
//    else
//        return false;
//
//    int i;
//    String tempString = "";
//
//    if (_currentState == M590_STATE_CELLULAR_CONNECTED)
//    {
//        sendCommand(M590_COMMAND_GET_SIGNAL_STRENGTH);
//        memset(_responseBuffer, 0, sizeof(_responseBuffer));
//        if (M590_SUCCESS == readForResponse(M590_RESPONSE_OK, _responseBuffer, sizeof(_responseBuffer)))
//        {
//            for (i = 0; (i < sizeof(_responseBuffer)) && (_responseBuffer[i] != ','); i++)
//            {
//                tempString += _responseBuffer[i];
//            }
//            *rssi = tempString.toInt();
//            tempString = "";
//
//            if (i < sizeof(_responseBuffer))
//            {
//                for (++i/*continue after the separator*/; (i < sizeof(_responseBuffer)) && (_responseBuffer[i] != '\0'); i++)
//                {
//                    tempString += _responseBuffer[i];
//                }
//            }
//            else
//            {
//                printDebug(F("Incomplete response"));
//                return false;
//            }
//
//            *ber = tempString.toInt();
//
//            return true;
//        }
//        else
//        {
//            printDebug(F("CSQ Error"));
//            return false;
//        }
//
//    }
//    else
//    {
//        printDebug(F("NOT connected"));
//        return false;
//    }
//}

