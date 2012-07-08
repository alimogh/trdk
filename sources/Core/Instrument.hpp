/**************************************************************************
 *   Created: May 19, 2012 1:07:25 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#pragma once

class Instrument : private boost::noncopyable {

public:

	typedef boost::posix_time::ptime MarketDataTime;

	typedef double Price;
	typedef boost::int64_t ScaledPrice;

public:

	explicit Instrument(
				const std::string &symbol,
				const std::string &primaryExchange,
				const std::string &exchange)
			: m_symbol(symbol),
			m_primaryExchange(primaryExchange),
			m_exchange(exchange),
			m_fullSymbol((boost::format("%1%:%2%:%3%") % m_symbol % m_primaryExchange % m_exchange).str()) {
		//...//
	}

	virtual ~Instrument() {
		//...//
	}

public:

	const std::string & GetFullSymbol() const throw() {
		return m_fullSymbol;
	}
	const std::string & GetSymbol() const throw() {
		return m_symbol;
	}
	const std::string & GetPrimaryExchange() const {
		return m_primaryExchange;
	}
	const std::string & GetExchange() const {
		return m_exchange;
	}

	const char * GetCurrency() const {
		return "USD";
	}

	unsigned int GetScale() const throw() {
		return 10000;
	}
	
	ScaledPrice Scale(Price price) const {
		return Util::Scale(price, GetScale());
	}
	Price Descale(ScaledPrice price) const {
		return Util::Descale(price, GetScale());
	}

private:

	const std::string m_symbol;
	const std::string m_primaryExchange;
	const std::string m_exchange;
	const std::string m_fullSymbol;

};
