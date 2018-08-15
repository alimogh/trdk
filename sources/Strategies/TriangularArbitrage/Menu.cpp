/*******************************************************************************
 *   Created: 2018/03/06 11:07:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "LegSetSelectionDialog.hpp"
#include "StrategyWindow.hpp"

using namespace trdk::FrontEnd;
using namespace trdk::Strategies::TriangularArbitrage;

StrategyMenuActionList CreateMenuActions(Engine &engine) {
#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
  return {1,
          {QObject::tr("&Triangular Arbitrage..."),
           [&engine](QWidget *parent) -> StrategyWidgetList {
             StrategyWidgetList result;
             const auto &conf =
                 LegSetSelectionDialog(engine, parent).RequestLegSet();
             if (conf) {
               result.emplace_back(
                   boost::make_unique<StrategyWindow>(engine, *conf, parent));
             }
             return result;
           }}};
}
