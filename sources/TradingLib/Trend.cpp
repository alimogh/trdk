/*******************************************************************************
 *   Created: 2017/08/28 10:48:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Trend.hpp"

using namespace trdk;
using namespace trdk::TradingLib;

class Trend::Implementation : public boost::noncopyable {
 public:
  boost::optional<bool> m_isRising;
};

Trend::Trend() : m_pimpl(std::make_unique<Implementation>()) {}

Trend::~Trend() = default;

bool Trend::IsExistent() const { return m_pimpl->m_isRising ? true : false; }

bool Trend::IsRising() const {
  return m_pimpl->m_isRising && *m_pimpl->m_isRising;
}

bool Trend::IsFalling() const {
  return m_pimpl->m_isRising && !*m_pimpl->m_isRising;
}

bool Trend::SetIsRising(bool isRising) {
  if (!m_pimpl->m_isRising || *m_pimpl->m_isRising != isRising) {
    m_pimpl->m_isRising = isRising;
    return true;
  } else {
    return false;
  }
}

bool Trend::Reset() {
  if (!m_pimpl->m_isRising) {
    return false;
  }
  m_pimpl->m_isRising = boost::none;
  return true;
}
