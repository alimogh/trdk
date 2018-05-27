/**************************************************************************
 *   Created: 2012/09/23 14:31:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Api.h"
#include "Context.hpp"
#include "Security.hpp"

////////////////////////////////////////////////////////////////////////////////

namespace trdk {

class TRDK_CORE_API Module : boost::noncopyable {
 public:
  typedef ModuleEventsLog Log;
  typedef ModuleTradingLog TradingLog;

  class TRDK_CORE_API SecurityList;

  typedef uintmax_t InstanceId;

  typedef boost::mutex Mutex;
  typedef Mutex::scoped_lock Lock;

  explicit Module(Context &,
                  const std::string &typeName,
                  const std::string &implementationName,
                  const std::string &instanceName,
                  const boost::property_tree::ptree &);
  virtual ~Module();

  TRDK_CORE_API friend std::ostream &operator<<(std::ostream &, const Module &);

  const boost::uuids::uuid &GetId() const;
  const InstanceId &GetInstanceId() const;
  const std::string &GetImplementationName() const;
  const std::string &GetInstanceName() const noexcept;

  Context &GetContext();
  const Context &GetContext() const;

  const std::string &GetStringId() const noexcept;

  Log &GetLog() const noexcept;
  TradingLog &GetTradingLog() const noexcept;
  //! Opens file stream to log module data and reports and reports file
  //! path to log.
  std::ofstream OpenDataLog(const std::string &fileExtension,
                            const std::string &suffix = std::string()) const;

  Lock LockForOtherThreads();

  //! Returns list of required services.
  virtual std::string GetRequiredSuppliers() const;

  void RaiseSettingsUpdateEvent(const boost::property_tree::ptree &);

 protected:
  Lock LockForOtherThreads() const {
    return const_cast<Module *>(this)->LockForOtherThreads();
  }

  virtual void OnSettingsUpdate(const boost::property_tree::ptree &);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace trdk

////////////////////////////////////////////////////////////////////////////////

class TRDK_CORE_API trdk::Module::SecurityList {
 public:
  class ConstIterator;

  class TRDK_CORE_API Iterator
      : public boost::iterator_facade<Iterator,
                                      Security,
                                      boost::incrementable_traversal_tag> {
    friend class ConstIterator;

   public:
    class Implementation;

    explicit Iterator(std::unique_ptr<Implementation> &&);
    Iterator(const Iterator &);
    ~Iterator();

    Iterator &operator=(const Iterator &);
    void Swap(Iterator &);

    Security &dereference() const;
    bool equal(const Iterator &) const;
    bool equal(const ConstIterator &) const;
    void increment();

   private:
    std::unique_ptr<Implementation> m_pimpl;
  };

  class TRDK_CORE_API ConstIterator
      : public boost::iterator_facade<ConstIterator,
                                      const Security,
                                      boost::incrementable_traversal_tag> {
    friend class Iterator;

   public:
    class Implementation;

    explicit ConstIterator(std::unique_ptr<Implementation> &&);
    explicit ConstIterator(const Iterator &);
    ConstIterator(const ConstIterator &);
    ~ConstIterator();

    ConstIterator &operator=(const ConstIterator &);
    void Swap(ConstIterator &);

    const Security &dereference() const;
    bool equal(const Iterator &) const;
    bool equal(const ConstIterator &) const;
    void increment();

   private:
    std::unique_ptr<Implementation> m_pimpl;
  };

  virtual ~SecurityList() = default;

  Iterator begin() { return GetBegin(); }
  ConstIterator cbegin() const { return GetBegin(); }

  Iterator end() { return GetEnd(); }
  ConstIterator cend() const { return GetEnd(); }

  virtual size_t GetSize() const = 0;
  virtual bool IsEmpty() const = 0;

  virtual Iterator GetBegin() = 0;
  virtual ConstIterator GetBegin() const = 0;

  virtual Iterator GetEnd() = 0;
  virtual ConstIterator GetEnd() const = 0;
};

//////////////////////////////////////////////////////////////////////////
