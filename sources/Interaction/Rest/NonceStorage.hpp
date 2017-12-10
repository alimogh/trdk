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
    Value initialNonce;
    boost::filesystem::path nonceStorageFile;

    explicit Settings(const Lib::IniSectionRef &conf, ModuleEventsLog &log)
        : initialNonce(conf.ReadTypedKey<Value>("initial_nonce", 1)),
          nonceStorageFile(conf.ReadFileSystemPath("nonce_storage_file_path")) {
      Log(log);
      Validate();
    }

    void Log(ModuleEventsLog &log) {
      log.Info("Initial nonce: %1%. Nonce storage  file: %2%",
               initialNonce,       // 1
               nonceStorageFile);  // 2
    }

    void Validate() {
      if (initialNonce <= 0) {
        throw TradingSystem::Error("Initial nonce could not be less than 1");
      }
    }
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
