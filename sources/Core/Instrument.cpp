/**************************************************************************
 *   Created: 2012/12/31 15:49:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Instrument.hpp"

using namespace Trader;
using namespace Trader::Lib;

//////////////////////////////////////////////////////////////////////////

class Instrument::Implementation : private boost::noncopyable {

public:

	const boost::shared_ptr<TradeSystem> m_tradeSystem;
	const std::string m_symbol;
	const std::string m_primaryExchange;
	std::string m_exchange;
	const std::string m_fullSymbol;
	const boost::shared_ptr<const Settings> m_settings;

public:

	explicit Implementation(
				boost::shared_ptr<TradeSystem> tradeSystem,
				const std::string &symbol,
				const std::string &primaryExchange,
				const std::string &exchange,
				boost::shared_ptr<const Settings> settings)
			: m_tradeSystem(tradeSystem),
			m_symbol(symbol),
			m_primaryExchange(primaryExchange),
			m_exchange(exchange),
			m_fullSymbol(CreateSymbolFullStr(m_symbol, m_primaryExchange, m_exchange)),
			m_settings(settings) {
		//...//
	}

	explicit Implementation(
				const std::string &symbol,
				const std::string &primaryExchange,
				const std::string &exchange,
				boost::shared_ptr<const Settings> settings)
			: m_symbol(symbol),
			m_primaryExchange(primaryExchange),
			m_exchange(exchange),
			m_fullSymbol(CreateSymbolFullStr(m_symbol, m_primaryExchange, m_exchange)),
			m_settings(settings) {
		//...//
	}


};

//////////////////////////////////////////////////////////////////////////

Instrument::Instrument(
			boost::shared_ptr<TradeSystem> tradeSystem,
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange,
			boost::shared_ptr<const Settings> settings)
		: m_pimpl(
			new Implementation(
				tradeSystem,
				symbol,
				primaryExchange,
				exchange,
				settings)) {
	//...//
}

Instrument::Instrument(
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange,
			boost::shared_ptr<const Settings> settings)
		: m_pimpl(
			new Implementation(
				symbol,
				primaryExchange,
				exchange,
				settings)) {
	//...//
}

Instrument::~Instrument() {
	delete m_pimpl;
}

const std::string & Instrument::GetFullSymbol() const throw() {
	return m_pimpl->m_fullSymbol;
}

const std::string & Instrument::GetSymbol() const throw() {
	return m_pimpl->m_symbol;
}

const std::string & Instrument::GetPrimaryExchange() const {
	return m_pimpl->m_primaryExchange;
}

const std::string & Instrument::GetExchange() const {
	return m_pimpl->m_exchange;
}

const TradeSystem & Instrument::GetTradeSystem() const {
	return const_cast<Instrument *>(this)->GetTradeSystem();
}

TradeSystem & Instrument::GetTradeSystem() {
	if (!m_pimpl->m_tradeSystem) {
		throw Exception("Instrument doesn't connected to trade system");
	}
	return *m_pimpl->m_tradeSystem;
}

const Settings & Instrument::GetSettings() const {
	return *m_pimpl->m_settings;
}

//////////////////////////////////////////////////////////////////////////
