/**************************************************************************
 *   Created: 2013/05/17 23:50:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Symbol.hpp"
#include "Foreach.hpp"
#include "Interlocking.hpp"
#include "Util.hpp"

using namespace trdk::Lib;

////////////////////////////////////////////////////////////////////////////////

Symbol::Error::Error(const char *what) throw()
		: Exception(what) {
	//...//
}

Symbol::StringFormatError::StringFormatError() throw()
		: Error("Wrong symbol string format") {
	//...//
}

Symbol::ParameterError::ParameterError(const char *what) throw()
		: Error(what) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

Symbol::Symbol()
		: m_securityType(numberOfSecurityTypes),
		m_strike(.0),
		m_hash(0),
		m_right(numberOfRights) {
	//...//
}

Symbol::Symbol(
			SecurityType securityType,
			const std::string &symbol,
			const std::string &currency)
		: m_securityType(securityType),
		m_symbol(symbol),
		m_currency(currency),
		m_strike(.0),
		m_hash(0),
		m_right(numberOfRights) {
	Assert(!m_symbol.empty());
	Assert(!m_currency.empty());
	if (m_symbol.empty()) {
		throw ParameterError("Symbol can't be empty");
	} else if (m_currency.empty()) {
		throw ParameterError("Symbol currency can't be empty");
	}
}

Symbol::Symbol(
			const std::string &symbol,
			const std::string &currency,
			const std::string &expirationDate,
			double strike)
		: m_securityType(SECURITY_TYPE_FUTURE_OPTION),
		m_symbol(symbol),
		m_currency(currency),
		m_expirationDate(expirationDate),
		m_strike(strike),
		m_hash(0),
		m_right(numberOfRights) {
	Assert(!m_symbol.empty());
	Assert(!m_currency.empty());
	Assert(!expirationDate.empty());
	Assert(!IsZero(strike));
	if (m_symbol.empty()) {
		throw ParameterError("Symbol can't be empty");
	} else if (m_currency.empty()) {
		throw ParameterError("Symbol currency can't be empty");
	}
}

Symbol::Symbol(
			SecurityType securityType,
			const std::string &symbol,
			const std::string &exchange,
			const std::string &primaryExchange,
			const std::string &currency)
		: m_securityType(securityType),
		m_symbol(symbol),
		m_exchange(exchange),
		m_primaryExchange(primaryExchange),
		m_currency(currency),
		m_strike(.0),
		m_hash(0),
		m_right(numberOfRights) {
	Assert(!m_symbol.empty());
	Assert(!m_exchange.empty());
	Assert(!m_primaryExchange.empty());
	Assert(!m_currency.empty());
	if (m_symbol.empty()) {
		throw ParameterError("Symbol can't be empty");
	} else if (m_exchange.empty()) {
		throw ParameterError("Symbol exchange can't be empty");
	} else if (m_primaryExchange.empty()) {
		throw ParameterError("Symbol primary exchange can't be empty");
	} else if (m_currency.empty()) {
		throw ParameterError("Symbol currency can't be empty");
	}
}

Symbol Symbol::Parse(
			const std::string &line,
			const std::string &defExchange,
			const std::string &defPrimaryExchange,
			const std::string &defCurrency) {
	
	Assert(!defExchange.empty());
	Assert(!defPrimaryExchange.empty());
	if (defExchange.empty()) {
		throw ParameterError("Default symbol exchange can't be empty");
	} else if (defPrimaryExchange.empty()) {
		throw ParameterError(
			"Default symbol primary exchange can't be empty");
	} else if (defCurrency.empty()) {
		throw ParameterError(
			"Default symbol currency can't be empty");
	}

	std::vector<std::string> subs;
	boost::split(subs, line, boost::is_any_of(":"));
	foreach (auto &s, subs) {
		boost::trim(s);
	}

	std::string primaryExchange = subs.size() >= 2 && !subs[1].empty()
		?	subs[1]
		:	defPrimaryExchange;
	if (	boost::iequals(primaryExchange, "FOREX")
			&& boost::iequals(defPrimaryExchange, "FOREX")) {
		return ParseCash(Symbol::SECURITY_TYPE_CASH, line, defExchange);
	}
	
	Symbol result;
	result.m_securityType = Symbol::SECURITY_TYPE_STOCK;
	primaryExchange.swap(result.m_primaryExchange);
	result.m_symbol = subs[0];
	if (result.m_symbol.empty()) {
		throw StringFormatError();
	}
	result.m_exchange = subs.size() >= 3 && !subs[2].empty()
		?	subs[2]
		:	defExchange;
	result.m_currency = subs.size() >= 4 && !subs[3].empty()
		?	subs[3]
		:	defCurrency;
	return result;

}

Symbol Symbol::ParseCash(
			SecurityType securityType,
			const std::string &line,
			const std::string &defExchange) {
	Assert(!defExchange.empty());
	if (defExchange.empty()) {
		throw ParameterError("Default symbol exchange can't be empty");
	}
	std::vector<std::string> subs;
	boost::split(subs, line, boost::is_any_of(":"));
	foreach (auto &s, subs) {
		boost::trim(s);
	}
	if (subs[0].empty() || subs.size() > 2) {
		throw StringFormatError();
	}
	boost::smatch symbolMatch;
	if (	!boost::regex_match(
				subs[0],
				symbolMatch,
				boost::regex("^([a-zA-Z]{3,3})[^a-zA-Z]*([a-zA-Z]{3,3})$"))) {
		throw StringFormatError();
	}
	Symbol result;
	result.m_securityType = securityType;
	result.m_symbol = symbolMatch[1].str();
	result.m_currency = symbolMatch[2].str();
	result.m_exchange = subs.size() >= 2 && !subs[1].empty()
		?	subs[1]
		:	defExchange;
	result.m_primaryExchange = "FOREX";
	return result;
}

Symbol Symbol::ParseCashFutureOption(const std::string &line) {

	// Special code only for custom branch (for ex. with currency extract
	// or exchange code)!
 
	std::vector<std::string> subs;
	boost::split(subs, line, boost::is_any_of(":"));
	foreach (auto &s, subs) {
		boost::trim(s);
		if (s.empty()) {
			throw StringFormatError();
		}
	}
	if (subs.size() != 7) {
		throw StringFormatError();
	}

	Symbol result;
	result.m_securityType = SECURITY_TYPE_FUTURE_OPTION;
	result.m_symbol = subs[0];
	result.m_currency = subs[1];
	result.m_exchange = subs[2];
	result.m_expirationDate = subs[3];
	try {
		result.m_strike
			= boost::lexical_cast<decltype(result.m_strike)>(subs[4]);
	} catch (const boost::bad_lexical_cast &) {
		throw ParameterError("Failed to parse FOP Symbol Strike");
	}
	result.m_right = ParseRight(subs[5]);
	
	const auto &fopStrPos = subs[6].find(" (FOP)");
	if (fopStrPos != std::string::npos) {
		result.m_tradingClass = subs[6].substr(0, fopStrPos);
	} else {
		result.m_tradingClass = subs[6];
	}
	
	return result;

}

Symbol Symbol::ParseCashFutureOption(
			const std::string &line,
			const std::string &expirationDate,
			double strike,
			const Right &right,
			const std::string &tradingClass,
			const std::string &defExchange) {

	Assert(!defExchange.empty());
	if (defExchange.empty()) {
		throw ParameterError("Default symbol exchange can't be empty");
	}

	std::vector<std::string> subs;
	boost::split(subs, line, boost::is_any_of(":"));
	foreach (auto &s, subs) {
		boost::trim(s);
	}
	if (subs[0].empty() || subs.size() > 2) {
		throw StringFormatError();
	}
	boost::smatch symbolMatch;
	if (	!boost::regex_match(
				subs[0],
				symbolMatch,
				boost::regex("^([a-zA-Z]{3,3})[^a-zA-Z]*([a-zA-Z]{3,3})$"))) {
		throw StringFormatError();
	}

	Symbol result;
	result.m_securityType = SECURITY_TYPE_FUTURE_OPTION;
	result.m_symbol = symbolMatch[1].str();
	result.m_currency = symbolMatch[2].str();
	result.m_exchange = subs.size() >= 2 && !subs[1].empty()
		?	subs[1]
		:	defExchange;
	result.m_expirationDate = expirationDate;
	result.m_strike = strike;
	result.m_right = right;
	result.m_tradingClass = tradingClass;
		
	return result;

}

std::string Symbol::GetAsString() const {
	std::ostringstream os;
	os << *this;
	return os.str();
}

Symbol::operator bool() const {
	Assert(
		m_securityType == numberOfSecurityTypes
		|| (!m_symbol.empty() && !m_currency.empty()));
	Assert(
		m_securityType != numberOfSecurityTypes
		|| (m_symbol.empty()
			&& m_exchange.empty()
			&& m_primaryExchange.empty()
			&& m_currency.empty()
			&& m_hash == 0));
	return m_securityType != numberOfSecurityTypes;
}

bool Symbol::operator <(const Symbol &rhs) const {
	if (m_primaryExchange < rhs.m_primaryExchange) {
		return true;
	} else if (m_primaryExchange != rhs.m_primaryExchange) {
		return false;
	} else if (m_exchange < rhs.m_exchange) {
		return true;
	} else if (m_exchange != rhs.m_exchange) {
		return false;
	} else if (m_symbol < rhs.m_symbol) {
		return true;
	} else if (m_symbol != rhs.m_symbol) {
		return false;
	} else {
		return m_currency < rhs.m_currency;
	}
}

bool Symbol::operator ==(const Symbol &rhs) const {
	//! @todo see TRDK-120
	if (this == &rhs) {
		return true;
	} else if (m_hash && rhs.m_hash) {
		Assert(
			(m_hash == rhs.m_hash
				&& m_symbol == rhs.m_symbol
				&& m_exchange == rhs.m_exchange
				&& m_primaryExchange == rhs.m_primaryExchange
				&& m_currency == rhs.m_currency)
			|| (m_hash != rhs.m_hash
				&& (m_symbol != rhs.m_symbol
					|| m_exchange != rhs.m_exchange
					|| m_primaryExchange != rhs.m_primaryExchange
					|| m_currency != rhs.m_currency)));
		return m_hash == rhs.m_hash;
	} else {
		return
			m_symbol == rhs.m_symbol
			&& m_currency == rhs.m_currency
			&& m_exchange == rhs.m_exchange
			&& m_primaryExchange == rhs.m_primaryExchange;
	}
}

bool Symbol::operator !=(const Symbol &rhs) const {
	return operator ==(rhs);
}

Symbol::Hash Symbol::GetHash() const {
	if (!m_hash) {
		std::ostringstream oss;
		oss << *this << ":" << m_currency;
		Interlocking::Exchange(
			const_cast<Symbol *>(this)->m_hash,
			stdext::hash_value(oss.str()));
		AssertNe(0, m_hash);
	}
	return m_hash;
}

Symbol::SecurityType Symbol::GetSecurityType() const {
	return m_securityType;
}

const std::string & Symbol::GetSymbol() const {
	return m_symbol;
}
		
const std::string & Symbol::GetExchange() const {
	return m_exchange;
}
		
const std::string & Symbol::GetPrimaryExchange() const {
	return m_primaryExchange;
}

const std::string & Symbol::GetCurrency() const {
	return m_currency;
}

const std::string & Symbol::GetExpirationDate() const {
	return m_expirationDate;
}

double Symbol::GetStrike() const {
	return m_strike;
}

const Symbol::Right & Symbol::GetRight() const {
	if (m_right == numberOfRights) {
		throw Lib::LogicError("Symbol has not Right");
	}
	return m_right;
}

Symbol::Right Symbol::ParseRight(const std::string &source) {
	//! @sa trdk::Lib::Symbol::GetRightAsString
	static_assert(numberOfRights == 2, "Right list changed.");
	if (boost::iequals(source, "put")) {
		return RIGHT_PUT;
	} else if (boost::iequals(source, "call")) {
		return RIGHT_CALL;
	} else {
		throw ParameterError("Failed to resolve FOP Right (Put or Call)");
	}
}

std::string Symbol::GetRightAsString() const {
	//! @sa trdk::Lib::Symbol::ParseRight
	static_assert(numberOfRights == 2, "Right list changed.");
	switch (GetRight()) {
		default:
			AssertEq(RIGHT_CALL, GetRight());
			return std::string();
		case RIGHT_CALL:
			return "Call";
		case RIGHT_PUT:
			return "Put";
	}
}

const std::string & Symbol::GetTradingClass() const {
	if (m_tradingClass.empty()) {
		throw Lib::LogicError("Symbol has not Trading Class");
	}
	return m_tradingClass;
}

////////////////////////////////////////////////////////////////////////////////

std::ostream & std::operator <<(std::ostream &os, const Symbol &symbol) {
	// If changing here - look at Symbol::GetHash, how hash creating.
	static_assert(Symbol::numberOfSecurityTypes == 3, "Security list changed.");
	switch (symbol.GetSecurityType()) {
		case Symbol::SECURITY_TYPE_STOCK:
			os
				<< symbol.GetSymbol()
				<< ":" << symbol.GetCurrency()
				<< ":" << symbol.GetPrimaryExchange()
				<< ':' << symbol.GetExchange();
			break;
		case Symbol::SECURITY_TYPE_FUTURE_OPTION:
			os
				<< symbol.GetSymbol()
				<< ":" << symbol.GetCurrency()
				<< ':' << symbol.GetExchange()
				<< ':' << symbol.GetExpirationDate()
				<< ':' << symbol.GetStrike()
				<< ':' << symbol.GetRightAsString()
				<< ':' << symbol.GetTradingClass()
				<< " (FOP)";
			break;
		case Symbol::SECURITY_TYPE_CASH:
			os
				<< symbol.GetSymbol() << symbol.GetCurrency()
				<< ":" << symbol.GetPrimaryExchange()
				<< ':' << symbol.GetExchange()
				<< " (CASH)";
			break;
		default:
			AssertEq(Symbol::SECURITY_TYPE_STOCK, symbol.GetSecurityType());
			break;
	}
	return os;
}

////////////////////////////////////////////////////////////////////////////////
