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

  MOCK_METHOD9(CopySubmittedOrder,
               void(const trdk::OrderId &,
                    const boost::posix_time::ptime &,
                    const trdk::Security &,
                    const trdk::Lib::Currency &,
                    const trdk::TradingSystem &,
                    const trdk::OrderSide &,
                    const trdk::Qty &,
                    const boost::optional<trdk::Price> &,
                    const trdk::TimeInForce &));
  MOCK_METHOD7(CopySubmittedOrder,
               void(const trdk::OrderId &,
                    const boost::posix_time::ptime &,
                    const trdk::Position &,
                    const trdk::OrderSide &,
                    const trdk::Qty &,
                    const boost::optional<trdk::Price> &,
                    const trdk::TimeInForce &));
  MOCK_METHOD5(CopyOrderStatus,
               void(const trdk::OrderId &,
                    const trdk::TradingSystem &,
                    const boost::posix_time::ptime &,
                    const trdk::OrderStatus &,
                    const trdk::Qty &));
  virtual void CopyOrder(const trdk::OrderId &,
                         const trdk::TradingSystem &,
                         const std::string &,
                         const trdk::OrderStatus &,
                         const trdk::Qty &,
                         const trdk::Qty &,
                         const boost::optional<trdk::Price> &,
                         const trdk::OrderSide &,
                         const trdk::TimeInForce &,
                         const boost::posix_time::ptime &,
                         const boost::posix_time::ptime &) override {}

  MOCK_METHOD6(CopyTrade,
               void(const boost::posix_time::ptime &,
                    const boost::optional<std::string> &,
                    const trdk::OrderId &,
                    const trdk::TradingSystem &,
                    const trdk::Price &,
                    const trdk::Qty &));

  MOCK_METHOD3(CopyOperationStart,
               void(const boost::uuids::uuid &,
                    const boost::posix_time::ptime &,
                    const trdk::Strategy &));
  MOCK_METHOD2(CopyOperationUpdate,
               void(const boost::uuids::uuid &, const trdk::Pnl::Data &));
  virtual void CopyOperationEnd(const boost::uuids::uuid &id,
                                const boost::posix_time::ptime &time,
                                std::unique_ptr<Pnl> &&pnl) override {
    CopyOperationEnd(id, time, *pnl);
  }
  MOCK_METHOD3(CopyOperationEnd,
               void(const boost::uuids::uuid &,
                    const boost::posix_time::ptime &,
                    const trdk::Pnl &));

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

  MOCK_METHOD4(CopyBalance,
               void(const trdk::TradingSystem &,
                    const std::string &symbol,
                    const trdk::Volume &,
                    const trdk::Volume &));
};
}  // namespace Mocks
}  // namespace Tests
}  // namespace trdk
