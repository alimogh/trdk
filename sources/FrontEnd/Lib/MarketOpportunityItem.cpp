﻿//
//    Created: 2018/08/26 10:22 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "MarketOpportunityItem.hpp"

using namespace trdk;
using namespace Lib;
using namespace FrontEnd;

class MarketOpportunityItem::Implementation {
 public:
  MarketOpportunityItem &m_self;
  QString m_symbols;
  const Strategy &m_strategy;
  Strategy::ProfitScannerSlotConnection m_strategySignalConnection;

  explicit Implementation(MarketOpportunityItem &self,
                          QString symbols,
                          const Strategy &strategy)
      : m_self(self),
        m_symbols(std::move(symbols)),
        m_strategy(strategy),
        m_strategySignalConnection(m_strategy.SubscribeToProfitScanner(
            [this](const Double &, bool) { emit m_self.ProfitUpdated(); })) {}
};

MarketOpportunityItem::MarketOpportunityItem(QString symbols,
                                             const Strategy &strategy)
    : m_pimpl(boost::make_unique<Implementation>(
          *this, std::move(symbols), strategy)) {}
MarketOpportunityItem::~MarketOpportunityItem() = default;

QVariant MarketOpportunityItem::GetProfit() const {
  const auto &opportunity = m_pimpl->m_strategy.GetProfitOpportunity();
  if (!opportunity) {
    return {};
  }
  return opportunity->first.Get() * 100;
}
bool MarketOpportunityItem::IsAvailable() const {
  const auto &opportunity = m_pimpl->m_strategy.GetProfitOpportunity();
  if (!opportunity) {
    return false;
  }
  return opportunity->second;
}
const Strategy &MarketOpportunityItem::GetStrategy() const {
  return m_pimpl->m_strategy;
}
const QString &MarketOpportunityItem::GetSymbolsTitle() const {
  return m_pimpl->m_symbols;
}
