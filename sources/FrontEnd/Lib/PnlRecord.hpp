//
//    Created: 2018/06/17 3:04 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once
#ifndef TRDK_FRONTEND_LIB_INCLUDED_PNLRECORD
#define TRDK_FRONTEND_LIB_INCLUDED_PNLRECORD

#ifdef ODB_COMPILER
#include "OperationRecord.hpp"
#endif

namespace trdk {
namespace FrontEnd {

class PnlRecord {
#ifdef TRDK_FRONTEND_LIB
  friend class odb::access;
#endif

 public:
  typedef std::uintmax_t Id;

 private:
  PnlRecord();

 public:
  explicit PnlRecord(QString symbol);
  PnlRecord(PnlRecord&&) noexcept;
  PnlRecord(const PnlRecord&);
  PnlRecord& operator=(PnlRecord&&) noexcept;
  PnlRecord& operator=(const PnlRecord&);
  ~PnlRecord();
  void Swap(PnlRecord&) noexcept;

  const Id& GetId() const;

  const QString& GetSymbol() const;

  const boost::shared_ptr<const OperationRecord>& GetOperation() const;

  const Volume& GetFinancialResult() const;
  void SetFinancialResult(Volume);

  const Volume& GetCommission() const;
  void SetCommission(Volume);

 private:
  void SetIdValue(const Id&);
  void SetSymbolValue(QString);

  void SetOperationValue(boost::shared_ptr<const OperationRecord>);

#ifndef ODB_COMPILER

  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;

#else

#pragma db object(PnlRecord) table("pnl")

#pragma db get(GetId) set(SetIdValue) id auto
  Id id;
#pragma db get(GetSymbol) set(SetSymbolValue(std::move(?))) not_null
  QString symbol;
#pragma db column("financial_result") get(GetFinancialResult) \
    set(SetFinancialResult) not_null
  Volume::ValueType financialResult;
#pragma db get(GetCommission) set(SetCommission) not_null
  Volume::ValueType commission;
#pragma db get(GetOperation) \
    set(SetOperationValue(std::move(?))) value_not_null not_null
  boost::shared_ptr<const OperationRecord> operation;

#pragma db index("unique_pnl_index") unique members(operation, symbol)

#endif
};

}  // namespace FrontEnd
}  // namespace trdk

#endif
