/*******************************************************************************
 *   Created: 2018/01/16 15:06:11
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Strategies {
namespace ArbitrageAdvisor {

Position *FindOppositePosition(Position &);
const Position *FindOppositePosition(const Position &);
}
}
}
