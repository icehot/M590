#include "M590.h"

/* Global variables*/
M590 gsm;
M590_CSQResultType csq;

void commandCbk(ResponseStateType response)
{
    Serial.print(F("#App: Response: ")); Serial.println(response);
}

void csqCbk(ResponseStateType response)
{
    if (response == M590_RESPONSE_SUCCESS)
        Serial.print(F("#App: RSSI: ")); Serial.print(csq.rssi); Serial.print(F(" BER: ")); Serial.println(csq.ber);
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

    if (gsm.init(115200, &Serial3, "0000"))//connect to M590 with 115200 baud, Serial3 uart port
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
    
    Serial.println("");

    gsm.checkAlive(&commandCbk);
    gsm.setSMSTextModeCharSetGSM(&commandCbk);
    gsm.deleteSMS(0, M590_SMS_DEL_ALL, &commandCbk);
    //gsm.sendUSSD("*133#", &commandCbk);
    gsm.getSignalStrength(&csq, &csqCbk);
    gsm.checkAlive(&commandCbk);
    
}

void loop()
{
  gsm.process();
}
