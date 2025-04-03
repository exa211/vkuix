#pragma once

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <utility>
#include <sstream>
#include <unordered_map>

//! Debug flag, If VOXLE_DEBUG_LOGGING is enabled, debug output should be printed.
#ifndef NDEBUG
#define VOXLE_DEBUG_LOGGING
#endif

#define VOXLE_GET_KEY() std::string(__FILE__) + ":" + std::to_string(__LINE__)
#define VOXLE_LOGPREFIX voxle::logging::getFileName(__FILE__) << ":" << __LINE__ << " "
namespace voxle::logging {
  enum class Severity {
    D, S, I, W, E, F
  };

// Constexpr function to extract just the file name from the full path
  inline static constexpr const char *getFileName(const char *path) {
    const char *file = path;
    while (*path) {
#ifdef _WIN32
      if (*path == '\\') {
#else
        if (*path == '/') {
#endif
        file = path + 1;
      }
      ++path;
    }
    return file;
  }
}

//! Hack to enable macro overloading. Used to overload LOG() macro.
#define CAT(A, B) A##B
#define SELECT(NAME, NUM) CAT(NAME##_, NUM)

#define GET_COUNT(_1, _2, _3, _4, _5, _6, COUNT, ...) COUNT
#define VA_SIZE(...) GET_COUNT(__VA_ARGS__, 6, 5, 4, 3, 2, 1)
#define VA_SELECT(NAME, ...) SELECT(NAME, VA_SIZE(__VA_ARGS__))(__VA_ARGS__)

//! Overloads
#define LOG(...) VA_SELECT(LOG, __VA_ARGS__)

#define LOG_EVERY(severity, n, x) voxle::logging::InternalLogCount::getInstance().update(                       \
      VOXLE_GET_KEY(), n, voxle::logging::InternalLog() << VOXLE_LOGPREFIX << x, voxle::logging::Severity::severity, voxle::logging::PolicyType::EVERY_N) // NOLINT(bugprone-macro-parentheses)
#define LOG_FIRST(severity, n, x) voxle::logging::InternalLogCount::getInstance().update(                       \
      VOXLE_GET_KEY(), n, voxle::logging::InternalLog() << VOXLE_LOGPREFIX << x, voxle::logging::Severity::severity, voxle::logging::PolicyType::FIRST_N) // NOLINT(bugprone-macro-parentheses)
#define LOG_TIMED(severity, n, x) voxle::logging::InternalLogCount::getInstance().update(                       \
      VOXLE_GET_KEY(), n, voxle::logging::InternalLog() << VOXLE_LOGPREFIX << x, voxle::logging::Severity::severity, voxle::logging::PolicyType::TIMED) // NOLINT(bugprone-macro-parentheses)

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"
#define LOG_2(severity, x) voxle::logging::InternalLog(voxle::logging::Severity::severity) << VOXLE_LOGPREFIX << x // NOLINT(bugprone-macro-parentheses)
#define LOG_3(severity, cond, x) if (cond) voxle::logging::InternalLog(voxle::logging::Severity::severity) << VOXLE_LOGPREFIX << x // NOLINT(bugprone-macro-parentheses)
#pragma clang diagnostic pop

namespace voxle::logging {
  struct Logging {
    inline static void call(Severity severity, const std::string &str) {
      switch (severity) {
        case Severity::D:
#ifdef VOXLE_DEBUG_LOGGING
          std::cout << "\x1B[32m[DEBUG] " << str << "\033[0m" << std::endl;
#endif
          return;
        case Severity::I:
          std::cout << "\x1B[37m[INFO]  " << str << "\033[0m" << std::endl;
          return;
        case Severity::S:
          std::cout << "\x1B[94m[SUCCESS]  " << str << "\033[0m" << std::endl;
          return;
        case Severity::W:
          std::cout << "\x1B[33m[WARN]  " << str << "\033[0m" << std::endl;
          return;
        case Severity::E:
          std::cout << "\x1B[31m[ERROR] " << str << "\033[0m" << std::endl;
          return;
        case Severity::F:
          std::cout << "\x1B[91m[FATAL] " << str << "\033[0m" << std::endl;
          abort();
      }
    }
  };

//! Composes a string with the same text that would be printed if format was used on printf(3)
  template<typename... Args>
  inline std::string formatToString(const char *f, Args... args) {
    size_t sz = snprintf(nullptr, 0, f, args...);
    if (sz == 0) {
      return "";
    }
    char *buf = (char *) malloc(sz + 1);
    snprintf(buf, sz + 1, f, args...);
    return buf;
  }

  inline std::string formatToString(const char *str) { return str; }

//! Internal log class
  class InternalLog {
  public:
    InternalLog() { should_print_ = false; };

    explicit InternalLog(Severity base_severity) : severity_(base_severity) {}

    InternalLog(InternalLog const &log) : severity_(log.severity_) {
      ss << log.ss.str();
    }

    Severity severity_{};
    std::stringstream ss{};

    virtual ~InternalLog() {
      if (!should_print_) {
        return;
      }
      voxle::logging::Logging::call(severity_, ss.str());
    }

  protected:
    bool should_print_{true};
  };

  template<typename T>
  InternalLog &&operator<<(InternalLog &&wrap, T const &whatever) {
    wrap.ss << whatever;
    return std::move(wrap);
  }

  class LogPolicy {
  public:
    virtual void update() = 0;

    [[nodiscard]] virtual bool shouldLog() const = 0;

