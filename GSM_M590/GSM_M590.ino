#include "M590.h"

/* Global variables*/
M590 gsm;
M590_SignalQuality_ResultType sq;
M590_USSDResponse_ResultType ussd;
M590_SMS_ResultType sms;
M590_SMSList_ResultType smsList;
M590_IP_ResultType ip;

void newSmsNotificationEnableCbk(ResponseStateType response)
{
    if (response == M590_RESPONSE_SUCCESS)
    {
        Serial.println(F("#App: Sms notification enabled"));
    }
    else
    {
        Serial.print(F("#App: CNMI error: ")); Serial.println(response);
    }
}

void newSmsNotificationCbk(byte index)
{
    Serial.println("\nNew SMS arrived!");
    /* Initialize sms buffer */

    sms.date = "";
    sms.sender = "";
    sms.status = "";
    sms.text = "";

    gsm.readSMS(index, &sms, &readSMSCbk);
}

void sendSmsCbk(ResponseStateType response)
{
    if (response == M590_RESPONSE_SUCCESS)
    {
        Serial.println(F("#App: Sms Sent"));
    }
    else
    {
        Serial.print(F("#App: CMGS error: ")); Serial.println(response);
    }
}

void readSMSListCbk(ResponseStateType response)
{
    if (response == M590_RESPONSE_SUCCESS)
    {
        Serial.println(F("#App: SMS List Read completed "));
        for (int i = 0; i < smsList.smsCount; i++)
        {
            Serial.print(" Record #"); Serial.println(i+1);
            Serial.print("  Status: "); Serial.println(smsList.smsArray[i].status);
            Serial.print("  Sender: "); Serial.println(smsList.smsArray[i].sender);
            Serial.print("  Date: ");   Serial.println(smsList.smsArray[i].date);
            Serial.print("  Text: ");   Serial.println(smsList.smsArray[i].text);
        }
    }
    else
    {
        Serial.print(F("#App: CMGL error: ")); Serial.println(response);
    }
}

void readSMSCbk(ResponseStateType response)
{
    if (response == M590_RESPONSE_SUCCESS)
    {
        Serial.println(F("#App: SMS Read completed "));
        Serial.print(" Status: "); Serial.println(sms.status);
        Serial.print(" Sender: "); Serial.println(sms.sender);
        Serial.print(" Date: ");   Serial.println(sms.date);
        Serial.print(" Text: ");   Serial.println(sms.text);
    }
    else
    {
        Serial.print(F("#App: CMGR error: ")); Serial.println(response);
    }
}

void deleteSMSCbk(ResponseStateType response)
{
    if (response == M590_RESPONSE_SUCCESS)
    {
        Serial.println(F("#App: SMS delete completed"));
    }
    else
    {
        Serial.print(F("#App: CMGD error: ")); Serial.println(response);
    }
}

void sqCbk(ResponseStateType response)
{
    if (response == M590_RESPONSE_SUCCESS)
    {
        Serial.print(F("#App: RSSI: ")); Serial.print(sq.rssi); Serial.print(F(" BER: ")); Serial.println(sq.ber);
    }
    else
    {
        Serial.print(F("#App: CSQ error: ")); Serial.println(response);
    }
}

void ussdCbk(ResponseStateType response)
{
    if (response == M590_RESPONSE_SUCCESS)
    {
        Serial.print(F("#App: USSD Response: ")); Serial.println(ussd);
    }
    else
    {
        Serial.print(F("#App: USSD error: ")); Serial.println(response);
    }
}

void aliveCbk(ResponseStateType response)
{
    if (response == M590_RESPONSE_SUCCESS)
    {
        Serial.println(F("#App: M590 is alive!"));
    }
    else
    {
        Serial.print(F("#App: M590 is not alive: ")); Serial.println(response);
    }
}

void settingsCbk(ResponseStateType response)
{
    if (response == M590_RESPONSE_SUCCESS)
    {
        Serial.println(F("#App: SMS Textmode and GSM Charset was set"));
    }
    else
    {
        Serial.print(F("#App: CMGF/CSCS error: ")); Serial.println(response);
    }
}

