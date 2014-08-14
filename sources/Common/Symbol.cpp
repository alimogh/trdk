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
		: m_hash(0) {
	//...//
}

Symbol::Data::Data()
		: securityType(numberOfSecurityTypes),
		strike(.0),
		right(numberOfRights) {
	//...//
}

Symbol::Symbol(
			SecurityType securityType,
			const std::string &symbol,
			const std::string &currency)
		: m_data(securityType, symbol, currency),
		m_hash(0) {
	Assert(!m_data.symbol.empty());
	Assert(!m_data.currency.empty());
	if (m_data.symbol.empty()) {
		throw ParameterError("Symbol can't be empty");
	} else if (m_data.currency.empty()) {
		throw ParameterError("Symbol currency can't be empty");
	}
}

Symbol::Data::Data(
			SecurityType securityType,
			const std::string &symbol,
			const std::string &currency)
		: securityType(securityType),
		symbol(symbol),
		currency(currency),
		strike(.0),
		right(numberOfRights) {
	//...//
}

Symbol::Symbol(
			const std::string &symbol,
			const std::string &currency,
			const std::string &expirationDate,
			double strike)
		: m_data(symbol, currency, expirationDate, strike),
		m_hash(0) {
	Assert(!m_data.symbol.empty());
	Assert(!m_data.currency.empty());
	Assert(!m_data.expirationDate.empty());
	Assert(!IsZero(m_data.strike));
	if (m_data.symbol.empty()) {
		throw ParameterError("Symbol can't be empty");
	} else if (m_data.currency.empty()) {
		throw ParameterError("Symbol currency can't be empty");
	}
}

Symbol::Data::Data(
			const std::string &symbol,
			const std::string &currency,
			const std::string &expirationDate,
			double strike)
		: securityType(SECURITY_TYPE_FUTURE_OPTION),
		symbol(symbol),
		currency(currency),
		expirationDate(expirationDate),
		strike(strike),
		right(numberOfRights) {
	//...//
}

Symbol::Symbol(
			SecurityType securityType,
			const std::string &symbol,
			const std::string &exchange,
			const std::string &primaryExchange,
			const std::string &currency)
		: m_data(securityType, symbol, exchange, primaryExchange, currency),
		m_hash(0) {
	Assert(!m_data.symbol.empty());
	Assert(!m_data.exchange.empty());
	Assert(!m_data.primaryExchange.empty());
	Assert(!m_data.currency.empty());
	if (m_data.symbol.empty()) {
		throw ParameterError("Symbol can't be empty");
	} else if (m_data.exchange.empty()) {
		throw ParameterError("Symbol exchange can't be empty");
	} else if (m_data.primaryExchange.empty()) {
		throw ParameterError("Symbol primary exchange can't be empty");
	} else if (m_data.currency.empty()) {
		throw ParameterError("Symbol currency can't be empty");
	}
}

Symbol::Data::Data(
			SecurityType securityType,
			const std::string &symbol,
			const std::string &exchange,
			const std::string &primaryExchange,
			const std::string &currency)
		: securityType(securityType),
		symbol(symbol),
		exchange(exchange),
		primaryExchange(primaryExchange),
		currency(currency),
		strike(.0),
		right(numberOfRights) {
	//...//
}

Symbol::Symbol(const Symbol &rhs)
		: m_data(rhs.m_data),
		m_hash(rhs.m_hash.load()) {
	//...//
}

Symbol & Symbol::operator =(const Symbol &rhs) {
	if (this != &rhs) {
		m_data = rhs.m_data;
		m_hash = rhs.m_hash.load();
	}
	return *this;
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
	result.m_data.securityType = Symbol::SECURITY_TYPE_STOCK;
	primaryExchange.swap(result.m_data.primaryExchange);
	result.m_data.symbol = subs[0];
	if (result.m_data.symbol.empty()) {
		throw StringFormatError();
	}
	result.m_data.exchange = subs.size() >= 3 && !subs[2].empty()
		?	subs[2]
		:	defExchange;
	result.m_data.currency = subs.size() >= 4 && !subs[3].empty()
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
	result.m_data.securityType = securityType;
	result.m_data.symbol = symbolMatch[1].str();
	result.m_data.currency = symbolMatch[2].str();
	result.m_data.exchange = subs.size() >= 2 && !subs[1].empty()
		?	subs[1]
		:	defExchange;
	result.m_data.primaryExchange = "FOREX";
	return result;
}

