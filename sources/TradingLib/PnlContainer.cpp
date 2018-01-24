/*******************************************************************************
 *   Created: 2018/01/23 11:34:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "PnlContainer.hpp"

using namespace trdk;
using namespace trdk::TradingLib;

////////////////////////////////////////////////////////////////////////////////

class PnlOneSymbolContainer::Implementation : private boost::noncopyable {
 public:
  Data m_data;
};

PnlOneSymbolContainer::PnlOneSymbolContainer()
    : m_pimpl(boost::make_unique<Implementation>()) {}
PnlOneSymbolContainer::~PnlOneSymbolContainer() = default;

void PnlOneSymbolContainer::Update(const Security &, const Volume &) {}

bool PnlOneSymbolContainer::IsProfit() const { return false; }

const PnlOneSymbolContainer::Data &PnlOneSymbolContainer::GetData() const {
  return m_pimpl->m_data;
}

////////////////////////////////////////////////////////////////////////////////
