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

#include "ShellApi.h"
#include "ShellFwd.hpp"
#include "Lib/Fwd.hpp"

namespace trdk {
namespace FrontEnd {
namespace Shell {

class TRDK_FRONTEND_SHELL_LIB_API SecurityListModel
    : public QAbstractItemModel {
  Q_OBJECT

 public:
  typedef QAbstractItemModel Base;

 private:
  enum Column {
    COLUMN_SYMBOL,
    COLUMN_SOURCE,
    COLUMN_BID_PRICE,
    COLUMN_ASK_PRICE,
    COLUMN_LAST_TIME,
    COLUMN_TYPE,
    COLUMN_CURRENCY,
    COLUMN_SYMBOL_FULL,
    COLUMN_TYPE_FULL,
    numberOfColumns
  };

 public:
  explicit SecurityListModel(Lib::Engine &, QWidget *parent);
  virtual ~SecurityListModel() override = default;

 public:
  const Security &GetSecurity(const QModelIndex &) const;
  Security &GetSecurity(const QModelIndex &);
  int GetSecurityIndex(const Security &) const;

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

 private slots:
  void OnStateChanged(bool isStarted);
  void UpdatePrice(const Security *);

 private:
  void Load();
  void Clear();

  QString ConvertFullToQString(const trdk::Lib::SecurityType &) const;

 private:
  Lib::Engine &m_engine;
  std::vector<Security *> m_securities;
};
}
}
}