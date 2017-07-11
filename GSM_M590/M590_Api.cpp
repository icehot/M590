#include "M590.h"


/* M590 class public API functions */

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
            /* Assign the resultType and result pointer */
            _asyncResultType = c.resultType;
            _asyncResultptr = c.resultptr;
            /* Assign the internal Cbk handler */
            _asyncInternalCbk = c.internalCbk;

            /** Start the response reading by setting the M590_RESPONSE_RUNNING state for responshandler() **/
            _responseState = M590_RESPONSE_RUNNING;

            MONITOR("<< ");
        }
        else
        {/* There is no job in the fifo, idle task can run */
            idleHandler();
        }
    }
    else
    {/* Process ongoing job*/
        responseHandler();
    }
}

bool M590::checkAlive(void(*commandCbk)(ResponseStateType response))
{/* Check if the modem is alive */

    CommandType c;
    c.command = M590_COMMAND_AT;
    c.parameter = "";
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
    c.response = M590_RESPONSE_OK;
    c.commandCbk = commandCbk;

    if (!_commandFifo.add(c))
        return false;

    /** Step 2: Set modem character set to ‘GSM’ with AT+CSCS="GSM" expect OK **/

    c.command = M590_COMMAND_SET_MODEM_GSM_CHARSET;
    c.parameter = "";
    c.response = M590_RESPONSE_OK;
    c.commandCbk = NULL;

    if (!_commandFifo.add(c))
        return false;

    return true;
}

