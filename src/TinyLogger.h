#pragma once
#include <Arduino.h>
#include <vector>
#if defined(ARDUINO_ARCH_ESP32)
  #include <mutex>
  #include <chrono>
#endif

enum class TinyLoggerLevel : uint8_t {
  SILENT,
  FATAL,
  ERROR,
  WARNING,
  INFO,
  NOTICE,
  TRACE,
  VERBOSE
};

auto operator<=>(TinyLoggerLevel level, uint8_t val) {
  return (static_cast<uint8_t>(level) <=> val);
}

auto operator==(TinyLoggerLevel level, uint8_t val) {
  return (level <=> val) == std::strong_ordering::equal;
}


template <typename THandler = Print> class TinyLogger {
public:
  TinyLogger() {
#if defined(ARDUINO_ARCH_ESP32)
    this->mutex = new std::timed_mutex();
#endif
  }

  ~TinyLogger() {
#if defined(ARDUINO_ARCH_ESP32)
    delete this->mutex;
#endif
  }

  void begin(THandler* handler, TinyLoggerLevel level = TinyLoggerLevel::ERROR) {
    this->handlers.push_back(handler);
    this->level = level;
  }

  bool lock() {
#if defined(ARDUINO_ARCH_ESP32)
    return this->mutex->try_lock_for(std::chrono::milliseconds(this->tryLockTimeout));
#else
    return true;
#endif
  }

  void unlock() {
#if defined(ARDUINO_ARCH_ESP32)
    this->mutex->unlock();
#endif
  }

  void setTryLockTimeout(unsigned short val) {
    this->tryLockTimeout = val;
  }

  void addHandler(THandler* handler) {
    this->handlers.push_back(handler);
  }

  void clearHandlers() {
    this->handlers.clear();
  }

  const std::vector<THandler*> getHandlers() {
    return this->handlers;
  }

  TinyLoggerLevel getLevel() {
    return this->level;
  }

  void setLevel(TinyLoggerLevel level) {
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

  void setDateCallback(std::function<tm()> callback) {
    this->dateCallback = callback;
  }

  void setDateCallback() {
    this->dateCallback = nullptr;
  }

  bool isDateCallbackSet() {
    return this->dateCallback != nullptr;
  }

  const char* getServiceDelimiter() {
    return this->serviceDelim;
  }

  void setServiceDelimiter(const char* value) {
    this->serviceDelim = value;
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

  const char* getDateTemplate() {
    return this->dateTemplate;
  }

  void setDateTemplate(const char* value) {
    this->dateTemplate = value;
  }

  const char* getServiceTemplate() {
    return this->serviceTemplate;
  }

  void setServiceTemplate(const char* value) {
    this->serviceTemplate = value;
  }

  const char* getMsgPrefix() {
    return this->msgPrefix;
  }

  void setMsgPrefix() {
    this->msgPrefix = nullptr;
  }

  void setMsgPrefix(const char* value) {
    this->msgPrefix = value;
  }

  const char* getMsgSuffix() {
    return this->msgSuffix;
  }

  void setMsgSuffix() {
    this->msgSuffix = nullptr;
  }

  void setMsgSuffix(const char* value) {
    this->msgSuffix = value;
  }


  void flush() {
    for (THandler* handler : this->handlers) {
      handler->flush();
    }
  }

  template <class T> void print(T msg) {
    for (THandler* handler : this->handlers) {
      handler->print(msg);
    }
  }

  template <class T> void println(T msg) {
    for (THandler* handler : this->handlers) {
      handler->println(msg);
    }
  }

  template <typename... Args> void printf(const __FlashStringHelper* msg, Args... args) {
    for (THandler* handler : this->handlers) {
      if (sizeof...(args) > 0) {
        handler->printf_P(reinterpret_cast<PGM_P>(msg), args...);

      } else {
        handler->print(msg);
      }
    }
  }

  template <class T, typename... Args> void printf(T msg, Args... args) {
    for (THandler* handler : this->handlers) {
      if (sizeof...(args) > 0) {
        handler->printf(msg, args...);
        
      } else {
        handler->print(msg);
      }
    }
  }

  void printService(nullptr_t service) {}

  virtual void printService(const char* service) {
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

  virtual void printService(const __FlashStringHelper* service) {
    PGM_P pService = reinterpret_cast<PGM_P>(service);

    char buffer[strlen_P(pService) + 1];
    strcpy_P(buffer, pService);

    return this->printService(buffer);
  }

  virtual void printLevel(TinyLoggerLevel level) {
    const __FlashStringHelper* str;

    switch (level) {
      default:
      case TinyLoggerLevel::SILENT:
        str = F("SILENT");
        break;
      case TinyLoggerLevel::FATAL:
        str = F("FATAL");
        break;
      case TinyLoggerLevel::ERROR:
        str = F("ERROR");
        break;
      case TinyLoggerLevel::WARNING:
        str = F("WARN");
        break;
      case TinyLoggerLevel::INFO:
        str = F("INFO");
        break;
      case TinyLoggerLevel::NOTICE:
        str = F("NOTICE");
        break;
      case TinyLoggerLevel::TRACE:
        str = F("TRACE");
        break;
      case TinyLoggerLevel::VERBOSE:
        str = F("VERB");
        break;
    }

    this->printf(this->levelTemplate, str);
  }

  template <class ST, class MT, typename... Args> void printFormatted(TinyLoggerLevel level, ST service, bool nl, MT msg, Args... args) {
    if (level > this->level) {
      return;
    }

    while(!this->lock()) {
      yield();
    }

    if (this->dateTemplate != nullptr) {
      struct tm* tm = nullptr;
      if (this->isDateSet()) {
        tm = this->date;
      }

      struct tm cTm;
      if (tm == nullptr && this->isDateCallbackSet()) {
        cTm = this->dateCallback();
        tm = &cTm;
      }

      if (tm != nullptr) {
        char buffer[64];
        if (strftime(buffer, sizeof(buffer), this->dateTemplate, tm) != 0) {
          this->print(buffer);
        }
      }
    }

    this->printService(service);
    this->printLevel(level);

    if (this->msgPrefix) {
      this->print(this->msgPrefix);
    }

    this->printf(msg, args...);

    if (this->msgSuffix != nullptr) {
      this->print(this->msgSuffix);
    }

    if (nl) {
      this->print(this->nlChar);
    }

    this->flush();
    this->unlock();
  }

  template <class T, typename... Args> void fatal(T msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::FATAL, nullptr, false, msg, args...);
  }

  template <class ST, class MT, typename... Args> void sfatal(ST service, MT msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::FATAL, service, false, msg, args...);
  }

  template <class T, typename... Args> void fatalln(T msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::FATAL, nullptr, true, msg, args...);
  }

  template <class ST, class MT, typename... Args> void sfatalln(ST service, MT msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::FATAL, service, true, msg, args...);
  }


  template <class T, typename... Args> void error(T msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::ERROR, nullptr, false, msg, args...);
  }

  template <class ST, class MT, typename... Args> void serror(ST service, MT msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::ERROR, service, false, msg, args...);
  }

  template <class T, typename... Args> void errorln(T msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::ERROR, nullptr, true, msg, args...);
  }

  template <class ST, class MT, typename... Args> void serrorln(ST service, MT msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::ERROR, service, true, msg, args...);
  }


  template <class T, typename... Args> void warning(T msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::WARNING, nullptr, false, msg, args...);
  }

  template <class ST, class MT, typename... Args> void swarning(ST service, MT msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::WARNING, service, false, msg, args...);
  }

  template <class T, typename... Args> void warningln(T msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::WARNING, nullptr, true, msg, args...);
  }

  template <class ST, class MT, typename... Args> void swarningln(ST service, MT msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::WARNING, service, true, msg, args...);
  }


  template <class T, typename... Args> void info(T msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::INFO, nullptr, false, msg, args...);
  }

  template <class ST, class MT, typename... Args> void sinfo(ST service, MT msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::INFO, service, false, msg, args...);
  }

  template <class T, typename... Args> void infoln(T msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::INFO, nullptr, true, msg, args...);
  }

  template <class ST, class MT, typename... Args> void sinfoln(ST service, MT msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::INFO, service, true, msg, args...);
  }


  template <class T, typename... Args> void notice(T msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::NOTICE, nullptr, false, msg, args...);
  }

  template <class ST, class MT, typename... Args> void snotice(ST service, MT msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::NOTICE, service, false, msg, args...);
  }

  template <class T, typename... Args> void noticeln(T msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::NOTICE, nullptr, true, msg, args...);
  }

  template <class ST, class MT, typename... Args> void snoticeln(ST service, MT msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::NOTICE, service, true, msg, args...);
  }


  template <class T, typename... Args> void trace(T msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::TRACE, nullptr, false, msg, args...);
  }

  template <class ST, class MT, typename... Args> void strace(ST service, MT msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::TRACE, service, false, msg, args...);
  }

  template <class T, typename... Args> void traceln(T msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::TRACE, nullptr, true, msg, args...);
  }

  template <class ST, class MT, typename... Args> void straceln(ST service, MT msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::TRACE, service, true, msg, args...);
  }


  template <class T, typename... Args> void verbose(T msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::VERBOSE, nullptr, false, msg, args...);
  }

  template <class ST, class MT, typename... Args> void sverbose(ST service, MT msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::VERBOSE, service, false, msg, args...);
  }

  template <class T, typename... Args> void verboseln(T msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::VERBOSE, nullptr, true, msg, args...);
  }

  template <class ST, class MT, typename... Args> void sverboseln(ST service, MT msg, Args... args) {
    this->printFormatted(TinyLoggerLevel::VERBOSE, service, true, msg, args...);
  }

