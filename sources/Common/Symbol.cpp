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

Symbol::StringFormatError::StringFormatError(const char *what) throw()
	: Error(what) {
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
	: securityType(numberOfSecurityTypes)
	, strike(.0)
	, right(numberOfRights)
	, fotBaseCurrency(numberOfCurrencies)
	, currency(numberOfCurrencies) {
	//...//
}

Symbol::Symbol(
		const std::string &line,
		const SecurityType &defSecurityType,
		const Currency &defCurrency)
	: m_hash(0) {

	/*
		 AAPL/201305/USD:NASDAQ/SMART:FUT
		 AAPL/201305/USD:NASDAQ:FUT
		 AAPL/201305/USD::FUT
		 AAPL/USD::STK
		 AAPL/USD
		 AAPL
	*/

	if (line.empty()) {
		throw StringFormatError("Symbol string can't be empty");
	}

	std::vector<std::string> subs;
	boost::split(subs, line, boost::is_any_of(":"));
	foreach (auto &s, subs) {
		boost::trim(s);
	}
	if (subs.empty() || subs.size() > 3 || line.empty()) {
		throw StringFormatError("Format is invalid");
	}
	
	//////////////////////////////////////////////////////////////////////////
	// Security type - always, last by ":". If type provided - exchanges should
	// be set or exchange filed should be empty
	// (ex.: XXX::FUT - two ":" is empty exchange).

	AssertEq(numberOfSecurityTypes, m_data.securityType);
	if (subs.size() > 2) {
		if (subs[2].empty()) {
			throw StringFormatError("Security type field is empty");
		}
		try {
			m_data.securityType = ConvertSecurityTypeFromString(subs[2]);
		} catch (const trdk::Lib::Exception &ex) {
			throw StringFormatError(ex.what());
		}
	} else if (defSecurityType == numberOfSecurityTypes) {
		throw StringFormatError("Security type is not set");
	} else {
		m_data.securityType = defSecurityType;
	}

	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Symbol parsing (separated by "/").
	
	std::vector<std::string> symbolSubs;
	boost::split(symbolSubs, subs[0], boost::is_any_of("/"));
	foreach (auto &s, symbolSubs) {
		boost::trim(s);
		if (s.empty()) {
			throw StringFormatError("One or more symbol fields are empty");
		}
	}
	
	symbolSubs.front().swap(m_data.symbol);
	
	static_assert(numberOfSecurityTypes == 5, "List changed.");
	size_t currencyIndex = 0;
	switch (m_data.securityType) {
		case SECURITY_TYPE_STOCK:
			if (symbolSubs.size() > 2) {
				throw StringFormatError("Too many fields for stock symbol");
			}
			currencyIndex = 1;
			break;
		case SECURITY_TYPE_FUTURES:
			if (symbolSubs.size() > 3) {
				throw StringFormatError("Too many fields for futures symbol");
			} else if (symbolSubs.size() < 2 || symbolSubs[1].empty()) {
				throw StringFormatError("Expiration date is not set");
			}
			symbolSubs[1].swap(m_data.expirationDate);
			currencyIndex = 2;
			break;
		case SECURITY_TYPE_FUTURES_OPTIONS:
			throw Exception("Futures options symbols are not supported");
		case SECURITY_TYPE_FOR:
			if (symbolSubs.size() > 2) {
				throw StringFormatError(
					"Too many fields for foreign currency exchange symbol");
			}
			currencyIndex = 1;
			break;
		case SECURITY_TYPE_FOR_FUTURES_OPTIONS:
			throw Exception(
				"Foreign currency exchange futures options symbols"
					" are not supported");
		default:
			AssertEq(SECURITY_TYPE_STOCK, m_data.securityType);
			throw Exception("System error: Unknown security ID");
	}
	
	AssertLt(0, currencyIndex);
	if (symbolSubs.size() > currencyIndex) {
		// Currency is always last:
		AssertEq(currencyIndex + 1, symbolSubs.size()); 
		try {
			m_data.currency
				= ConvertCurrencyFromIso(symbolSubs[currencyIndex]);
		} catch (const trdk::Lib::Exception &ex) {
			throw StringFormatError(ex.what());
		}
	} else if (defCurrency == numberOfCurrencies) {
		throw StringFormatError("Currency is not set");
	} else {
		m_data.currency = defCurrency;
	}

	//////////////////////////////////////////////////////////////////////////
	// Exchange.

	if (subs.size() > 1 && !subs[1].empty()) {
		
		std::vector<std::string> exchangeSubs;
		boost::split(exchangeSubs, subs[1], boost::is_any_of("/"));
		if (!exchangeSubs.empty()) {
		
			foreach (auto &s, exchangeSubs) {
				boost::trim(s);
				if (s.empty()) {
					throw StringFormatError(
						"One or more exchange fields are empty");
				}
			}
		
			static_assert(numberOfSecurityTypes == 5, "List changed.");
			switch (m_data.securityType) {
				case SECURITY_TYPE_STOCK:
					if (exchangeSubs.size() > 2) {
						throw StringFormatError(
							"Too many fields for stock exchange");
					} else if (exchangeSubs.size() > 1) {
						exchangeSubs[0].swap(m_data.primaryExchange);
						exchangeSubs[1].swap(m_data.exchange);
					} else {
						exchangeSubs[0].swap(m_data.exchange);
					}
					break;
				case SECURITY_TYPE_FUTURES:
					if (exchangeSubs.size() > 1) {
						throw StringFormatError(
							"Too many fields for futures exchange");
					}
					exchangeSubs[0].swap(m_data.exchange);
					break;
				case SECURITY_TYPE_FUTURES_OPTIONS:
					throw Exception(
						"Futures options exchanges are not supported");
				case SECURITY_TYPE_FOR:
					if (exchangeSubs.size() > 1) {
						throw StringFormatError(
							"Too many fields for"
								" foreign currency exchange exchange");
					}
					exchangeSubs[0].swap(m_data.exchange);
					break;
				case SECURITY_TYPE_FOR_FUTURES_OPTIONS:
					throw Exception(
						"Foreign currency exchange futures options exchanges"
							" are not supported");
				default:
					AssertEq(SECURITY_TYPE_STOCK, m_data.securityType);
					throw Exception("System error: Unknown security ID");

			}
		}

	}

	//////////////////////////////////////////////////////////////////////////


}

Symbol::Symbol(const Symbol &rhs)
	: m_data(rhs.m_data)
	, m_hash(rhs.m_hash.load()) {
	//...//
}

Symbol & Symbol::operator =(const Symbol &rhs) {
	if (this != &rhs) {
		m_data = rhs.m_data;
		m_hash = rhs.m_hash.load();
	}
	return *this;
}

Symbol::operator bool() const {
	return m_data.securityType != numberOfSecurityTypes;
}


std::string Symbol::GetAsString() const {
	std::ostringstream os;
	os << *this;
	return os.str();
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
	} else if (m_data.currency < rhs.m_data.currency) {
		return true;
	} else if (m_data.currency != rhs.m_data.currency) {
		return false;
	} else if (m_data.symbol < rhs.m_data.symbol) {
		return true;
	} else if (m_data.symbol != rhs.m_data.symbol) {
		return false;
	} else if (m_data.securityType < rhs.m_data.securityType) {
		return true;
	} else if (m_data.securityType != rhs.m_data.securityType) {
		return false;
	} else {
		return m_data.expirationDate < rhs.m_data.expirationDate;
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
				&& m_data.primaryExchange == rhs.m_data.primaryExchange)
			|| (m_hash != rhs.m_hash
				&& (m_data.symbol != rhs.m_data.symbol
					|| m_data.exchange != rhs.m_data.exchange
					|| m_data.primaryExchange != rhs.m_data.primaryExchange)));
		return m_hash == rhs.m_hash;
	} else {
		return
			m_data.securityType == rhs.m_data.securityType
			&& m_data.currency == rhs.m_data.currency
			&& m_data.symbol == rhs.m_data.symbol
			&& m_data.expirationDate == rhs.m_data.expirationDate
			&& m_data.exchange == rhs.m_data.exchange
			&& m_data.primaryExchange == rhs.m_data.primaryExchange;
	}
}

