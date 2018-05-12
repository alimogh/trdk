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

class TRDK_INTERACTION_REST_API NonceStorage : boost::noncopyable {
 public:
  typedef std::uint64_t Value;

  class Generator : boost::noncopyable {
   public:
    virtual ~Generator() = default;

    virtual Value TakeNextNonce() = 0;
  };
  class Int32TimedGenerator : public Generator {
   public:
    Int32TimedGenerator();
    ~Int32TimedGenerator() override = default;

    Value TakeNextNonce() override { return m_nextValue++; }

   private:
    int32_t m_nextValue;
  };
  class TRDK_INTERACTION_REST_API UnsignedInt64TimedGenerator
      : public Generator {
   public:
    UnsignedInt64TimedGenerator();
    ~UnsignedInt64TimedGenerator() override = default;

    Value TakeNextNonce() override { return m_nextValue++; }

   private:
    uint64_t m_nextValue;
  };

  typedef boost::mutex Mutex;
  typedef Mutex::scoped_lock Lock;

  class TakenValue {
   public:
    TakenValue(Value &&value, Lock &&lock)
        : m_value(std::move(value)), m_lock(std::move(lock)) {}
    TakenValue(TakenValue &&) = default;
    TakenValue(const TakenValue &) = delete;
    TakenValue &operator=(TakenValue &&) = delete;
    TakenValue &operator=(const TakenValue &) = delete;

    const Value &Get() const { return m_value; }
    void Use() { m_lock.unlock(); }

   private:
    const Value m_value;
    Lock m_lock;
  };

  explicit NonceStorage(std::unique_ptr<Generator> &&);
  NonceStorage(NonceStorage &&) = default;

  TakenValue TakeNonce();

 private:
  std::unique_ptr<Generator> m_generator;
  Mutex m_mutex;
};
}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