protected:
  std::vector<THandler*> handlers;
  TinyLoggerLevel level = TinyLoggerLevel::ERROR;
  struct tm* date = nullptr;
#if defined(ARDUINO_ARCH_ESP32)
  mutable std::timed_mutex* mutex;
#endif
  unsigned short tryLockTimeout = 50;
  std::function<tm()> dateCallback = nullptr;
  const char* dateTemplate = "[%d.%m.%Y %H:%M:%S]";
  const char* serviceTemplate = "[%s]";
  const char* serviceDelim = ".";
  const char* levelTemplate = "[%s]";
  const char* nlChar = "\r\n";
  const char* msgPrefix = " ";
  const char* msgSuffix = nullptr;
};

#ifndef TINYLOGGER_GLOBAL_NAME
  #define TINYLOGGER_GLOBAL_NAME Log
#endif

#ifndef TINYLOGGER_GLOBAL_HANDLER
  #define TINYLOGGER_GLOBAL_HANDLER Print
#endif

#ifdef TINYLOGGER_GLOBAL
  auto TINYLOGGER_GLOBAL_NAME = TinyLogger<TINYLOGGER_GLOBAL_HANDLER>();
  extern TinyLogger<TINYLOGGER_GLOBAL_HANDLER> TINYLOGGER_GLOBAL_NAME;
#endif