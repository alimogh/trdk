/*******************************************************************************
 *   Created: 2017/09/29 11:26:45
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
namespace Shell {

class SecurityListModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  typedef QAbstractItemModel Base;

 private:
  enum Column {
    COLUMN_SYMBOL,
    COLUMN_SOURCE,
    COLUMN_BID_PRICE,
    COLUMN_ASK_PRICE,
    COLUMN_TYPE,
    COLUMN_CURRENCY,
    COLUMN_SYMBOL_FULL,
    COLUMN_TYPE_FULL,
    numberOfColumns
  };

 public:
  explicit SecurityListModel(Engine &, QWidget *parent);
  virtual ~SecurityListModel() override = default;

 public:
  virtual QVariant headerData(int section,
                              Qt::Orientation,
                              int role) const override;
  virtual QVariant data(const QModelIndex &index, int role) const override;
  virtual QModelIndex index(int row,
                            int column,
                            const QModelIndex &parent) const override;
  virtual QModelIndex parent(const QModelIndex &index) const override;
  virtual int rowCount(const QModelIndex &parent) const override;
  virtual int columnCount(const QModelIndex &parent) const override;

 protected:
  const Security &GetSecurity(const QModelIndex &) const;
  int GetSecurityIndex(const Security &) const;

 private slots:
  void StateChanged(bool isStarted);
  void BidPriceUpdate(const Security &, const Price &);
  void AskPriceUpdate(const Security &, const Price &);

 private:
  void Load();
  void Clear();

  void UpdateValue(const Security &, const Column &);

  QString ConvertFullToQString(const Lib::SecurityType &) const;

 private:
  Engine &m_engine;
  std::vector<const Security *> m_securities;
};
}
}
}