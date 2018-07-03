/*******************************************************************************
 *   Created: 2018/01/28 05:06:18
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace FrontEnd {
namespace Detail {

////////////////////////////////////////////////////////////////////////////////

struct BalanceRecord {
  const QString symbol;
  double available;
  double locked;
  QTime time;
  const bool isUsed;

  explicit BalanceRecord(QString symbol,
                         Volume available,
                         Volume locked,
                         bool isUsed);
  void Update(Volume newAvailable, Volume newLocked);
  Volume GetTotal() const;
};

////////////////////////////////////////////////////////////////////////////////

enum BalanceColumn {
  BALANCE_COLUMN_EXCHANGE_OR_SYMBOL,
  BALANCE_COLUMN_AVAILABLE,
  BALANCE_COLUMN_LOCKED,
  BALANCE_COLUMN_TOTAL,
  BALANCE_COLUMN_UPDATE_TIME,
  BALANCE_COLUMN_USED,
  numberOfBalanceColumns
};

////////////////////////////////////////////////////////////////////////////////

inline Qt::AlignmentFlag GetBalanceFieldAligment(const BalanceColumn &column) {
  static_assert(numberOfBalanceColumns == 6, "List changed.");
  switch (column) {
    case BALANCE_COLUMN_AVAILABLE:
    case BALANCE_COLUMN_LOCKED:
    case BALANCE_COLUMN_TOTAL:
      return Qt::AlignRight;
    default:
      return Qt::AlignLeft;
  }
}

////////////////////////////////////////////////////////////////////////////////

class BalanceItem {
 public:
  BalanceItem();
  BalanceItem(BalanceItem &&) = delete;
  BalanceItem(const BalanceItem &) = default;
  BalanceItem &operator=(BalanceItem &&) = default;
  BalanceItem &operator=(const BalanceItem &) = default;
  virtual ~BalanceItem() = default;

  void AppendChild(boost::shared_ptr<BalanceItem> &&);
  int GetRow() const;
  int GetNumberOfChilds() const;
  BalanceItem *GetChild(int row);
  BalanceItem *GetParent();
  virtual QVariant GetData(int column) const = 0;
  virtual bool HasEmpty() const;
  virtual bool HasLocked() const;
  virtual bool IsUsed() const;

 private:
  BalanceItem *m_parent;
  int m_row;
  std::vector<boost::shared_ptr<BalanceItem>> m_childItems;
};

////////////////////////////////////////////////////////////////////////////////

class BalanceTradingSystemItem : public BalanceItem {
 public:
  explicit BalanceTradingSystemItem(const TradingSystem &);
  ~BalanceTradingSystemItem() override = default;

  QVariant GetData(int column) const override;

 private:
  QVariant m_data;
};

////////////////////////////////////////////////////////////////////////////////

class BalanceDataItem : public BalanceItem {
 public:
  typedef BalanceItem Base;

  explicit BalanceDataItem(const boost::shared_ptr<BalanceRecord> &);
  ~BalanceDataItem() override = default;

  BalanceRecord &GetRecord();
  QVariant GetData(int column) const override;
  bool HasEmpty() const override;
  bool HasLocked() const override;
  bool IsUsed() const override;

 private:
  const boost::shared_ptr<BalanceRecord> m_data;
};

////////////////////////////////////////////////////////////////////////////////
}  // namespace Detail
}  // namespace FrontEnd
}  // namespace trdk