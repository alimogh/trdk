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

  class Generator {
   public:
    Generator() = default;
    Generator(Generator &&) = default;
    Generator(const Generator &) = delete;
    Generator &operator=(Generator &&) = delete;
    Generator &operator=(const Generator &) = delete;
    virtual ~Generator() = default;

    virtual Value TakeNextNonce() = 0;
  };
  class Int32SecondsGenerator : public Generator {
   public:
    Int32SecondsGenerator();
    Int32SecondsGenerator(Int32SecondsGenerator &&) = default;
    Int32SecondsGenerator(const Int32SecondsGenerator &) = delete;
    Int32SecondsGenerator &operator=(Int32SecondsGenerator &&) = delete;
    Int32SecondsGenerator &operator=(const Int32SecondsGenerator &) = delete;
    ~Int32SecondsGenerator() override = default;

    Value TakeNextNonce() override { return m_nextValue++; }

   private:
    int32_t m_nextValue;
  };
  class TRDK_INTERACTION_REST_API UnsignedInt64SecondsGenerator
      : public Generator {
   public:
    UnsignedInt64SecondsGenerator();
    UnsignedInt64SecondsGenerator(UnsignedInt64SecondsGenerator &&) = default;
    UnsignedInt64SecondsGenerator(const UnsignedInt64SecondsGenerator &) =
        delete;
    UnsignedInt64SecondsGenerator &operator=(UnsignedInt64SecondsGenerator &&) =
        delete;
    UnsignedInt64SecondsGenerator &operator=(
        const UnsignedInt64SecondsGenerator &) = delete;
    ~UnsignedInt64SecondsGenerator() override = default;

    Value TakeNextNonce() override { return m_nextValue++; }

   private:
    uint64_t m_nextValue;
  };
  class TRDK_INTERACTION_REST_API UnsignedInt64MicrosecondsGenerator
      : public Generator {
   public:
    UnsignedInt64MicrosecondsGenerator();
    UnsignedInt64MicrosecondsGenerator(UnsignedInt64MicrosecondsGenerator &&) =
        default;
    UnsignedInt64MicrosecondsGenerator(
        const UnsignedInt64MicrosecondsGenerator &) = delete;
    UnsignedInt64MicrosecondsGenerator &operator=(
        UnsignedInt64MicrosecondsGenerator &&) = delete;
    UnsignedInt64MicrosecondsGenerator &operator=(
        const UnsignedInt64MicrosecondsGenerator &) = delete;
    ~UnsignedInt64MicrosecondsGenerator() override = default;

    Value TakeNextNonce() override { return m_nextValue++; }

   private:
    uint64_t m_nextValue;
  };

  typedef boost::mutex Mutex;
  typedef Mutex::scoped_lock Lock;

  class TakenValue {
   public:
    TakenValue(const Value &value, Lock lock)
        : m_value(value), m_lock(std::move(lock)) {}
    TakenValue(TakenValue &&) = default;
    TakenValue(const TakenValue &) = delete;
    TakenValue &operator=(TakenValue &&) = delete;
    TakenValue &operator=(const TakenValue &) = delete;
    ~TakenValue() = default;

    const Value &Get() const { return m_value; }
    void Use() { m_lock.unlock(); }

   private:
    const Value m_value;
    Lock m_lock;
  };

  explicit NonceStorage(std::unique_ptr<Generator> &&);
  NonceStorage(NonceStorage &&) = delete;
  NonceStorage(const NonceStorage &) = delete;
  NonceStorage &operator=(NonceStorage &&) = delete;
  NonceStorage &operator=(const NonceStorage &) = delete;
  ~NonceStorage() = default;

  TakenValue TakeNonce();

 private:
  std::unique_ptr<Generator> m_generator;
  Mutex m_mutex;
};
}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
