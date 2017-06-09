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