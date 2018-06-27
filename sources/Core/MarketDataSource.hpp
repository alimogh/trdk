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

class TRDK_CORE_API MarketDataSource : virtual public Interactor {
 public:
  typedef Interactor Base;

  typedef ModuleEventsLog Log;
  typedef ModuleTradingLog TradingLog;

  explicit MarketDataSource(Context&,
                            std::string instanceName,
                            std::string title);
  MarketDataSource(MarketDataSource&&);
  MarketDataSource(const MarketDataSource&) = delete;
  MarketDataSource& operator=(MarketDataSource&&) = delete;
  MarketDataSource& operator=(const MarketDataSource&) = delete;
  ~MarketDataSource() override;

  bool operator==(const MarketDataSource& rhs) const { return this == &rhs; }

  bool operator!=(const MarketDataSource& rhs) const {
    return !operator==(rhs);
  }

  TRDK_CORE_API friend std::ostream& operator<<(std::ostream&,
                                                const MarketDataSource&);

  void AssignIndex(size_t);
  size_t GetIndex() const;

  Context& GetContext();
  const Context& GetContext() const;

  Log& GetLog() const noexcept;
  TradingLog& GetTradingLog() const noexcept;

  //! Identifies Market Data Source object by verbose name.
  /** Market Data Source instance name is unique, but can be empty.
   */
  const std::string& GetInstanceName() const;

  const std::string& GetTitle() const;

  const std::string& GetStringId() const noexcept;

  //! Makes connection with Market Data Source.
  virtual void Connect() = 0;

  virtual void SubscribeToSecurities() = 0;

  //! Returns security without expiration date, creates new object if it
  //! doesn't exist yet.
  /**
   * @sa trdk::MarketDataSource::FindSecurity
   * @sa trdk::MarketDataSource::GetActiveSecurityCount
   */
  Security& GetSecurity(const Lib::Symbol&);
  //! Returns security with expiration date, creates new object if it
  //! doesn't exist yet.
  /**
   * @sa trdk::MarketDataSource::FindSecurity
   * @sa trdk::MarketDataSource::GetActiveSecurityCount
   */
  Security& GetSecurity(const Lib::Symbol&, const Lib::ContractExpiration&);

  //! Finds security without expiration date.
  /**
   * Doesn't search security with expiration date.
   * @sa trdk::MarketDataSource::GetSecurity
   * @sa trdk::MarketDataSource::GetActiveSecurityCount
   * @return nullptr if security object doesn't exist.
   */
  Security* FindSecurity(const Lib::Symbol&);
  //! Finds security without expiration date.
  /**
   * @sa trdk::MarketDataSource::GetSecurity
   * @sa trdk::MarketDataSource::GetActiveSecurityCount
   * @return nullptr if security object doesn't exist.
   */
  const Security* FindSecurity(const Lib::Symbol&) const;
  //! Finds security with expiration date.
  /**
   * Doesn't search security with expiration date.
   * @sa trdk::MarketDataSource::GetSecurity
   * @sa trdk::MarketDataSource::GetActiveSecurityCount
   * @return nullptr if security object doesn't exist.
   */
  Security* FindSecurity(const Lib::Symbol&, const Lib::ContractExpiration&);
  //! Finds security with expiration date.
  /**
   * @sa trdk::MarketDataSource::GetSecurity
   * @sa trdk::MarketDataSource::GetActiveSecurityCount
   * @return nullptr if security object doesn't exist.
   */
  const Security* FindSecurity(const Lib::Symbol&,
                               const Lib::ContractExpiration&) const;

  size_t GetActiveSecurityCount() const;

  void ForEachSecurity(const boost::function<void(const Security&)>&) const;
  void ForEachSecurity(const boost::function<void(Security&)>&);

  //! Starts the asynchronous operation of switching security to the contract
  //! with the next expiry date.
  /** @param[in,out] security Security to switch. Should be not explicit.
   */
  void SwitchToNextContract(Security& security) const;

 protected:
  Security& CreateSecurity(const Lib::Symbol&);

  //! Creates security object.
  /**
   * Each object, that implements CreateNewSecurityObject should wait for log
   * flushing before destroying objects.
   */
  virtual Security& CreateNewSecurityObject(const Lib::Symbol&) = 0;

  //! Finds an expiration.
  /**
   * Returns expiration by any previous date, even if contract is not started
   * yet.
   */
  virtual boost::optional<Lib::ContractExpiration> FindContractExpiration(
      const Lib::Symbol&, const boost::gregorian::date&) const;

  //! Switches security to the contract with specified expiration.
  /**
   * @param[in,out] security   Security to switch. Should be not explicit.
   * @param[in,out] expiration Required expiration.
   */
  virtual void SwitchToContract(
      Security& security, const Lib::ContractExpiration&& expiration) const;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

////////////////////////////////////////////////////////////////////////////////
}  // namespace trdk
