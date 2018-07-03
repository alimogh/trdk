//
//    Created: 2018/04/07 3:47 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "TotalResultsReportModel.hpp"
#include "Engine.hpp"
#include "OperationRecord.hpp"
#include "OperationStatus.hpp"
#include "PnlRecord.hpp"

using namespace trdk::FrontEnd;
namespace mi = boost::multi_index;

namespace {
enum Column {
  COLUMN_SYMBOL,
  COLUMN_FINANCIAL_RESULT,
  COLUMN_COMMISSION,
  COLUMN_TOTAL,
  numberOfColumns
};

class Item {
 public:
  explicit Item(boost::shared_ptr<const PnlRecord> pnl)
      : m_symbol(pnl->GetSymbol()),
        m_finResult(pnl->GetFinancialResult()),
        m_commission(pnl->GetCommission()) {
    auto operationId = pnl->GetOperation()->GetId();
    m_data.emplace(std::move(operationId), std::move(pnl));
  }

  const QString& GetSymbol() const { return m_symbol; }

  void Update(const boost::shared_ptr<const PnlRecord>& pnl) {
    const auto& it = m_data.find(pnl->GetOperation()->GetId());
    if (it != m_data.cend()) {
      auto& record = it->second;
      m_finResult -= record->GetFinancialResult();
      m_commission -= record->GetCommission();
      record = pnl;
    } else {
      m_data.emplace(pnl->GetOperation()->GetId(), pnl);
    }
    m_finResult += pnl->GetFinancialResult();
    m_commission += pnl->GetCommission();
  }

  QVariant GetData(const Column& column) const {
    static_assert(numberOfColumns == 4, "List changed.");
    switch (column) {
      case COLUMN_SYMBOL:
        return m_symbol;
      case COLUMN_FINANCIAL_RESULT:
        return m_finResult.Get();
      case COLUMN_COMMISSION:
        return m_commission.Get();
      case COLUMN_TOTAL:
        return (m_finResult - m_commission).Get();
      default:
        return {};
    }
  }

 private:
  QString m_symbol;
  trdk::Volume m_finResult;
  trdk::Volume m_commission;
  boost::unordered_map<QUuid, boost::shared_ptr<const PnlRecord>> m_data;
};  // namespace

struct BySymbol {};
struct BySequence {};

struct ItemContainer {
  boost::shared_ptr<Item> item;

  const QString& GetSymbol() const { return item->GetSymbol(); }
};

typedef boost::multi_index_container<
    ItemContainer,
    mi::indexed_by<
        mi::hashed_unique<mi::tag<BySymbol>,
                          mi::const_mem_fun<ItemContainer,
                                            const QString&,
                                            &ItemContainer::GetSymbol>>,
        mi::random_access<mi::tag<BySequence>>>>
    Data;

}  // namespace

class TotalResultsReportModel::Implementation : boost::noncopyable {
 public:
  TotalResultsReportModel& m_self;
  const Engine& m_engine;

  QDateTime m_startTime;
  boost::optional<QDateTime> m_endTime;

  Data m_data;

  explicit Implementation(TotalResultsReportModel& self, const Engine& engine)
      : m_self(self), m_engine(engine) {}

  void Update(const std::vector<boost::shared_ptr<const PnlRecord>>& update) {
    auto& data = m_data.get<BySymbol>();
    for (const auto& pnl : update) {
      const auto& it = data.find(pnl->GetSymbol());
      if (it == data.cend()) {
        const auto index = static_cast<int>(m_data.size());
        m_self.beginInsertRows(QModelIndex(), index, index);
        data.emplace(ItemContainer{boost::make_shared<Item>(pnl)});
        m_self.endInsertRows();
      } else {
        const auto index = static_cast<int>(std::distance(
            m_data.get<BySequence>().cbegin(), m_data.project<BySequence>(it)));
        auto& record = const_cast<Item&>(*it->item);
        record.Update(pnl);
        emit m_self.dataChanged(
            m_self.createIndex(index, 0, &record),
            m_self.createIndex(index, numberOfColumns - 1, &record));
      }
    }
  }
};

TotalResultsReportModel::TotalResultsReportModel(Engine& engine,
                                                 QObject* parent)
    : Base(parent), m_pimpl(boost::make_unique<Implementation>(*this, engine)) {
  Verify(connect(&m_pimpl->m_engine, &Engine::OperationUpdated, this,
                 &TotalResultsReportModel::UpdateOperation));
}

TotalResultsReportModel::~TotalResultsReportModel() = default;

void TotalResultsReportModel::Build(const QDateTime& start,
                                    const boost::optional<QDateTime>& end,
                                    const boost::optional<QString>& strategy) {
  {
    beginResetModel();
    m_pimpl->m_data.clear();
    endResetModel();
  }
  m_pimpl->m_startTime = start;
  m_pimpl->m_endTime = end;
  for (const auto& operation :
       m_pimpl->m_engine.GetOperations(m_pimpl->m_startTime, m_pimpl->m_endTime,
                                       true, true, true, strategy)) {
    UpdateOperation(operation);
  }
}

void TotalResultsReportModel::UpdateOperation(
    const boost::shared_ptr<const OperationRecord>& operation) {
  if (!(operation->GetStartTime() >= m_pimpl->m_startTime &&
        (!m_pimpl->m_endTime ||
         operation->GetStartTime() <= *m_pimpl->m_endTime)) &&
      !(operation->GetStatus() == +OperationStatus::Active &&
        !m_pimpl->m_endTime) &&
      !(operation->GetStatus() != +OperationStatus::Active) &&
      !(operation->GetEndTime() >= m_pimpl->m_startTime &&
        (!m_pimpl->m_endTime ||
         operation->GetEndTime() <= *m_pimpl->m_endTime))) {
    return;
  }
  m_pimpl->Update(operation->GetPnl());
}

int TotalResultsReportModel::rowCount(const QModelIndex&) const {
  return static_cast<int>(m_pimpl->m_data.size());
}

int TotalResultsReportModel::columnCount(const QModelIndex&) const {
  return numberOfColumns;
}

QModelIndex TotalResultsReportModel::parent(const QModelIndex&) const {
  return {};
}

QVariant TotalResultsReportModel::headerData(const int section,
                                             const Qt::Orientation orientation,
                                             const int role) const {
  if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
    return Base::headerData(section, orientation, role);
  }
  static_assert(numberOfColumns == 4, "List changed.");
  switch (section) {
    case COLUMN_SYMBOL:
      return tr("Symbol");
    case COLUMN_FINANCIAL_RESULT:
      return tr("Financial Result");
    case COLUMN_COMMISSION:
      return tr("Commission");
    case COLUMN_TOTAL:
      return tr("Total Result");
    default:
      return {};
  }
}

QModelIndex TotalResultsReportModel::index(const int row,
                                           const int column,
                                           const QModelIndex&) const {
  if (column < 0 || column > numberOfColumns || row < 0 ||
      row >= static_cast<int>(m_pimpl->m_data.size())) {
    return {};
  }
  return createIndex(
      row, column,
      &const_cast<Item&>(
          *(m_pimpl->m_data.get<BySequence>().begin() + row)->item));
}

QVariant TotalResultsReportModel::data(const QModelIndex& index,
                                       const int role) const {
  if (!index.isValid() || index.column() < 0 ||
      index.column() >= numberOfColumns) {
    return {};
  }
  switch (role) {
    case Qt::DisplayRole:
      return static_cast<const Item*>(index.internalPointer())
          ->GetData(static_cast<Column>(index.column()));
    default:
      return {};
  }
}
