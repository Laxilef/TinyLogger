#pragma once
#include <Arduino.h>
#include <vector>
#if defined(ARDUINO_ARCH_ESP32)
  #include <mutex>
  #include <chrono>
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

  void begin(Stream* stream, Level level = Level::ERROR) {
    this->streams.push_back(stream);
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

  virtual void printLevel(Level level) {
    const __FlashStringHelper* str;

    switch (level) {
      default:
      case Level::SILENT:
        str = F("SILENT");
        break;
      case Level::FATAL:
        str = F("FATAL");
        break;
      case Level::ERROR:
        str = F("ERROR");
        break;
      case Level::WARNING:
        str = F("WARN");
        break;
      case Level::INFO:
        str = F("INFO");
        break;
      case Level::NOTICE:
        str = F("NOTICE");
        break;
      case Level::TRACE:
        str = F("TRACE");
        break;
      case Level::VERBOSE:
        str = F("VERB");
        break;
    }

    this->printf(this->levelTemplate, str);
  }

  template <class ST, class MT, typename... Args> void printFormatted(Level level, ST service, bool nl, MT msg, Args... args) {
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

protected:
  std::vector<Stream*> streams;
  Level level = Level::ERROR;
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

TinyLogger Log = TinyLogger();
extern TinyLogger Log;