bool Symbol::operator !=(const Symbol &rhs) const {
	return !operator ==(rhs);
}

Symbol::Hash Symbol::GetHash() const {
 	if (!m_hash) {
		std::ostringstream oss;
		oss << *this;
		const_cast<Symbol *>(this)->m_hash = boost::hash_value(oss.str());
		AssertNe(0, m_hash);
	}
	return m_hash;
}

const SecurityType & Symbol::GetSecurityType() const {
	AssertNe(numberOfSecurityTypes, m_data.securityType);
	return m_data.securityType;
}

const std::string & Symbol::GetSymbol() const {
	Assert(!m_data.symbol.empty());
	return m_data.symbol;
}

const std::string & Symbol::GetExchange() const {
	if (m_data.exchange.empty()) {
		throw Lib::LogicError("Symbol doesn't have Exchange");
	}
	return m_data.exchange;
}

const std::string & Symbol::GetPrimaryExchange() const {
	if (m_data.primaryExchange.empty()) {
		throw Lib::LogicError("Symbol doesn't have Primary Exchange");
	}
	return m_data.primaryExchange;
}

const std::string & Symbol::GetExpirationDate() const {
	if (m_data.expirationDate.empty()) {
		throw Lib::LogicError("Symbol doesn't have Expiration Date");
	}
	return m_data.expirationDate;
}

