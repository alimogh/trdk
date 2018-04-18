//
//    Created: 2018/04/07 3:47 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

#include "Api.h"
#include "Fwd.hpp"

namespace trdk {
namespace FrontEnd {

class TRDK_FRONTEND_LIB_API TotalResultsReportModel
    : public QAbstractItemModel {
  Q_OBJECT

 public:
  typedef QAbstractItemModel Base;

  explicit TotalResultsReportModel(Engine&, QObject* parent);
  ~TotalResultsReportModel() override;

  void Build(const QDateTime& start,
             const boost::optional<QDateTime>& end,
             const boost::optional<QString>& strategy);

  QVariant headerData(int section, Qt::Orientation, int role) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QModelIndex index(int row,
                    int column,
                    const QModelIndex& parent) const override;
  QModelIndex parent(const QModelIndex& index) const override;
  int rowCount(const QModelIndex& parent) const override;
  int columnCount(const QModelIndex& parent) const override;

 private slots:
  void UpdateOperation(const Orm::Operation&);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace FrontEnd
}  // namespace trdk