void connectCbk0(ResponseStateType response)
{/* TCPSetup Result */
    if (response == M590_RESPONSE_SUCCESS)
    {
        Serial.println(F("#App: TCP0 Setup done"));

        if (gsm.send(M590_GPRS_LINK_0, "GET /vshymanskyy/tinygsm/master/extras/logo.txt HTTP/1.0\r\nHost: cdn.rawgit.com\r\nConnection: close\r\n\r\n", &tcpSendCbk))
        //if (gsm.send(M590_GPRS_LINK_0, "GET /vshymanskyy/tinygsm/master/extras/logo.txt HTTP/1.0\r\n\r\n", &tcpSendCbk))
        {
            Serial.println(F("#App: TCP0 Send started"));
        }
        else
        {
            Serial.println(F("#App: TCP0 Send Error"));
        }
    }
    else
    {
        Serial.print(F("#App: TCPSETUP0 error: ")); Serial.println(response);
    }
}

void disconnectCbk0(ResponseStateType response)
{
    Serial.println(F("\n\n #App: Link 0 Closed \n\n"));
}

void connectCbk1(ResponseStateType response)
{/* TCPSetup Result */
    if (response == M590_RESPONSE_SUCCESS)
    {
        Serial.println(F("#App: TCP1 Setup done"));

        if (gsm.send(M590_GPRS_LINK_1, "GET /index.htm HTTP/1.0\r\n\r\n", &tcpSendCbk))
        {
            Serial.println(F("#App: TCP1 Send started"));
        }
        else
        {
            Serial.println(F("#App: TCP1 Send Error"));
        }
    }
    else
    {
        Serial.print(F("#App: TCPSETUP1 error: ")); Serial.println(response);
    }
}

void disconnectCbk1(ResponseStateType response)
{
    Serial.println(F("\n\n#App: Link 1 Closed\n\n"));
}



void tcpSendCbk(ResponseStateType response)
{/* TCPSetup Result */
    if (response == M590_RESPONSE_SUCCESS)
    {
        Serial.println(F("#App: TCP Sent"));
    }
    else
    {
        Serial.print(F("#App: TCPSend error: ")); Serial.println(response);
    }
}

void setup()
{
    Serial.begin(115200);

    Serial.println(F("\n>> M590 driver test <<\n"));

    if (gsm.enableDebugSerial(&Serial)) //optionally output debug information on Serial
    {
        Serial.println(F("#App: DebugSerial Enabled"));
    }
    else
    {
        Serial.println(F("#App: Serialdebug Failed"));
    }

    if (gsm.init(115200, &Serial3, "0000"))//connect to M590 with 115200 baud, Serial3 uart port, PIN 0000
    {
        Serial.println(F("#App: Init Done"));
    }
    else
    {
        Serial.println(F("#App: Init Failed"));
        while (1);
    }

    if (gsm.waitForNetWork())
    {
        Serial.println(F("#App: Network Registration Done"));
    }
    else
    {
        Serial.println(F("#App: Network Registration Failed"));
    }
    /*
    if (gsm.attachGPRS("net", "", "", &ip))
    {
        Serial.print(F("#App: GPRS attached, obtained IP: ")); Serial.println(ip);
    }
    else
    {
        Serial.println("App: GPRS attach failed");
        while (1);
    }
    */

  
    gsm.checkAlive(&aliveCbk);
    gsm.setSMSTextModeCharSetGSM(&settingsCbk);
    gsm.deleteSMS(0, M590_SMS_DEL_ALL, &deleteSMSCbk);
    gsm.sendUSSD("*133#", &ussd, &ussdCbk);
    gsm.getSignalStrength(&sq, &sqCbk);
    //gsm.readSMS(1, &sms, &readSMSCbk);
    gsm.readSMSList(M590_SMS_ALL, &smsList, &readSMSListCbk);
    //gsm.sendSMS("+40745662769", "Hello World", &sendSmsCbk);
    gsm.enableNewSMSNotification(&newSmsNotificationCbk, &newSmsNotificationEnableCbk);
    //gsm.connect(M590_GPRS_LINK_0, "cdn.rawgit.com", 80, &connectCbk0, &disconnectCbk0);
    //gsm.connect(M590_GPRS_LINK_1, "icehot.go.ro", 80, &connectCbk1, &disconnectCbk1);

    gsm.checkAlive(&aliveCbk);
    //gsm.disconnect(M590_GPRS_LINK_0, &disconnectCbk0);
}

void loop()
{
  gsm.process();

  if (gsm.available(M590_GPRS_LINK_0))
  {
      Serial.print(gsm.read(M590_GPRS_LINK_0));
  }

  if (gsm.available(M590_GPRS_LINK_1))
  {
      Serial.print(gsm.read(M590_GPRS_LINK_1));
  }
}
