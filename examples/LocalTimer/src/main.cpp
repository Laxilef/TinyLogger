#include <Arduino.h>
#include <TinyLogger.h>

void setup() {
  Serial.begin(115200);
  Log.addStream(&Serial);
  
  Log.setLevel(TinyLogger::Level::VERBOSE);
  Log.setDateTemplate("\033[1m[%H:%M:%S]\033[22m");
  Log.setDateCallback([] {
    unsigned int time = millis() / 1000;
    int sec = time % 60;
    int min = time % 3600 / 60;
    int hour = time / 3600;
    
    return tm{sec, min, hour};
  });
}

unsigned long prevReport = 0;
void loop() {
  if (millis() - prevReport > 1000) {
    Log.infoln("Loop");
    
    Log.sfatalln("MAINLOOP", "Test fatal msg");
    Log.serrorln("MAINLOOP", "Test error msg");
    Log.swarningln("MAINLOOP", "Test warning msg");
    Log.sinfoln("MAINLOOP", "Test info msg");
    Log.snoticeln("MAINLOOP", "Test notice msg");

#ifdef ESP32
    Log.straceln("MAINLOOP", "Esp32 free heap: %d", ESP.getFreeHeap());
#endif

#ifdef ESP8266
    Log.straceln("MAINLOOP", "Esp8266 free heap: %d", ESP.getFreeHeap());
#endif

    Log.verboseln("NESTED.SERVICE.NAME", "After start: %d ms.", millis());

    prevReport = millis();
  }
}