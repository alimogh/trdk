/*******************************************************************************
 *   Created: 2018/01/31 23:44:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "TakerStrategyWindow.hpp"

using namespace trdk::FrontEnd;
using namespace trdk::Strategies::MarketMaker;

StrategyMenuActionList CreateMenuActions(Engine &engine) {
  return {1,
          {QObject::tr("&Market Maker \"Taker\"..."),
           [&engine](QWidget *parent) -> StrategyWidgetList {
             StrategyWidgetList result;
             for (const auto &symbol :
                  SymbolSelectionDialog(engine, parent).RequestSymbols()) {
               result.emplace_back(boost::make_unique<TakerStrategyWindow>(
                   engine, symbol, parent));
             }
             return result;
           }}};
}
