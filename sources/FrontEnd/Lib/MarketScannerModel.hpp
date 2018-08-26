//
//    Created: 2018/08/24 16:36
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

namespace trdk {
namespace FrontEnd {

class TRDK_FRONTEND_LIB_API MarketScannerModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  typedef QAbstractItemModel Base;

  explicit MarketScannerModel(Engine &, QWidget *parent);
  MarketScannerModel(MarketScannerModel &&) = delete;
  MarketScannerModel(const MarketScannerModel &) = delete;
  MarketScannerModel &operator=(MarketScannerModel &&) = delete;
  MarketScannerModel &operator=(const MarketScannerModel &) = delete;
  ~MarketScannerModel() override;

  QVariant headerData(int section, Qt::Orientation, int role) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QModelIndex index(int row,
                    int column,
                    const QModelIndex &parent) const override;
  QModelIndex parent(const QModelIndex &index) const override;
  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;

 public slots:
  void Refresh();

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace FrontEnd
}  // namespace trdk