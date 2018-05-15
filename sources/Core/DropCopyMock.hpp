/**************************************************************************
 *   Created: 2016/09/10 16:22:24
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "DropCopy.hpp"
#include "PriceBook.hpp"
#include "Security.hpp"
#include "Strategy.hpp"

namespace trdk {
namespace Tests {
namespace Mocks {

class DropCopy : public trdk::DropCopy {
 public:
  ~DropCopy() override = default;

  MOCK_METHOD0(Flush, void());
  MOCK_METHOD0(Dump, void());

  MOCK_METHOD9(CopySubmittedOrder,
               void(const OrderId &,
                    const boost::posix_time::ptime &,
                    const Security &,
                    const Lib::Currency &,
                    const TradingSystem &,
                    const OrderSide &,
                    const Qty &,
                    const boost::optional<Price> &,
                    const TimeInForce &));
  MOCK_METHOD7(CopySubmittedOrder,
               void(const OrderId &,
                    const boost::posix_time::ptime &,
                    const Position &,
                    const OrderSide &,
                    const Qty &,
                    const boost::optional<Price> &,
                    const TimeInForce &));
  MOCK_METHOD9(CopyOrderSubmitError,
               void(const boost::posix_time::ptime &,
                    const Security &,
                    const Lib::Currency &,
                    const TradingSystem &,
                    const OrderSide &,
                    const Qty &,
                    const boost::optional<Price> &,
                    const TimeInForce &,
                    const std::string &error));
  MOCK_METHOD7(CopyOrderSubmitError,
               void(const boost::posix_time::ptime &,
                    const Position &,
                    const OrderSide &,
                    const Qty &,
                    const boost::optional<Price> &,
                    const TimeInForce &,
                    const std::string &error));
  MOCK_METHOD5(CopyOrderStatus,
               void(const OrderId &,
                    const TradingSystem &,
                    const boost::posix_time::ptime &,
                    const OrderStatus &,
                    const Qty &));

  void CopyOrder(const OrderId &,
                 const TradingSystem &,
                 const std::string &,
                 const OrderStatus &,
                 const Qty &,
                 const Qty &,
                 const boost::optional<Price> &,
                 const OrderSide &,
                 const TimeInForce &,
                 const boost::posix_time::ptime &,
                 const boost::posix_time::ptime &) override {}

  MOCK_METHOD6(CopyTrade,
               void(const boost::posix_time::ptime &,
                    const boost::optional<std::string> &,
                    const OrderId &,
                    const TradingSystem &,
                    const Price &,
                    const Qty &));

  MOCK_METHOD3(CopyOperationStart,
               void(const boost::uuids::uuid &,
                    const boost::posix_time::ptime &,
                    const Strategy &));
  MOCK_METHOD2(CopyOperationUpdate,
               void(const boost::uuids::uuid &, const Pnl::Data &));

  void CopyOperationEnd(const boost::uuids::uuid &id,
                        const boost::posix_time::ptime &time,
                        std::unique_ptr<Pnl> &&pnl) override {
    CopyOperationEnd(id, time, *pnl);
  }

  MOCK_METHOD3(CopyOperationEnd,
               void(const boost::uuids::uuid &,
                    const boost::posix_time::ptime &,
                    const Pnl &));

  MOCK_METHOD2(CopyBook, void(const Security &, const PriceBook &));

  MOCK_METHOD7(CopyBar, void(const Security &, const Bar &));

  MOCK_METHOD3(CopyLevel1,
               void(const Security &,
                    const boost::posix_time::ptime &,
                    const Level1TickValue &));
  MOCK_METHOD4(CopyLevel1,
               void(const Security &,
                    const boost::posix_time::ptime &,
                    const Level1TickValue &,
                    const Level1TickValue &));
  MOCK_METHOD5(CopyLevel1,
               void(const Security &,
                    const boost::posix_time::ptime &,
                    const Level1TickValue &,
                    const Level1TickValue &,
                    const Level1TickValue &));
  MOCK_METHOD6(CopyLevel1,
               void(const Security &,
                    const boost::posix_time::ptime &,
                    const Level1TickValue &,
                    const Level1TickValue &,
                    const Level1TickValue &,
                    const Level1TickValue &));
  MOCK_METHOD3(CopyLevel1,
               void(const Security &,
                    const boost::posix_time::ptime &,
                    const std::vector<Level1TickValue> &));

  MOCK_METHOD4(CopyBalance,
               void(const TradingSystem &,
                    const std::string &symbol,
                    const Volume &,
                    const Volume &));
};

}  // namespace Mocks
}  // namespace Tests
}  // namespace trdk
