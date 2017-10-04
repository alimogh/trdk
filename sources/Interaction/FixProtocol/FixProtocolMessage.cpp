/*******************************************************************************
 *   Created: 2017/09/22 00:36:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "FixProtocolMessage.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::FixProtocol;

////////////////////////////////////////////////////////////////////////////////

void Message::WriteStandardTrailer(std::vector<char> &result,
                                   unsigned char soh) const {
  boost::format sub("10=%03d");
  sub % Detail::CalcCheckSum(result.cbegin(), result.cend());
  const auto &subStr = sub.str();
  std::copy(subStr.cbegin(), subStr.cend(), std::back_inserter(result));
  result.emplace_back(soh);
}

////////////////////////////////////////////////////////////////////////////////