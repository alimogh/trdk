/*******************************************************************************
 *   Created: 2017/10/16 02:35:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace FrontEnd {
namespace Lib {

typedef std::vector<std::unique_ptr<QWidget>> StrategyWidgetList;
typedef boost::function<trdk::FrontEnd::Lib::StrategyWidgetList(
    QWidget *parent)>
    StrategyWindowFactory;
typedef std::vector<
    std::pair<QString, trdk::FrontEnd::Lib::StrategyWindowFactory>>
    StrategyMenuActionList;
}  // namespace Lib
}  // namespace FrontEnd
}  // namespace trdk
