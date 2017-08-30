/*******************************************************************************
 *   Created: 2017/08/26 18:59:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "DocFeelsTrend.hpp"

namespace trdk {
namespace Tests {
namespace Mocks {

class DocFeelsTrend : public trdk::Strategies::DocFeels::Trend {
 public:
  virtual ~DocFeelsTrend() override = default;
};
}
}
}