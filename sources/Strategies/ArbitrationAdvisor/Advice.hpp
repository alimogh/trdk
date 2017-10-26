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

struct Advice {
  struct Side {
    trdk::Price price;
    trdk::Price qty;
  };
  struct SecuritySignal {
    trdk::Security *security;
    bool isBidSignaled;
    bool isAskSignaled;
  };

  trdk::Security *security;

  boost::posix_time::ptime time;
  Side bid;
  Side ask;

  trdk::Price bestSpreadValue;
  trdk::Lib::Double bestSpreadRatio;

  bool isSignaled;
  std::vector<SecuritySignal> securitySignals;
};
}
}
}