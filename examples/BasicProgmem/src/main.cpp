#include <Arduino.h>
#include <TinyLogger.h>

#ifndef PROGMEM
  #define PROGMEM 
#endif

const char S_MAINLOOP[] PROGMEM = "MAINLOOP";

void setup() {
  Serial.begin(115200);
  Log.begin(&Serial, TinyLogger::Level::VERBOSE);
}

unsigned long prevReport = 0;
void loop() {
  if (millis() - prevReport > 1000) {
    Log.infoln(F("Loop"));
    
    Log.sfatalln(FPSTR(S_MAINLOOP), F("Test fatal msg"));
    Log.serrorln(FPSTR(S_MAINLOOP), F("Test error msg"));
    Log.swarningln(FPSTR(S_MAINLOOP), F("Test warning msg"));
    Log.sinfoln(FPSTR(S_MAINLOOP), F("Test info msg"));
    Log.snoticeln(FPSTR(S_MAINLOOP), F("Test notice msg"));

    Log.verboseln(F("NESTED.SERVICE.NAME"), F("After start: %d ms."), millis());

    prevReport = millis();
  }
}