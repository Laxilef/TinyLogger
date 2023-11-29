#pragma once
#include <vector>
#include <Arduino.h>

#ifndef TINYLOGGER_NTPCLIENT
  #define TINYLOGGER_NTPCLIENT false
#endif

#if TINYLOGGER_NTPCLIENT
  #include <NTPClient.h>
#endif

class TinyLogger {
public:
  enum Level {
    SILENT,
    FATAL,
    ERROR,
    WARNING,
    INFO,
    NOTICE,
    TRACE,
    VERBOSE
  };

  void begin(Stream* stream, Level level = Level::ERROR) {
    this->streams.push_back(stream);
    this->level = level;
  }

  void lock() {
#if defined(ESP32)
    this->locked = true;
    this->lockedTime = millis();
#endif
  }

  void unlock() {
#if defined(ESP32)
    this->locked = false;
#endif
  }

  bool isLocked() {
#if defined(ESP32)
    if (this->locked) {
      if (millis() - this->lockedTime >= this->lockTimeout) {
        this->unlock();
      }
    }

    return this->locked;
#else
    return false;
#endif
  }

  void setLockTimeout(unsigned short val) {
#if defined(ESP32)
    this->lockTimeout = val;
#endif
  }

  void addStream(Stream* stream) {
    this->streams.push_back(stream);
  }

  void clearStreams() {
    this->streams.clear();
  }

  const std::vector<Stream*> getStreams() {
    return this->streams;
  }

  Level getLevel() {
    return this->level;
  }

  void setLevel(Level level) {
    this->level = level;
  }

  void setDate(struct tm* date) {
    this->date = date;
  }

  void setDate() {
    this->date = nullptr;
  }

  bool isDateSet() {
    return this->date != nullptr;
  }

#if TINYLOGGER_NTPCLIENT
  void setNtpClient(NTPClient* ntpClient) {
    this->ntpClient = ntpClient;
  }

  void setNtpClient() {
    this->ntpClient = nullptr;
  }

  bool isNtpClientSet() {
    return this->ntpClient != nullptr;
  }
#endif

  const char* getServiceTemplate() {
    return this->serviceTemplate;
  }

  void setServiceTemplate(const char* value) {
    this->serviceTemplate = value;
  }

  const char* getServiceDelimiter() {
    return this->serviceDelim;
  }

  void setServiceDelimiter(const char* value) {
    this->serviceTemplate = value;
  }

  void setServiceDelimiter() {
    this->serviceDelim = nullptr;
  }

  void setLevelTemplate(const char* value) {
    this->levelTemplate = value;
  }

  const char* getLevelTemplate() {
    return this->levelTemplate;
  }


  void flush() {
    for (Stream* stream : this->streams) {
      stream->flush();
    }
  }

  template <class T> void print(T msg) {
    for (Stream* stream : this->streams) {
      stream->print(msg);
    }
  }

  template <class T> void println(T msg) {
    for (Stream* stream : this->streams) {
      stream->println(msg);
    }
  }

  template <typename... Args> void printf(const __FlashStringHelper* msg, Args... args) {
    for (Stream* stream : this->streams) {
      if (sizeof...(args) > 0) {
        stream->printf_P(reinterpret_cast<PGM_P>(msg), args...);

      } else {
        stream->print(msg);
      }
    }
  }

  template <class T, typename... Args> void printf(T msg, Args... args) {
    for (Stream* stream : this->streams) {
      if (sizeof...(args) > 0) {
        stream->printf(msg, args...);
        
      } else {
        stream->print(msg);
      }
    }
  }

  void printService(const char* service) {
    if (service != nullptr && strlen(service) > 0) {
      if (this->serviceDelim != nullptr && strlen(this->serviceDelim) > 0 && strstr(service, this->serviceDelim) != NULL) {
        char* tmp = strdup(service);
        char* item = strtok(tmp, this->serviceDelim);
        
        while (item != NULL) {
          this->printf(this->serviceTemplate, item);
          item = strtok(NULL, this->serviceDelim);
        }
        free(tmp);

      } else {
        this->printf(this->serviceTemplate, service);
      }
    }
  }

  void printService(const __FlashStringHelper* service) {
    PGM_P pService = reinterpret_cast<PGM_P>(service);

    char buffer[strlen_P(pService) + 1];
    strcpy_P(buffer, pService);

    return this->printService(buffer);
  }

  template <class ST, class MT, typename... Args> void printFormatted(Level level, ST service, bool nl, MT msg, Args... args) {
    if (level > this->level) {
      return;
    }

#if defined(ESP32)
    while (this->isLocked()) {
      yield();
    }

    this->lock();
#endif

    if (this->dateTemplate != nullptr) {
      struct tm* tm = nullptr;
      if (this->isDateSet()) {
        tm = this->date;
      }

#if TINYLOGGER_NTPCLIENT
      if (tm == nullptr && this->isNtpClientSet()) {
        time_t now = this->ntpClient->getEpochTime();
        tm = localtime(&now);
      }
#endif

      if (tm != nullptr) {
        char* buffer = new char[64];
        if (strftime(buffer, sizeof(buffer), this->dateTemplate, tm) != 0) {
          this->print(buffer);
        }
        delete[] buffer;
      }
    }

    this->printService(service);    
    this->printf(this->levelTemplate, TinyLogger::level2str(level));
    this->print(" ");
    this->printf(msg, args...);

    if (nl) {
      this->print(this->nlChar);
    }

    this->flush();
#if defined(ESP32)
    this->unlock();
#endif
  }

  template <class T, typename... Args> void fatal(T msg, Args... args) {
    this->printFormatted(Level::FATAL, nullptr, false, msg, args...);
  }

