/*******************************************************************************
 *   Created: 2017/09/30 01:41:03
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "ShellDropCopy.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::FrontEnd::Shell;

namespace sh = trdk::FrontEnd::Shell;
namespace pt = boost::posix_time;
namespace ids = boost::uuids;

sh::DropCopy::DropCopy(QObject *parent) : QObject(parent) {}

void sh::DropCopy::Flush() {}

void sh::DropCopy::Dump() {}

DropCopyStrategyInstanceId sh::DropCopy::RegisterStrategyInstance(
    const Strategy &) {
  return 0;
}

DropCopyStrategyInstanceId sh::DropCopy::ContinueStrategyInstance(
    const Strategy &, const pt::ptime &) {
  return 0;
}

DropCopyDataSourceInstanceId sh::DropCopy::RegisterDataSourceInstance(
    const Strategy &, const ids::uuid &, const ids::uuid &) {
  return 0;
}

void sh::DropCopy::CopyOrder(const ids::uuid &,
                             const std::string *,
                             const pt::ptime &,
                             const pt::ptime *,
                             const OrderStatus &,
                             const ids::uuid &,
                             const int64_t *,
                             const Security &,
                             const TradingSystem &,
                             const OrderSide &,
                             const Qty &,
                             const Price *,
                             const TimeInForce *,
                             const Lib::Currency &,
                             const Qty *,
                             const Qty &,
                             const Price *,
                             const Qty *,
                             const Price *,
                             const Qty *) {}

void sh::DropCopy::CopyTrade(const pt::ptime &,
                             const std::string &,
                             const ids::uuid &,
                             const Price &,
                             const Qty &,
                             const Price &,
                             const Qty &,
                             const Price &,
                             const Qty &) {}

void sh::DropCopy::ReportOperationStart(const Strategy &,
                                        const ids::uuid &,
                                        const pt::ptime &) {}

void sh::DropCopy::ReportOperationEnd(const ids::uuid &,
                                      const pt::ptime &,
                                      const CloseReason &,
                                      const OperationResult &,
                                      const Volume &,
                                      FinancialResult &&) {}

void sh::DropCopy::CopyBook(const Security &, const PriceBook &) {}

void sh::DropCopy::CopyBar(const DropCopyDataSourceInstanceId &,
                           size_t,
                           const pt::ptime &,
                           const Price &,
                           const Price &,
                           const Price &,
                           const Price &) {}

void sh::DropCopy::CopyAbstractData(const DropCopyDataSourceInstanceId &,
                                    size_t,
                                    const pt::ptime &,
                                    const Double &) {}

void sh::DropCopy::CopyLevel1Tick(const Security &security,
                                  const pt::ptime &,
                                  const trdk::Level1TickValue &) {
  emit PriceUpdate(security);
}
void sh::DropCopy::CopyLevel1Tick(const Security &security,
                                  const pt::ptime &,
                                  const Level1TickValue &,
                                  const Level1TickValue &) {
  emit PriceUpdate(security);
}
void sh::DropCopy::CopyLevel1Tick(const Security &security,
                                  const pt::ptime &,
                                  const Level1TickValue &,
                                  const Level1TickValue &,
                                  const Level1TickValue &) {
  emit PriceUpdate(security);
}
void sh::DropCopy::CopyLevel1Tick(const Security &security,
                                  const pt::ptime &,
                                  const Level1TickValue &,
                                  const Level1TickValue &,
                                  const Level1TickValue &,
                                  const Level1TickValue &) {
  emit PriceUpdate(security);
}
void sh::DropCopy::CopyLevel1Tick(const Security &security,
                                  const pt::ptime &,
                                  const std::vector<Level1TickValue> &) {
  emit PriceUpdate(security);
}