bool M590::sendUSSD(char * ussdString, M590_USSDResponse_ResultType* resultptr, void(*commandCbk)(ResponseStateType response))
{
    /* Check the connected state */
    if (_networkState == M590_NET_REGISTERED)
    {
        CommandType c;
        c.command = M590_COMMAND_USSD_START;
        c.parameter += ussdString;
        c.parameter += F("\",15"); //M590_COMMAND_USSD_END
        c.response = M590_RESPONSE_OK;
        c.commandCbk = commandCbk;
        c.timeout = 10 * ASYNC_TIMEOUT;

        c.resultType = M590_RES_CUSD;
        if (resultptr != NULL)
            c.resultptr = (void*)resultptr;
        else
            return false;

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

bool M590::sendSMS(const char *phoneNumber, const char* smsText, void(*commandCbk)(ResponseStateType response) = NULL)
{
    /* Check the connected state */
    if (_networkState == M590_NET_REGISTERED)
    {

        CommandType c;
        c.command = M590_COMMAND_SMS_START;
        c.parameter += phoneNumber;
        c.parameter += F("\""); //M590_COMMAND_SMS_PHONE_NR_END
        c.response = M590_RESPONSE_OK; /* First stage of SMS sending, wait for ">" character */
        c.commandCbk = commandCbk; /* Handler for sms text sending at the second stage */
        c.timeout = 10 * ASYNC_TIMEOUT;

        c.resultType = M590_RES_CMGS;

        c.resultptr = (void*)smsText; /* Reuse of result pointer */

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

    c.response = M590_RESPONSE_OK;
    c.commandCbk = commandCbk;

    if (_commandFifo.add(c))
        return true;
    else
        return false;
}

bool M590::readSMS(byte index, M590_SMS_ResultType* resultptr, void(*commandCbk)(ResponseStateType response) = NULL)
{
    CommandType c;

    c.command = M590_COMMAND_SMS_READ;
    c.parameter = "";
    c.parameter += index;
    c.response = M590_RESPONSE_OK;
    c.commandCbk = commandCbk;
    c.resultType = M590_RES_CMGR;
    if (resultptr != NULL)
        c.resultptr = (void*)resultptr;
    else
        return false;

    if (_commandFifo.add(c))
        return true;
    else
        return false;
}

bool M590::readSMSList(M590_SMSReadFlagType readFlag, M590_SMSList_ResultType* resultptr, void(*commandCbk)(ResponseStateType response) = NULL)
{
    CommandType c;

    c.command = M590_COMMAND_SMS_LIST_READ;
    c.parameter = "";
    c.parameter += readFlag;
    c.response = M590_RESPONSE_OK;
    c.commandCbk = commandCbk;
    c.resultType = M590_RES_CMGL;
    if (resultptr != NULL)
    {
        resultptr->smsCount = 0;
        c.resultptr = (void*)resultptr;
    }
    else
        return false;

    if (_commandFifo.add(c))
        return true;
    else
        return false;
}

bool M590::getSignalStrength(M590_SignalQuality_ResultType* resultptr, void(*commandCbk)(ResponseStateType response))
{
    CommandType c;

    c.command = M590_COMMAND_GET_SIGNAL_STRENGTH;
    c.parameter = "";
    c.response = M590_RESPONSE_OK;
    c.commandCbk = commandCbk;
    c.resultType = M590_RES_CSQ;
    if (resultptr != NULL)
        c.resultptr = (void*)resultptr;
    else
        return false;

    if (_commandFifo.add(c))
        return true;
    else
        return false;
}

bool M590::enableNewSMSNotification(void(*newSMSnotificationCbk)(byte index), void(*commandCbk)(ResponseStateType response))
{
    CommandType c;

    c.command = M590_COMMAND_SMS_NOTIFICATION_ENABLE;
    c.parameter = "";
    c.response = M590_RESPONSE_OK;
    c.commandCbk = commandCbk;
    c.resultType = M590_RES_NULL;
    
    if (newSMSnotificationCbk != NULL)
        _newSMSnotificationCbk = newSMSnotificationCbk;
    else
        return false;

    if (_commandFifo.add(c))
    {
        _smsNotificationEnabled = true;
        return true;
    }
    else 
    { 
        return false;
    }
}

bool M590::disableNewSMSNotification(void(*commandCbk)(ResponseStateType response))
{
    CommandType c;

    c.command = M590_COMMAND_SMS_NOTIFICATION_DISABLE;
    c.parameter = "";
    c.response = M590_RESPONSE_OK;
    c.commandCbk = commandCbk;
    c.resultType = M590_RES_NULL;
    _newSMSnotificationCbk = NULL;

    if (_commandFifo.add(c))
    {
        _smsNotificationEnabled = false;
        return true;
    }
    else
        return false;
}

bool M590::connect(M590_GPRSLinkType link, const char *host, unsigned int port, void(*connectCbk)(ResponseStateType response) = NULL, void(*disconnectCbk)(ResponseStateType response) = NULL)
{/* At the first stage get the Ip address of the host*/
    CommandType c;

    c.command = M590_COMMAND_GPRS_DNS;
    c.parameter = "";
    c.parameter += host;
    c.parameter += "\"";
    c.response = M590_RESPONSE_DNS_OK;
    c.commandCbk = NULL;
    c.resultType = M590_RES_DNS;
    _socket[link].ip = "";
    c.resultptr = (void*)&_socket[link].ip;
    c.timeout = DNS_TIMEOUT;

    /* Save the port and commandCbk for phase2 */
    _socket[link].port = port;
    _socket[link]._connectCbk = connectCbk;
    _socket[link]._disconnectCbk = disconnectCbk;

    /* Assign the next phase handler*/
    if (link == M590_GPRS_LINK_0)
        c.internalCbk = &tcpSetup0;
    else if (link == M590_GPRS_LINK_1)
        c.internalCbk = &tcpSetup1;
    else
        printDebug(M590_ERROR_GPRS_INVALID_LINK);

    if (_commandFifo.add(c))
        return true;
    else
        return false;
}

void M590::tcpSetup0(ResponseStateType response)
{/* DNS check completed */
    CommandType c;

    if (response == M590_RESPONSE_SUCCESS)
    {
        c.command = M590_COMMAND_GPRS_TCPSETUP;
        c.parameter = "";
        c.parameter += M590_GPRS_LINK_0;
        c.parameter += ",";
        c.parameter += _socket[M590_GPRS_LINK_0].ip;
        c.parameter += ",";
        c.parameter += _socket[M590_GPRS_LINK_0].port;
        c.response = M590_RESPONSE_TCP0_OK;
        c.resultType = M590_RES_TCPSETUP0;
        c.commandCbk = _socket[M590_GPRS_LINK_0]._connectCbk;
        c.timeout = TCP_TIMEOUT;

        if (_commandFifo.add(c) == false)
            printDebug(M590_ERROR_FIFO_FULL);
    }
    else
    {
        printDebug(M590_ERROR_GPRS_DNS_ERROR);
    }
}

void M590::tcpSetup1(ResponseStateType response)
{/* DNS check completed */
    CommandType c;

    if (response == M590_RESPONSE_SUCCESS)
    {
        c.command = M590_COMMAND_GPRS_TCPSETUP;
        c.parameter = "";
        c.parameter += M590_GPRS_LINK_1;
        c.parameter += ",";
        c.parameter += _socket[M590_GPRS_LINK_1].ip;
        c.parameter += ",";
        c.parameter += _socket[M590_GPRS_LINK_1].port;
        c.response = M590_RESPONSE_TCP1_OK;
        c.resultType = M590_RES_TCPSETUP1;
        c.commandCbk = _socket[M590_GPRS_LINK_1]._connectCbk;
        c.timeout = TCP_TIMEOUT;

        if (_commandFifo.add(c) == false)
            printDebug(M590_ERROR_FIFO_FULL);
    }
    else
    {
        printDebug(M590_ERROR_GPRS_DNS_ERROR);
    }
}

bool M590::disconnect(M590_GPRSLinkType link, void(*commandCbk)(ResponseStateType response) = NULL)
{
    CommandType c;

    c.command = M590_COMMAND_GPRS_TCPCLOSE;
    c.parameter = "";
    c.parameter += link;
    c.response = M590_RESPONSE_OK;
    c.commandCbk = commandCbk;

    if (_commandFifo.add(c))
        return true;
    else
        return false;
}

bool M590::send(M590_GPRSLinkType link, const char * buffer, void(*commandCbk)(ResponseStateType response))
{
    CommandType c;

    if (_socket[link].connected == true)
    {
        c.command = M590_COMMAND_GPRS_TCPSEND;
        c.parameter = "";
        c.parameter += link;
        c.parameter += ",";
        c.parameter += strlen(buffer);
        c.resultptr = (void*)buffer;
        c.resultType = M590_RES_TCPSEND;

        if (link == M590_GPRS_LINK_0)
        {
            c.response = M590_RESPONSE_TCP0_SEND;
        }
        else
        {
            c.response = M590_RESPONSE_TCP1_SEND;
        }

        if (_commandFifo.add(c))
            return true;
        else
            return false;
    }
    else
    {
        if (link == 0)
            printDebug(M590_ERROR_GPRS_LINK0_CLOSED);
        else /* (link == 1) */
            printDebug(M590_ERROR_GPRS_LINK1_CLOSED);
        return false;
    }
}

bool M590::available(M590_GPRSLinkType link)
{
    return !(_socket[link].receiveFifo.isEmpty());
}

char M590::read(M590_GPRSLinkType link)
{
    char c = 0;

    if (!_socket[link].receiveFifo.isEmpty())
        _socket[link].receiveFifo.remove(&c);

    return c;
}

GprsStateType M590::readyGPRS()
{
    return _gprsState;
}