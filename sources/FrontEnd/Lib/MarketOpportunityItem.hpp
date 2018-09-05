//
//    Created: 2018/08/26 2:58 PM
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

class MarketOpportunityItem : public QObject {
  Q_OBJECT

 public:
  MarketOpportunityItem(QString symbols, const Strategy &);
  MarketOpportunityItem(MarketOpportunityItem &&) = delete;
  MarketOpportunityItem(const MarketOpportunityItem &) = delete;
  MarketOpportunityItem &operator=(MarketOpportunityItem &&) = delete;
  MarketOpportunityItem &operator=(const MarketOpportunityItem &) = delete;
  virtual ~MarketOpportunityItem();

  QVariant GetProfit() const;
  const Strategy &GetStrategy() const;
  const QString &GetSymbolsTitle() const;

  virtual QString GetSymbolsConfig() const = 0;
  virtual QString GetTitle() const = 0;

 signals:
  void ProfitUpdated();

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

}  // namespace FrontEnd
}  // namespace trdk