double Symbol::GetStrike() const {
	return m_data.strike;
}

const Symbol::Right & Symbol::GetRight() const {
	if (m_data.right == numberOfRights) {
		throw Lib::LogicError("Symbol doesn't have Right");
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

const Currency & Symbol::GetCurrency() const {
	AssertNe(numberOfCurrencies, m_data.currency);
	return m_data.currency;
}

const Currency & Symbol::GetFotBaseCurrency() const {
	if (m_data.fotBaseCurrency == numberOfCurrencies) {
		throw Lib::LogicError("Symbol doesn't have Base Currency");
	}
	return m_data.fotBaseCurrency;
}

const Currency & Symbol::GetFotQuoteCurrency() const {
	if (m_data.currency == numberOfCurrencies) {
		throw Lib::LogicError("Symbol doesn't have Quote Currency");
	}
	return m_data.currency;
}

////////////////////////////////////////////////////////////////////////////////

std::ostream & trdk::Lib::operator <<(std::ostream &os, const Symbol &symbol) {
	// If changing here - look at Symbol::GetHash, how hash creating.
	static_assert(numberOfSecurityTypes == 5, "List changed.");
	switch (symbol.GetSecurityType()) {
		case SECURITY_TYPE_STOCK:
			os << symbol.GetSymbol() << '/' << symbol.GetCurrency();
			if (!symbol.m_data.primaryExchange.empty()) {
				os << ':' << symbol.m_data.primaryExchange;
			}
			if (!symbol.m_data.exchange.empty()) {
				os << ':' << symbol.m_data.exchange;
			}
			break;
		case SECURITY_TYPE_FOR:
			os << symbol.GetSymbol() << '/' << symbol.GetCurrency();
			if (!symbol.m_data.exchange.empty()) {
				os << ':' << symbol.m_data.exchange;
			}
			break;
		case SECURITY_TYPE_FUTURES:
			os
				<< symbol.GetSymbol()
				<< '/' << symbol.GetExpirationDate()
				<< '/' << symbol.GetCurrency()
				<< ':' << symbol.GetExchange();
			break;
		case SECURITY_TYPE_FOR_FUTURES_OPTIONS:
		default:
			AssertEq(SECURITY_TYPE_STOCK, symbol.GetSecurityType());
			os << "<UNKNOWN>";
			return os;
	}
	os << "(" << symbol.GetSecurityType() << ')';
	return os;
}

////////////////////////////////////////////////////////////////////////////////
