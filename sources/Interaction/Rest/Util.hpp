/*******************************************************************************
 *   Created: 2017/10/30 22:56:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Interaction {
namespace Rest {

inline void MakeServerAnswerDebugDump(const boost::property_tree::ptree &answer,
                                      TradingSystem &ts) {
#ifdef DEV_VER
  std::stringstream ss;
  boost::property_tree::json_parser::write_json(ss, answer);
  ts.GetLog().Debug(ss.str().c_str());
#else
  Lib::UseUnused(answer, ts);
#endif
}
}
}
}
