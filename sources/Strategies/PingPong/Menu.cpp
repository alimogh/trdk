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

using namespace trdk;
using namespace Strategies::PingPong;
using namespace FrontEnd;
namespace pp = Strategies::PingPong;
namespace ptr = boost::property_tree;

StrategyMenuActionList CreateMenuActions(Engine &engine) {
  return {1,
          {QObject::tr("&Ping Pong..."),
           [&engine](QWidget *parent) -> StrategyWidgetList {
             StrategyWidgetList result;
             auto transaction = engine.GetContext().StartAdding();
             for (const auto &symbol :
                  SymbolSelectionDialog(engine, parent).RequestSymbols()) {
               result.emplace_back(boost::make_unique<StrategyWindow>(
                   engine, symbol, *transaction, parent));
             }
             transaction->Commit();
             return result;
           }}};
}

StrategyWidgetList RestoreStrategyWidgets(
    Engine &engine,
    const QUuid &typeId,
    const QUuid &instanceId,
    const QString &name,
    const ptr::ptree &config,
    Context::AddingTransaction &transaction,
    QWidget *parent) {
  StrategyWidgetList result;
  if (ConvertToBoostUuid(typeId) == pp::Strategy::typeId) {
    result.emplace_back(boost::make_unique<StrategyWindow>(
        engine, instanceId, name, config, transaction, parent));
  }
  return result;
}