    virtual void onLog() {};

    virtual ~LogPolicy() = default;

  protected:
    explicit LogPolicy(int max) : max_(max) {}

    int counter_{0};
    int max_{0};
  };

  class OccasionPolicy : public LogPolicy {
  public:
    explicit OccasionPolicy(int max) : LogPolicy(max) { counter_ = max_; }

    inline void update() override {
      should_log_ = false;

      if (counter_ % max_ == 0) {
        should_log_ = true;
        counter_ = 0;
      }
      counter_++;
    }

    [[nodiscard]] inline bool shouldLog() const override { return should_log_; }

  private:
    bool should_log_{false};
  };

  class FirstNOccurrencesPolicy : public LogPolicy {
  public:
    explicit FirstNOccurrencesPolicy(int max) : LogPolicy(max) {}

    inline void update() override {
      if (!is_n_occurences_reached) {
        counter_++;
      }

      if (counter_ > max_) {
        is_n_occurences_reached = true;
      }
    }

    [[nodiscard]] inline bool shouldLog() const override {
      return !is_n_occurences_reached;
    }

  private:
    bool is_n_occurences_reached = false;
  };

  using namespace std::chrono;

  class TimePolicy : public LogPolicy {
  public:
    explicit TimePolicy(int max) : LogPolicy(max) {};

    inline void update() override {
      now_ = duration_cast<microseconds>(system_clock::now().time_since_epoch())
        .count();
    }

    [[nodiscard]] inline bool shouldLog() const override {
      if (now_ >= last_ + (max_ * 1000000)) {
        return true;
      }
      return false;
    }

    void onLog() override { last_ = now_; }

  private:
    long now_{0};
    long last_{0};
  };

  enum PolicyType {
    FIRST_N, EVERY_N, TIMED
  };

  typedef std::shared_ptr<LogPolicy> LogPolicyPtr;

  class LogPolicyFactory {
  public:
    static LogPolicyPtr create(PolicyType policy_type, int max) {
      switch (policy_type) {
        case FIRST_N:
          return std::make_shared<FirstNOccurrencesPolicy>(max);
        case EVERY_N:
          return std::make_shared<OccasionPolicy>(max);
        case TIMED:
          return std::make_shared<TimePolicy>(max);
        default:
          abort();
      }
    }
  };

  struct LogStatementData {
    LogStatementData(LogPolicyPtr log_policy, Severity severity_type)
      : log_policy_(std::move(log_policy)), severity_type_(severity_type) {}

    LogPolicyPtr log_policy_;
    std::string msg{};
    Severity severity_type_;
  };

  class InternalLogCount {
  public:
    static InternalLogCount &getInstance() {
      static InternalLogCount instance;
      return instance;
    }

    inline void update(const std::string &key, int max,
                       const InternalLog &internal_log,
                       const Severity base_severity, PolicyType policy_type) {
      update(key, max, internal_log.ss.str(), base_severity, policy_type);
    }

    inline void update(const std::string &key, int max,
                       const std::string &log_msg,
                       const Severity base_severity, PolicyType policy_type) {

      updatePolicy(key, max, log_msg, base_severity, policy_type);
      mtx_.lock();
      logIfReady(key, log_msg);
      mtx_.unlock();
    }

    inline void updatePolicy(const std::string &key, int max,
                             const std::string &log_msg,
                             Severity base_severity, PolicyType policy_type) {
      mtx_.lock();
      if (!keyExists(key)) {
        LogStatementData data(LogPolicyFactory::create(policy_type, max),
                              base_severity);
        data.msg = log_msg;
        updateLogPolicyData(&data);
        occurences_.insert({key, data});
      } else {
        LogStatementData *data = &occurences_.at(key);
        updateLogPolicyData(data);
      }
      mtx_.unlock();
    }

    inline bool shouldLog(const std::string &key) {
      mtx_.lock();
      bool res = occurences_.at(key).log_policy_->shouldLog();
      mtx_.unlock();
      return res;
    }

    inline void log(const std::string &key) {
      mtx_.lock();
      occurences_.at(key).log_policy_->onLog();
      mtx_.unlock();
    }

  private:
    inline bool keyExists(const std::string &key) {
      return occurences_.find(key) != occurences_.end();
    }

    static inline void updateLogPolicyData(LogStatementData *data) {
      data->log_policy_->update();
    }

    inline void logIfReady(const std::string &key, const std::string &log_msg) {
      LogStatementData *data = &occurences_.at(key);
      if (data->log_policy_->shouldLog()) {
        data->log_policy_->onLog();
        data->msg = log_msg;
        InternalLog(data->severity_type_) << data->msg;
      }
    }

    InternalLogCount() = default;

    std::unordered_map<std::string, LogStatementData> occurences_{};
    std::mutex mtx_{};
  };


  class InternalPolicyLog : public InternalLog {
  public:
    InternalPolicyLog(std::string key, int n, Severity base_severity,
                      PolicyType policy_type)
      : key_(std::move(key)), n_(n), policy_type_(policy_type),
        InternalLog(base_severity) {
      should_print_ = false;
    };

    ~InternalPolicyLog() override {
      if (should_update_) {
        InternalLogCount::getInstance().update(key_, n_, ss.str(), severity_, policy_type_);
      }
    }

  protected:
    bool should_update_{true};
    std::string key_{};
    int n_{};
    PolicyType policy_type_{};
  };
}
