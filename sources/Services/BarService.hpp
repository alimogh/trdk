/**************************************************************************
 *   Created: 2016/09/20 22:21:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Service.hpp"
#include "Api.h"

namespace trdk {
namespace Services {

//! Bars collection service.
class TRDK_SERVICES_API BarService : public trdk::Service {
 public:
  typedef trdk::Service Base;

  //! General service error.
  class Error : public trdk::Lib::Exception {
   public:
    explicit Error(const char *) throw();
  };

  //! Throws when client code requests bar which does not exist.
  class BarDoesNotExistError : public Error {
   public:
    explicit BarDoesNotExistError(const char *) throw();
  };

  //! Bar data.
  struct TRDK_SERVICES_API Bar {
    boost::posix_time::ptime time;

    trdk::ScaledPrice maxAskPrice;
    trdk::ScaledPrice openAskPrice;
    trdk::ScaledPrice closeAskPrice;

    trdk::ScaledPrice minBidPrice;
    trdk::ScaledPrice openBidPrice;
    trdk::ScaledPrice closeBidPrice;

    trdk::ScaledPrice openTradePrice;
    trdk::ScaledPrice closeTradePrice;

    trdk::ScaledPrice highTradePrice;
    trdk::ScaledPrice lowTradePrice;

    trdk::Qty tradingVolume;

    Bar();
  };

 public:
  explicit BarService(Context &context,
                      const std::string &typeUuid,
                      const std::string &implementationName,
                      const std::string &instanceName,
                      const Lib::IniSectionRef &);
  virtual ~BarService();

 public:
  //! Number of bars.
  virtual size_t GetSize() const = 0;

  virtual bool IsEmpty() const = 0;

 public:
  virtual const trdk::Security &GetSecurity() const = 0;

  //! Returns bar by index.
  /** First bar has index "zero".
    * @throw trdk::Services::BarService::BarDoesNotExistError
    * @sa trdk::Services::BarService::GetBarByReversedIndex
    */
  virtual Bar GetBar(size_t index) const = 0;

  //! Returns bar by reversed index.
  /** Last bar has index "zero".
    * @throw trdk::Services::BarService::BarDoesNotExistError
    * @sa trdk::Services::BarService::GetBarByIndex
    */
  virtual Bar GetBarByReversedIndex(size_t index) const = 0;

  //! Returns last bar.
  /** @throw trdk::Services::BarService::BarDoesNotExistError
    * @sa trdk::Services::BarService::GetBarByReversedIndex
    */
  virtual Bar GetLastBar() const = 0;

  virtual void DropLastBarCopy(
      const trdk::DropCopyDataSourceInstanceId &) const = 0;
  virtual void DropUncompletedBarCopy(
      const trdk::DropCopyDataSourceInstanceId &) const = 0;
};
}
}
