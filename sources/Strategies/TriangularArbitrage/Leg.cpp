/*******************************************************************************
 *   Created: 2018/03/10 03:41:14
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Leg.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::TriangularArbitrage;

LegPolicy::LegPolicy(const std::string &symbol) : m_symbol(symbol) {}
const std::string &LegPolicy::GetSymbol() const { return m_symbol; }

boost::unordered_set<Security *> &LegPolicy::GetSecurities() {
  return m_securities;
}

const boost::unordered_set<Security *> &LegPolicy::GetSecurities() const {
  return const_cast<LegPolicy *>(this)->GetSecurities();
}

void LegPolicy::AddSecurities(Security &security) {
  if (!m_securities.emplace(&security).second) {
    throw Exception("Leg security isn't unique");
  }
}
