//
//    Created: 2018/07/06 10:11 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "PnlRecord.hpp"

using namespace trdk;
using namespace FrontEnd;

class PnlRecord::Implementation {
 public:
  QString m_symbol;
  Id m_id = std::numeric_limits<Id>::max();
  odb::lazy_shared_ptr<const OperationRecord> m_operation;
  Volume m_financialResult = 0;
  Volume m_commission = 0;

  Implementation() = default;
  explicit Implementation(QString symbol,
                          boost::shared_ptr<const OperationRecord> operation)
      : m_symbol(std::move(symbol)), m_operation(std::move(operation)) {}
  Implementation(Implementation&&) = default;
  Implementation(const Implementation&) = default;
  Implementation& operator=(Implementation&&) = default;
  Implementation& operator=(const Implementation&) = default;
  ~Implementation() = default;
};

PnlRecord::PnlRecord() : m_pimpl(boost::make_unique<Implementation>()) {}
PnlRecord::PnlRecord(QString symbol,
                     boost::shared_ptr<const OperationRecord> operation)
    : m_pimpl(boost::make_unique<Implementation>(std::move(symbol),
                                                 std::move(operation))) {}
PnlRecord::PnlRecord(PnlRecord&&) noexcept = default;
PnlRecord::PnlRecord(const PnlRecord& rhs)
    : m_pimpl(boost::make_unique<Implementation>(*rhs.m_pimpl)) {}
PnlRecord& PnlRecord::operator=(PnlRecord&&) noexcept = default;
PnlRecord& PnlRecord::operator=(const PnlRecord& rhs) {
  PnlRecord(rhs).Swap(*this);
  return *this;
}
PnlRecord::~PnlRecord() = default;
void PnlRecord::Swap(PnlRecord& rhs) noexcept { m_pimpl.swap(rhs.m_pimpl); }

const PnlRecord::Id& PnlRecord::GetId() const { return m_pimpl->m_id; }
void PnlRecord::SetIdValue(const Id& id) { m_pimpl->m_id = id; }

const QString& PnlRecord::GetSymbol() const { return m_pimpl->m_symbol; }
void PnlRecord::SetSymbolValue(QString symbol) {
  m_pimpl->m_symbol = std::move(symbol);
}

const odb::lazy_shared_ptr<const OperationRecord>& PnlRecord::GetOperation()
    const {
  return m_pimpl->m_operation;
}
void PnlRecord::SetOperationValue(
    odb::lazy_shared_ptr<const OperationRecord> operation) {
  m_pimpl->m_operation = std::move(operation);
}

const Volume& PnlRecord::GetFinancialResult() const {
  return m_pimpl->m_financialResult;
}
void PnlRecord::SetFinancialResult(Volume financialResult) {
  m_pimpl->m_financialResult = std::move(financialResult);
}

const Volume& PnlRecord::GetCommission() const { return m_pimpl->m_commission; }
void PnlRecord::SetCommission(Volume comission) {
  m_pimpl->m_commission = std::move(comission);
}
