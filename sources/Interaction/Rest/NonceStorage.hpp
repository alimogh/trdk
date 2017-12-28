/*******************************************************************************
 *   Created: 2017/12/09 02:14:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Interaction {
namespace Rest {

class NonceStorage : private boost::noncopyable {
 public:
  typedef std::int32_t Value;

  struct Settings {
    bool isTrading;
    Value initialNonce;
    boost::filesystem::path nonceStorageFile;

    explicit Settings(const std::string &keyName,
                      const std::string &tag,
                      const Lib::IniSectionRef &conf,
                      ModuleEventsLog &log,
                      bool isTrading = false)
        : isTrading(isTrading),
          initialNonce(conf.ReadTypedKey<Value>(
              !isTrading ? "initial_nonce" : "initial_trading_nonce", 1)),
          nonceStorageFile(conf.ReadFileSystemPath(
                               !isTrading ? "nonce_storage_dir"
                                          : "trading_nonce_storage_dir") /
                           (tag + "_" + keyName + ".nonce")) {
      Log(log);
      Validate();
    }

    void Log(ModuleEventsLog &log) {
      log.Debug("%1% nonce: %2%. Nonce storage file: %3%.",
                !isTrading ? "Initial" : "Trading initial",  // 1
                initialNonce,                                // 2
                nonceStorageFile);                           // 3
    }

    void Validate() {}
  };

  typedef boost::mutex Mutex;
  typedef Mutex::scoped_lock Lock;

  class TakenValue {
   public:
    TakenValue(const Value &&value, Lock &&lock)
        : m_value(std::move(value)), m_lock(std::move(lock)) {}

   public:
    const Value &Get() const { return m_value; }
    void Use() { m_lock.unlock(); }

   private:
    const Value m_value;
    Lock m_lock;
  };

 public:
  explicit NonceStorage(const Settings &, ModuleEventsLog &);
  NonceStorage(NonceStorage &&) = default;

 public:
  TakenValue TakeNonce();

 private:
  Value m_value;
  std::ofstream m_storage;
  Mutex m_mutex;
};
}
}
}