  template <class ST, class MT, typename... Args> void sfatal(ST service, MT msg, Args... args) {
    this->printFormatted(Level::FATAL, service, false, msg, args...);
  }

  template <class T, typename... Args> void fatalln(T msg, Args... args) {
    this->printFormatted(Level::FATAL, nullptr, true, msg, args...);
  }

  template <class ST, class MT, typename... Args> void sfatalln(ST service, MT msg, Args... args) {
    this->printFormatted(Level::FATAL, service, true, msg, args...);
  }


  template <class T, typename... Args> void error(T msg, Args... args) {
    this->printFormatted(Level::ERROR, nullptr, false, msg, args...);
  }

  template <class ST, class MT, typename... Args> void serror(ST service, MT msg, Args... args) {
    this->printFormatted(Level::ERROR, service, false, msg, args...);
  }

  template <class T, typename... Args> void errorln(T msg, Args... args) {
    this->printFormatted(Level::ERROR, nullptr, true, msg, args...);
  }

  template <class ST, class MT, typename... Args> void serrorln(ST service, MT msg, Args... args) {
    this->printFormatted(Level::ERROR, service, true, msg, args...);
  }


  template <class T, typename... Args> void warning(T msg, Args... args) {
    this->printFormatted(Level::WARNING, nullptr, false, msg, args...);
  }

  template <class ST, class MT, typename... Args> void swarning(ST service, MT msg, Args... args) {
    this->printFormatted(Level::WARNING, service, false, msg, args...);
  }

  template <class T, typename... Args> void warningln(T msg, Args... args) {
    this->printFormatted(Level::WARNING, nullptr, true, msg, args...);
  }

  template <class ST, class MT, typename... Args> void swarningln(ST service, MT msg, Args... args) {
    this->printFormatted(Level::WARNING, service, true, msg, args...);
  }


  template <class T, typename... Args> void info(T msg, Args... args) {
    this->printFormatted(Level::INFO, nullptr, false, msg, args...);
  }

  template <class ST, class MT, typename... Args> void sinfo(ST service, MT msg, Args... args) {
    this->printFormatted(Level::INFO, service, false, msg, args...);
  }

  template <class T, typename... Args> void infoln(T msg, Args... args) {
    this->printFormatted(Level::INFO, nullptr, true, msg, args...);
  }

  template <class ST, class MT, typename... Args> void sinfoln(ST service, MT msg, Args... args) {
    this->printFormatted(Level::INFO, service, true, msg, args...);
  }


  template <class T, typename... Args> void notice(T msg, Args... args) {
    this->printFormatted(Level::NOTICE, nullptr, false, msg, args...);
  }

  template <class ST, class MT, typename... Args> void snotice(ST service, MT msg, Args... args) {
    this->printFormatted(Level::NOTICE, service, false, msg, args...);
  }

  template <class T, typename... Args> void noticeln(T msg, Args... args) {
    this->printFormatted(Level::NOTICE, nullptr, true, msg, args...);
  }

  template <class ST, class MT, typename... Args> void snoticeln(ST service, MT msg, Args... args) {
    this->printFormatted(Level::NOTICE, service, true, msg, args...);
  }


  template <class T, typename... Args> void trace(T msg, Args... args) {
    this->printFormatted(Level::TRACE, nullptr, false, msg, args...);
  }

  template <class ST, class MT, typename... Args> void strace(ST service, MT msg, Args... args) {
    this->printFormatted(Level::TRACE, service, false, msg, args...);
  }

  template <class T, typename... Args> void traceln(T msg, Args... args) {
    this->printFormatted(Level::TRACE, nullptr, true, msg, args...);
  }

  template <class ST, class MT, typename... Args> void straceln(ST service, MT msg, Args... args) {
    this->printFormatted(Level::TRACE, service, true, msg, args...);
  }


  template <class T, typename... Args> void verbose(T msg, Args... args) {
    this->printFormatted(Level::VERBOSE, nullptr, false, msg, args...);
  }

  template <class ST, class MT, typename... Args> void sverbose(ST service, MT msg, Args... args) {
    this->printFormatted(Level::VERBOSE, service, false, msg, args...);
  }

  template <class T, typename... Args> void verboseln(T msg, Args... args) {
    this->printFormatted(Level::VERBOSE, nullptr, true, msg, args...);
  }

  template <class ST, class MT, typename... Args> void sverboseln(ST service, MT msg, Args... args) {
    this->printFormatted(Level::VERBOSE, service, true, msg, args...);
  }

  static String level2str(Level level, const char* prefix = nullptr, const char* suffix = nullptr) {
    const char* str;

    switch (level) {
      default:
      case Level::SILENT:
        str = PSTR("SILENT");
        break;
      case Level::FATAL:
        str = PSTR("FATAL");
        break;
      case Level::ERROR:
        str = PSTR("ERROR");
        break;
      case Level::WARNING:
        str = PSTR("WARN");
        break;
      case Level::INFO:
        str = PSTR("INFO");
        break;
      case Level::TRACE:
        str = PSTR("TRACE");
        break;
      case Level::VERBOSE:
        str = PSTR("VERB");
        break;
    }

    return str;
  }

protected:
  std::vector<Stream*> streams;
  Level level = Level::ERROR;
  struct tm* date = nullptr;
#if defined(ESP32)
  bool locked = false;
  unsigned short lockTimeout = 1000;
  unsigned long lockedTime = 0;
#endif
#if TINYLOGGER_NTPCLIENT
  NTPClient* ntpClient = nullptr;
#endif
  const char* dateTemplate = "[%d.%m.%Y %H:%M:%S]";
  const char* serviceTemplate = "[%s]";
  const char* serviceDelim = ".";
  const char* levelTemplate = "[%s]";
  const char* nlChar = "\r\n";
};

TinyLogger Log = TinyLogger();
extern TinyLogger Log;