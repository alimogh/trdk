/*******************************************************************************
 *   Created: 2017/10/22 21:55:27
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

struct AdviceSide {
  trdk::Price price;
  trdk::Price qty;
};
struct AdviceSecuritySignal {
  trdk::Security *security;
  bool isBestBid;
  bool isBestAsk;
};

struct Advice {
  trdk::Security *security;

  boost::posix_time::ptime time;
  AdviceSide bid;
  AdviceSide ask;

  trdk::Price bestSpreadValue;
  trdk::Lib::Double bestSpreadRatio;

  bool isSignaled;
  std::vector<AdviceSecuritySignal> securitySignals;
};
}
}
}