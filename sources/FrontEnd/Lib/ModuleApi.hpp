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

typedef std::vector<std::unique_ptr<QWidget>> StrategyWidgetList;
typedef boost::function<StrategyWidgetList(QWidget *parent)>
    StrategyWindowFactory;
typedef std::vector<std::pair<QString, StrategyWindowFactory>>
    StrategyMenuActionList;

}  // namespace FrontEnd
}  // namespace trdk
