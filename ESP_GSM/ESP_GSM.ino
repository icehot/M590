#include "M590.h"

M590 gsm;
bool entered = false;

byte rssi, ber;
char smsBuffer[255];//@Todo: SMS buffer size
char smsStatus[16];
char smsSender[16];
char smsDate[24];

unsigned long int start;

void notificationCMGR()
{
    Serial.println(F("CMGR notification!"));

    Serial.println(smsStatus);
    Serial.println(smsSender);
    Serial.println(smsDate);
    Serial.println(smsBuffer);

    if (gsm.deleteSMS(0, M590_SMS_DEL_ALL))
    {
        Serial.println(F("SMS Deleted Succesfully"));
    }
    else
    {
        Serial.println(F("SMS Delete Error"));
    }

    
    if (gsm.disableNewSMSNotification())
    {
        Serial.println(F("SMS Notification Disabled"));
    }
    else
    {
        Serial.println(F("SMS Notification Disable problem"));
    }
}

void notificationUSSD()
{
    Serial.println(F("CUSD notification!"));
}

void notificationCMGL()
{
    Serial.println(F("CMGL notification!"));
    Serial.println(smsBuffer);

    if (gsm.deleteSMS(0, M590_SMS_DEL_ALL))
    {
        Serial.println(F("SMS Deleted Succesfully"));
    }
    else
    {
        Serial.println(F("SMS Delete Error"));
    }

    if (gsm.disableNewSMSNotification())
    {
        Serial.println(F("SMS Notification Disabled"));
    }
    else
    {
        Serial.println(F("SMS Notification Disable problem"));
    }

}

void newSMSnotification( byte index)
{
    Serial.print(F("SMS Arrived to "));
    Serial.print(index); 
    Serial.println(F(" record"));

    if (gsm.readParsedSMS_Async(index, smsBuffer, sizeof(smsBuffer), smsStatus, sizeof(smsStatus), smsSender, sizeof(smsSender), smsDate, sizeof(smsDate), &notificationCMGR))
    {
        Serial.println(F("SMS Read  Started"));
    }
    else
    {
        Serial.println(F("SMS Read  Error"));
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println(F("\n\n>> M590 connectCellular example <<"));
    gsm.begin(115200, &Serial3); //connect to M590 with 115200 baud, Serial3 uart port
    gsm.enableDebugSerial(&Serial); //optionally output debug information on Serial
    gsm.initialize(""); //enter your PIN here, leave empty for no pin
}

void loop()
{
    gsm.loop(); //the loop method needs to be called often

    if ((millis() > 3000) && (entered == false))
    {
        /* Sync commands*/

        if(gsm.setSMSTextModeCharSetGSM())
        {
            Serial.println("Text Mode and GSM CharSet was set");
        }
        else
        {
            Serial.println(F("CMGF or CSCS Error"));
        }

        /* Deleta all sms from storage */

        if (gsm.deleteSMS(0, M590_SMS_DEL_ALL))
        {
            Serial.println(F("SMS Deleted Succesfully"));
        }
        else
        {
            Serial.println(F("SMS Delete Error"));
        }

        if (gsm.enableNewSMSNotification(&newSMSnotification))
        {
            Serial.println(F("SMS Notification enabled"));
        }
        else
        {
            Serial.println(F("SMS Notification problem "));
        }

        /* Start of asynchronous commands */
        start = millis();
        if (gsm.SendUSSD_Async("*133#", &notificationUSSD))
        {
            Serial.println(F("USSD Sent Succesfully"));
        }
        else
        {
            Serial.println(F("USSD Error"));
        }
        Serial.print(F("Runtime (ms): ")); Serial.println(millis() - start);

        entered = true;
    }
}

/* Tests for synchronous behaviour

if(gsm.setSMSTextModeCharSetGSM())
{
Serial.println("Text Mode and GSM CharSet was set");
}
else
{
Serial.println(F("CMGF or CSCS Error"));
}

if (gsm.getSignalStrength(&rssi, &ber))
{
Serial.print("RSSI: ");Serial.print(rssi);
Serial.print(" BER: ");Serial.println(ber);
}
else
{
Serial.println(F("CSQ Error"));
}

if (gsm.sendSMS("+40745662769", "Hello Bello"))
{
Serial.println(F("SMS Sent Succesfully"));
}
else
{
Serial.println(F("SMS Error"));
}

if (gsm.sendUSSD("*133#"))
{
Serial.println(F("USSD Sent Succesfully"));
}
else
{
Serial.println(F("USSD Error"));
}

if (gsm.readSMS(1, smsBuffer, sizeof(smsBuffer)))
{
Serial.println(smsBuffer);
}
else
{
Serial.println(F("SMS Read Error"));
}
*/



