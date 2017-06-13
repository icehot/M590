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

bool M590::deleteSMS(byte index, DeleteFlagType deleteFlag, void(*commandCbk)(ResponseStateType response))
{
    CommandType c;
    char parameter[5];

    c.command = M590_COMMAND_SMS_DELETE;

    /* Build up the parameter list */
    sprintf(parameter, "%d", index);
    strcpy(parameter + strlen(parameter), ",");
    sprintf(parameter + strlen(parameter), "%d", deleteFlag);

    c.parameter = parameter;

    c.responebuffer = NULL;
    c.response = M590_RESPONSE_OK;
    c.commandCbk = commandCbk;

    if (_commandFifo.add(c))
        return true;
    else
        return false;
}

bool M590::getSignalStrength(M590_CSQResultType* csq, void(*commandCbk)(ResponseStateType response))
{
    CommandType c;

    c.command = M590_COMMAND_GET_SIGNAL_STRENGTH;
    c.parameter = "";
    c.responebuffer = _internalBuffer;
    c.responsebufferSize = sizeof(_internalBuffer);
    c.response = M590_RESPONSE_OK;
    c.commandCbk = commandCbk;
    c.resultType = M590_RES_CSQ;
    if (csq != NULL)
        c.resultptr = (void*)csq;
    else
        return false;

    if (_commandFifo.add(c))
        return true;
    else
        return false;
}
