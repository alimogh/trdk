/*******************************************************************************
 *   Created: 2018/01/27 16:08:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "OperationModelUtils.hpp"

using namespace trdk;
using namespace FrontEnd;
using namespace Detail;

namespace pt = boost::posix_time;
namespace ids = boost::uuids;

namespace {

void AddToBalanceString(const QString& symbol,
                        const Volume& value,
                        const bool showPlus,
                        QString& destination) {
  if (!value) {
    return;
  }
  if (!destination.isEmpty()) {
    destination += ", ";
  }
  if (showPlus && value > 0) {
    destination += '+';
  }
  {
    std::ostringstream os;
    os << value;
    auto strValue = os.str();
    boost::trim_right_if(strValue, [](char ch) { return ch == '0'; });
    if (!strValue.empty() &&
        (strValue.back() == '.' || strValue.back() == ',')) {
      strValue.pop_back();
    }
    if (strValue.empty()) {
      strValue = "0.00000001";
    }
    destination += QString::fromStdString(std::move(strValue));
  }
  destination += " " + symbol;
}

}  // namespace

OperationRecord::OperationRecord(const Orm::Operation& operation)
    : id(operation.getId().toString()),
      startTime(ConvertFromDbDateTime(operation.getStartTime())),
      strategyName(operation.getStrategyInstance()->getName()) {
  Update(operation);
}

void OperationRecord::Update(const Orm::Operation& operation) {
  endTime = ConvertFromDbDateTime(operation.getEndTime());
  {
    financialResult.clear();
    commission.clear();
    totalResult.clear();
    for (const auto& pnl : operation.getPnl()) {
      const auto& symbol = pnl->getSymbol();
      AddToBalanceString(symbol, pnl->getFinancialResult(), true,
                         financialResult);
      AddToBalanceString(symbol, pnl->getCommission(), false, commission);
      AddToBalanceString(symbol,
                         pnl->getFinancialResult() - pnl->getCommission(), true,
                         totalResult);
    }
  }
  {
    status = operation.getStatus();
    static_assert(Orm::OperationStatus::numberOfStatuses == 6, "List changed.");
    switch (status) {
      case Orm::OperationStatus::ACTIVE:
        statusName = QObject::tr("active");
        break;
      case Orm::OperationStatus::CANCELED:
        statusName = QObject::tr("canceled");
        break;
      case Orm::OperationStatus::PROFIT:
        statusName = QObject::tr("profit");
        break;
      case Orm::OperationStatus::LOSS:
        statusName = QObject::tr("loss");
        break;
      case Orm::OperationStatus::COMPLETED:
        statusName = QObject::tr("completed");
        break;
      default:
        AssertEq(Orm::OperationStatus::ERROR, status);
      case Orm::OperationStatus::ERROR:
        statusName = QObject::tr("error");
        break;
    }
  }
}
