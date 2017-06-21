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
            _internalCbk = c.internalCbk;

            /** Start the response reading by setting the M590_RESPONSE_RUNNING state for responshandler() **/
            _responseState = M590_RESPONSE_RUNNING;

            MONITOR("<< ");
        }
        else
        {/* There is no job in the fifo, idle task can run */
            gatewayHandler();
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

bool M590::connect(const char *host, unsigned int port, void(*commandCbk)(ResponseStateType response))
{/* At the first stage get the Ip address of the host*/
    CommandType c;

    c.command = M590_COMMAND_GPRS_DNS;
    c.parameter = "";
    c.parameter += host;
    c.parameter += "\"";
    c.response = M590_RESPONSE_DNS_OK;
    c.commandCbk = NULL;
    c.resultType = M590_RES_DNS;
    _internalString = "";
    c.resultptr = (void*)&_internalString;
    c.timeout = DNS_TIMEOUT;

    /* Assign the next phase handler*/
    c.internalCbk = &connectPhase2;

    /* Save the port */
    _asyncPort = port;

    if (_commandFifo.add(c))
        return true;
    else
        return false;
}

void M590::connectPhase2(ResponseStateType response)
{/* DNS check completed */
    CommandType c;

    Serial.print("DNS Response: "); Serial.println(response);
    Serial.print("IP: "); Serial.println(_internalString);

    if (response == M590_RESPONSE_SUCCESS)
    {
        c.command = M590_COMMAND_GPRS_TCPSETUP;
        c.parameter = "";
        c.parameter += 0; //@Todo: Link 0 or 1
        c.parameter += ",";
        c.parameter += _internalString;
        //c.parameter += "/json";
        c.parameter += ",";
        c.parameter += 80;// _asyncPort;
        c.response = M590_RESPONSE_TCP0_OK;
        c.commandCbk = NULL;
        c.resultType = M590_RES_NULL;
        c.timeout = TCP_TIMEOUT;

        /* Assign the next phase handler*/
        c.internalCbk = &connectPhase3;

        if (_commandFifo.add(c) == false )
            printDebug(M590_ERROR_FIFO_FULL);
    }
    else
    {
        printDebug(M590_ERROR_GPRS_DNS_ERROR);
    }
}

void M590::connectPhase3(ResponseStateType response)
{/* TCPSetup Result */
    Serial.print("TCP Setup: "); Serial.println(response);
}
