/**************************************************************************
 *   Created: 2016/09/10 16:22:24
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "DropCopy.hpp"
#include "PriceBook.hpp"
#include "Security.hpp"
#include "Strategy.hpp"

namespace trdk {
namespace Tests {
namespace Mocks {

class DropCopy : public trdk::DropCopy {
 public:
  virtual ~DropCopy() override = default;

 public:
  MOCK_METHOD0(Flush, void());
  MOCK_METHOD0(Dump, void());

  MOCK_METHOD1(RegisterStrategyInstance,
               DropCopyStrategyInstanceId(const trdk::Strategy &));
  MOCK_METHOD2(ContinueStrategyInstance,
               DropCopyStrategyInstanceId(const trdk::Strategy &,
                                          const boost::posix_time::ptime &));
  MOCK_METHOD3(RegisterDataSourceInstance,
               DropCopyDataSourceInstanceId(const trdk::Strategy &,
                                            const boost::uuids::uuid &type,
                                            const boost::uuids::uuid &id));

  virtual void CopyOrder(const boost::uuids::uuid &,
                         const std::string *,
                         const boost::posix_time::ptime &,
                         const boost::posix_time::ptime *,
                         const trdk::OrderStatus &,
                         const boost::uuids::uuid &,
                         const int64_t *,
                         const trdk::Security &,
                         const trdk::TradingSystem &,
                         const trdk::OrderSide &,
                         const trdk::Qty &,
                         const trdk::Price *,
                         const trdk::TimeInForce *,
                         const trdk::Lib::Currency &,
                         const trdk::Qty &,
                         const trdk::Price *,
                         const trdk::Qty *,
                         const trdk::Price *,
                         const trdk::Qty *) {
    //! @todo Make wrapper to struct to test this method.
    throw std::exception("Method is not mocked");
  }

  MOCK_METHOD9(CopyTrade,
               void(const boost::posix_time::ptime &,
                    const std::string &tradingSystemTradeId,
                    const boost::uuids::uuid &orderId,
                    const trdk::Price &price,
                    const trdk::Qty &qty,
                    const trdk::Price &bestBidPrice,
                    const trdk::Qty &bestBidQty,
                    const trdk::Price &bestAskPrice,
                    const trdk::Qty &bestAskQty));

  MOCK_METHOD3(ReportOperationStart,
               void(const trdk::Strategy &,
                    const boost::uuids::uuid &id,
                    const boost::posix_time::ptime &));
  void ReportOperationEnd(const boost::uuids::uuid &id,
                          const boost::posix_time::ptime &time,
                          const trdk::CloseReason &reason,
                          const trdk::OperationResult &result,
                          const trdk::Volume &pnl,
                          trdk::FinancialResult &&financialResult) {
    ReportOperationEnd(id, time, reason, result, pnl, financialResult);
  }
  MOCK_METHOD6(ReportOperationEnd,
               void(const boost::uuids::uuid &id,
                    const boost::posix_time::ptime &,
                    const trdk::CloseReason &,
                    const trdk::OperationResult &,
                    const trdk::Volume &pnl,
                    const trdk::FinancialResult &));

  MOCK_METHOD2(CopyBook, void(const trdk::Security &, const trdk::PriceBook &));

  MOCK_METHOD7(CopyBar,
               void(const trdk::DropCopyDataSourceInstanceId &,
                    size_t index,
                    const boost::posix_time::ptime &,
                    const trdk::Price &,
                    const trdk::Price &,
                    const trdk::Price &,
                    const trdk::Price &));
  MOCK_METHOD4(CopyAbstractData,
               void(const trdk::DropCopyDataSourceInstanceId &,
                    size_t index,
                    const boost::posix_time::ptime &,
                    const trdk::Lib::Double &value));

  MOCK_METHOD3(CopyLevel1,
               void(const trdk::Security &,
                    const boost::posix_time::ptime &,
                    const trdk::Level1TickValue &));
  MOCK_METHOD4(CopyLevel1,
               void(const trdk::Security &,
                    const boost::posix_time::ptime &,
                    const trdk::Level1TickValue &,
                    const trdk::Level1TickValue &));
  MOCK_METHOD5(CopyLevel1,
               void(const trdk::Security &,
                    const boost::posix_time::ptime &,
                    const trdk::Level1TickValue &,
                    const trdk::Level1TickValue &,
                    const trdk::Level1TickValue &));
  MOCK_METHOD6(CopyLevel1,
               void(const trdk::Security &,
                    const boost::posix_time::ptime &,
                    const trdk::Level1TickValue &,
                    const trdk::Level1TickValue &,
                    const trdk::Level1TickValue &,
                    const trdk::Level1TickValue &));
  MOCK_METHOD3(CopyLevel1,
               void(const trdk::Security &,
                    const boost::posix_time::ptime &,
                    const std::vector<trdk::Level1TickValue> &));
};
}
}
}