Symbol Symbol::ParseCashFutureOption(
			const std::string &line,
			const std::string &expirationDate,
			double strike,
			const Right &right,
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
	result.m_data.securityType = SECURITY_TYPE_FUTURE_OPTION;
	result.m_data.symbol = symbolMatch[1].str();
	result.m_data.currency = symbolMatch[2].str();
	result.m_data.exchange = subs.size() >= 2 && !subs[1].empty()
		?	subs[1]
		:	defExchange;
	result.m_data.expirationDate = expirationDate;
	result.m_data.strike = strike;
	result.m_data.right = right;
	
	return result;

}

std::string Symbol::GetAsString() const {
	std::ostringstream os;
	os << *this;
	return os.str();
}

Symbol::operator bool() const {
	Assert(
		m_data.securityType == numberOfSecurityTypes
		|| (!m_data.symbol.empty() && !m_data.currency.empty()));
	Assert(
		m_data.securityType != numberOfSecurityTypes
		|| (m_data.symbol.empty()
			&& m_data.exchange.empty()
			&& m_data.primaryExchange.empty()
			&& m_data.currency.empty()
			&& m_hash == 0));
	return m_data.securityType != numberOfSecurityTypes;
}

bool Symbol::operator <(const Symbol &rhs) const {
	if (m_data.primaryExchange < rhs.m_data.primaryExchange) {
		return true;
	} else if (m_data.primaryExchange != rhs.m_data.primaryExchange) {
		return false;
	} else if (m_data.exchange < rhs.m_data.exchange) {
		return true;
	} else if (m_data.exchange != rhs.m_data.exchange) {
		return false;
	} else if (m_data.symbol < rhs.m_data.symbol) {
		return true;
	} else if (m_data.symbol != rhs.m_data.symbol) {
		return false;
	} else {
		return m_data.currency < rhs.m_data.currency;
	}
}

bool Symbol::operator ==(const Symbol &rhs) const {
	//! @todo see TRDK-120
	if (this == &rhs) {
		return true;
	} else if (m_hash && rhs.m_hash) {
		Assert(
			(m_hash == rhs.m_hash
				&& m_data.symbol == rhs.m_data.symbol
				&& m_data.exchange == rhs.m_data.exchange
				&& m_data.primaryExchange == rhs.m_data.primaryExchange
				&& m_data.currency == rhs.m_data.currency)
			|| (m_hash != rhs.m_hash
				&& (m_data.symbol != rhs.m_data.symbol
					|| m_data.exchange != rhs.m_data.exchange
					|| m_data.primaryExchange != rhs.m_data.primaryExchange
					|| m_data.currency != rhs.m_data.currency)));
		return m_hash == rhs.m_hash;
	} else {
		return
			m_data.symbol == rhs.m_data.symbol
			&& m_data.currency == rhs.m_data.currency
			&& m_data.exchange == rhs.m_data.exchange
			&& m_data.primaryExchange == rhs.m_data.primaryExchange;
	}
}

bool Symbol::operator !=(const Symbol &rhs) const {
	return operator ==(rhs);
}

Symbol::Hash Symbol::GetHash() const {
 	if (!m_hash) {
		std::ostringstream oss;
		oss << *this << ":" << m_data.currency;
		const_cast<Symbol *>(this)->m_hash = boost::hash_value(oss.str());
		AssertNe(0, m_hash);
	}
	return m_hash;
}

Symbol::SecurityType Symbol::GetSecurityType() const {
	return m_data.securityType;
}

const std::string & Symbol::GetSymbol() const {
	return m_data.symbol;
}
		
const std::string & Symbol::GetExchange() const {
	return m_data.exchange;
}
		
const std::string & Symbol::GetPrimaryExchange() const {
	return m_data.primaryExchange;
}

const std::string & Symbol::GetCurrency() const {
	return m_data.currency;
}

const std::string & Symbol::GetExpirationDate() const {
	return m_data.expirationDate;
}

double Symbol::GetStrike() const {
	return m_data.strike;
}

const Symbol::Right & Symbol::GetRight() const {
	if (m_data.right == numberOfRights) {
		throw Lib::LogicError("Symbol has not Right");
	}
	return m_data.right;
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
