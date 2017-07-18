/**************************************************************************
 *   Created: 2012/12/31 15:49:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Instrument.hpp"

using namespace trdk;
using namespace trdk::Lib;

//////////////////////////////////////////////////////////////////////////

class Instrument::Implementation : private boost::noncopyable {
 public:
  Context &m_context;
  const Symbol m_symbol;

 public:
  explicit Implementation(Context &context, const Symbol &symbol)
      : m_context(context), m_symbol(symbol) {}
};

//////////////////////////////////////////////////////////////////////////

Instrument::Instrument(Context &context, const Symbol &symbol)
    : m_pimpl(new Implementation(context, symbol)) {}

Instrument::~Instrument() { delete m_pimpl; }

const Symbol &Instrument::GetSymbol() const noexcept {
  return m_pimpl->m_symbol;
}

const Context &Instrument::GetContext() const {
  return const_cast<Instrument *>(this)->GetContext();
}

Context &Instrument::GetContext() { return m_pimpl->m_context; }

std::ostream &trdk::operator<<(std::ostream &oss,
                               const Instrument &instrument) {
  oss << instrument.GetSymbol();
  return oss;
}
