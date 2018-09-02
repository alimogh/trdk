//
//    Created: 2018/08/24 16:33
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

class TRDK_FRONTEND_LIB_API MarketScannerView : public QTableView {
  Q_OBJECT

 public:
  typedef QTableView Base;

  explicit MarketScannerView(QWidget *parent);
  MarketScannerView(MarketScannerView &&) = delete;
  MarketScannerView(const MarketScannerView &) = delete;
  MarketScannerView &operator=(MarketScannerView &&) = delete;
  MarketScannerView &operator=(const MarketScannerView &) = delete;
  ~MarketScannerView() override;

 signals:
  void StrategyRequested(const QString &title,
                         const QString &module,
                         const QString &factoryName,
                         const QString &params);

 protected:
  void rowsInserted(const QModelIndex &, int start, int end) override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace FrontEnd
}  // namespace trdk