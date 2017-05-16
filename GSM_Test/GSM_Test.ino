void setup()
{
    /* Initialize Serial communication with PC*/
    Serial.begin(115200);
    Serial.flush();
    Serial.println("\Arduino is Up!");

    /* Initialize M590 GSM module*/
    Serial3.begin(115200);
    Serial3.flush();
    Serial3.print("AT\r");
}

void loop()
{
    while (Serial3.available() > 0) {
        Serial.write(Serial3.read());
    }
    while (Serial.available() > 0) {
        Serial3.write(Serial.read());
    }
}