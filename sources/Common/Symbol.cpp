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
#include <xhash>

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

Symbol::Symbol(
			const std::string &symbol,
			const std::string &exchange,
			const std::string &primaryExchange)
		: m_symbol(symbol),
		m_exchange(exchange),
		m_primaryExchange(primaryExchange),
		m_hash(0) {
	Assert(!m_symbol.empty());
	Assert(!m_exchange.empty());
	Assert(!m_primaryExchange.empty());
	if (m_symbol.empty()) {
		throw ParameterError("Symbol can't be empty");
	} else if (m_exchange.empty()) {
		throw ParameterError("Symbol exchange can't be empty");
	} else if (m_primaryExchange.empty()) {
		throw ParameterError("Symbol primary exchange can't be empty");
	}
}

Symbol Symbol::Parse(
			const std::string &line,
			const std::string &defExchange,
			const std::string &defPrimaryExchange) {
	Assert(!defExchange.empty());
	Assert(!defPrimaryExchange.empty());
	if (defExchange.empty()) {
		throw ParameterError("Default symbol exchange can't be empty");
	} else if (defPrimaryExchange.empty()) {
		throw ParameterError(
			"Default symbol primary exchange can't be empty");
	}
	std::vector<std::string> subs;
	boost::split(subs, line, boost::is_any_of(":"));
	foreach (auto &s, subs) {
		boost::trim(s);
	}
	Symbol result;
	result.m_symbol = subs[0];
	if (result.m_symbol.empty()) {
		throw StringFormatError();
	}
	result.m_exchange = subs.size() == 3 && !subs[2].empty()
		?	subs[2]
		:	defExchange;
	result.m_primaryExchange = subs.size() >= 2 && !subs[1].empty()
		?	subs[1]
		:	defPrimaryExchange;
	return result;
}

std::string Symbol::GetAsString() const {
	std::ostringstream os;
	os << *this;
	return os.str();
}

bool Symbol::operator <(const Symbol &rhs) const {
	return
		m_primaryExchange < rhs.m_primaryExchange
			|| (m_primaryExchange == rhs.m_primaryExchange
				&& m_exchange < rhs.m_exchange)
			|| (m_primaryExchange == rhs.m_primaryExchange
				&& m_exchange == rhs.m_exchange
				&& m_symbol < rhs.m_symbol);
}

bool Symbol::operator ==(const Symbol &rhs) const {
	if (this == &rhs) {
		return true;
	} else if (m_hash && rhs.m_hash) {
		Assert(
			(m_hash == rhs.m_hash
				&& m_symbol == m_symbol
				&& m_exchange == m_exchange
				&& m_primaryExchange == m_primaryExchange)
			|| (m_hash != rhs.m_hash
				&& (m_symbol != m_symbol
					|| m_exchange != m_exchange
					|| m_primaryExchange != m_primaryExchange)));
		return m_hash == rhs.m_hash;
	} else {
		return
			m_symbol == rhs.m_symbol
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
		oss << *this;
		Interlocking::Exchange(
			const_cast<Symbol *>(this)->m_hash,
			stdext::hash_value(oss.str()));
		AssertNe(0, m_hash);
	}
	return m_hash;
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

////////////////////////////////////////////////////////////////////////////////

std::ostream & std::operator <<(std::ostream &os, const Symbol &symbol) {
	os
		<< symbol.GetSymbol()
		<< ':' << symbol.GetPrimaryExchange()
		<< ':' << symbol.GetExchange();
	return os;
}

////////////////////////////////////////////////////////////////////////////////
