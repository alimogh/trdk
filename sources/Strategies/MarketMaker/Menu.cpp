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
#include "SymbolSelectionDialog.hpp"
#include "TrendRepeatingStrategyWindow.hpp"

using namespace trdk::Strategies::MarketMaker;
using namespace trdk::FrontEnd::Lib;

StrategyMenuActionList CreateMenuActions(Engine &engine) {
  return {
      1,
      {QObject::tr("Market making by trend..."),
       [&engine](QWidget *parent) -> StrategyWidgetList {
         StrategyWidgetList result;
         const auto &symbol =
             SymbolSelectionDialog(engine, parent).RequestSymbol();
         if (symbol) {
           result.emplace_back(boost::make_unique<TrendRepeatingStrategyWindow>(
               engine, *symbol, parent));
         }
         return result;
       }}};
}
