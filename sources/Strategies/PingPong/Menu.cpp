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
#include "Strategy.hpp"
#include "StrategyWindow.hpp"

using namespace trdk::Strategies::PingPong;
using namespace trdk::FrontEnd;

namespace pp = trdk::Strategies::PingPong;

StrategyMenuActionList CreateMenuActions(Engine &engine) {
  return {1,
          {QObject::tr("&Ping Pong..."),
           [&engine](QWidget *parent) -> StrategyWidgetList {
             StrategyWidgetList result;
             for (const auto &symbol :
                  SymbolSelectionDialog(engine, parent).RequestSymbols()) {
               result.emplace_back(
                   boost::make_unique<StrategyWindow>(engine, symbol, parent));
             }
             return result;
           }}};
}

StrategyWidgetList RestoreStrategyWidgets(Engine &engine,
                                          const QUuid &typeId,
                                          const QUuid &instanceId,
                                          const QString &name,
                                          const QString &config,
                                          QWidget *parent) {
  StrategyWidgetList result;
  if (ConvertToBoostUuid(typeId) == pp::Strategy::typeId) {
    result.emplace_back(boost::make_unique<StrategyWindow>(
        engine, instanceId, name, config, parent));
  }
  return result;
}
