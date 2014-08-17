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
	const Currency m_currency;

public:

	explicit Implementation(
				Context &context,
				const Symbol &symbol,
				const Currency &currency)
			: m_context(context),
			m_symbol(symbol),
			m_currency(currency) {
		//...//
	}

};

//////////////////////////////////////////////////////////////////////////

Instrument::Instrument(
			Context &context,
			const Symbol &symbol,
			const Currency &currency)
		: m_pimpl(new Implementation(context, symbol, currency)) {
	//...//
}

Instrument::~Instrument() {
	delete m_pimpl;
}

const Symbol & Instrument::GetSymbol() const throw() {
	return m_pimpl->m_symbol;
}

Instrument::Currency Instrument::GetCurrency() const {
	return m_pimpl->m_currency;
}

namespace {

	namespace Currencies { namespace Iso4217 {
		const std::string usd = "USD";
		const std::string eur = "EUR";
	} }

};

const std::string & Instrument::GetCurrencyIso() const {
	using namespace Currencies::Iso4217;
	static_assert(numberOfCurrencies == 2, "Currency list changed.");
	switch (GetCurrency()) {
		case USD:
			return usd;
		case EUR:
			return eur;
		default:
			AssertEq(USD, GetCurrency());
			throw std::exception("Internal error: Unknown currency ID");
	}
}

const Context & Instrument::GetContext() const {
	return const_cast<Instrument *>(this)->GetContext();
}

Context & Instrument::GetContext() {
	return m_pimpl->m_context;
}

//////////////////////////////////////////////////////////////////////////

std::ostream & std::operator <<(
			std::ostream &oss,
			const Instrument &instrument) {
	oss << instrument.GetSymbol();
	return oss;
}

//////////////////////////////////////////////////////////////////////////
