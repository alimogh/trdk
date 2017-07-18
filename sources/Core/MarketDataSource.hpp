/**************************************************************************
 *   Created: 2012/07/15 13:20:35
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
#include "Interactor.hpp"

namespace trdk {

////////////////////////////////////////////////////////////////////////////////

//! Market data source factory.
/** Result can't be nullptr.
  */
typedef boost::shared_ptr<trdk::MarketDataSource>(MarketDataSourceFactory)(
    size_t index,
    trdk::Context &,
    const std::string &instanceName,
    const trdk::Lib::IniSectionRef &);

////////////////////////////////////////////////////////////////////////////////

class TRDK_CORE_API MarketDataSource : virtual public trdk::Interactor {
 public:
  typedef trdk::Interactor Base;

  typedef trdk::ModuleEventsLog Log;
  typedef trdk::ModuleTradingLog TradingLog;

 public:
  class TRDK_CORE_API Error : public Base::Error {
   public:
    explicit Error(const char *what) throw();
  };

 public:
  explicit MarketDataSource(size_t index,
                            trdk::Context &,
                            const std::string &instanceName);
  virtual ~MarketDataSource();

  bool operator==(const MarketDataSource &rhs) const { return this == &rhs; }
  bool operator!=(const MarketDataSource &rhs) const {
    return !operator==(rhs);
  }

  TRDK_CORE_API friend std::ostream &operator<<(std::ostream &,
                                                const trdk::MarketDataSource &);

 public:
  size_t GetIndex() const;

  trdk::Context &GetContext();
  const trdk::Context &GetContext() const;

  trdk::MarketDataSource::Log &GetLog() const throw();
  trdk::MarketDataSource::TradingLog &GetTradingLog() const throw();

  //! Identifies Market Data Source object by verbose name.
  /** Market Data Source instance name is unique, but can be empty.
    */
  const std::string &GetInstanceName() const;
  const std::string &GetStringId() const throw();

 public:
  //! Makes connection with Market Data Source.
  virtual void Connect(const trdk::Lib::IniSectionRef &) = 0;

  virtual void SubscribeToSecurities() = 0;

 public:
  //! Returns security without expiration date, creates new object if it
  //! doesn't exist yet.
  /** @sa trdk::MarketDataSource::FindSecurity
    * @sa trdk::MarketDataSource::GetActiveSecurityCount
    */
  trdk::Security &GetSecurity(const trdk::Lib::Symbol &);
  //! Returns security with expiration date, creates new object if it
  //! doesn't exist yet.
  /** @sa trdk::MarketDataSource::FindSecurity
    * @sa trdk::MarketDataSource::GetActiveSecurityCount
    */
  trdk::Security &GetSecurity(const trdk::Lib::Symbol &,
                              const trdk::Lib::ContractExpiration &);

  //! Finds security without expiration date.
  /** Doesn't search security with expiration date.
    * @sa trdk::MarketDataSource::GetSecurity
    * @sa trdk::MarketDataSource::GetActiveSecurityCount
    * @return nullptr if security object doesn't exist.
    */
  trdk::Security *FindSecurity(const trdk::Lib::Symbol &);
  //! Finds security without expiration date.
  /** @sa trdk::MarketDataSource::GetSecurity
    * @sa trdk::MarketDataSource::GetActiveSecurityCount
    * @return nullptr if security object doesn't exist.
    */
  const trdk::Security *FindSecurity(const trdk::Lib::Symbol &) const;
  //! Finds security with expiration date.
  /** Doesn't search security with expiration date.
    * @sa trdk::MarketDataSource::GetSecurity
    * @sa trdk::MarketDataSource::GetActiveSecurityCount
    * @return nullptr if security object doesn't exist.
    */
  trdk::Security *FindSecurity(const trdk::Lib::Symbol &,
                               const trdk::Lib::ContractExpiration &);
  //! Finds security with expiration date.
  /** @sa trdk::MarketDataSource::GetSecurity
    * @sa trdk::MarketDataSource::GetActiveSecurityCount
    * @return nullptr if security object doesn't exist.
    */
  const trdk::Security *FindSecurity(
      const trdk::Lib::Symbol &, const trdk::Lib::ContractExpiration &) const;

  size_t GetActiveSecurityCount() const;

  void ForEachSecurity(
      const boost::function<bool(const trdk::Security &)> &) const;

  //! Switches security to the next contract ordered by expiration date.
  /** @param security  Security to switch. Should be not explicit.
    */
  virtual void SwitchToNextContract(trdk::Security &security);

 protected:
  trdk::Security &CreateSecurity(const trdk::Lib::Symbol &);

  //! Creates security object.
  /** Each object, that implements CreateNewSecurityObject should wait
    * for log flushing before destroying objects.
    */
  virtual trdk::Security &CreateNewSecurityObject(
      const trdk::Lib::Symbol &) = 0;

 private:
  class Implementation;
  Implementation *const m_pimpl;
};

////////////////////////////////////////////////////////////////////////////////
}
