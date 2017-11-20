/*******************************************************************************
 *   Created: 2017/11/19 18:26:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "BittrexUtil.hpp"

using namespace trdk::Interaction;

std::string Rest::NormilizeBittrexSymbol(const std::string &source) {
  std::vector<std::string> subs;
  boost::split(subs, source, boost::is_any_of("_"));
  if (subs.size() != 2) {
    return source;
  }
  subs[0].swap(subs[1]);
  return boost::join(subs, "-");
}
