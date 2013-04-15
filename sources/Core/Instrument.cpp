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
	const std::string m_symbol;
	const std::string m_primaryExchange;
	std::string m_exchange;
	const std::string m_fullSymbol;

public:

	explicit Implementation(
				Context &context,
				const std::string &symbol,
				const std::string &primaryExchange,
				const std::string &exchange)
			: m_context(context),
			m_symbol(symbol),
			m_primaryExchange(primaryExchange),
			m_exchange(exchange),
			m_fullSymbol(CreateSymbolFullStr(m_symbol, m_primaryExchange, m_exchange)) {
		//...//
	}

};

//////////////////////////////////////////////////////////////////////////

Instrument::Instrument(
			Context &context,
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange)
		: m_pimpl(
			new Implementation(
				context,
				symbol,
				primaryExchange,
				exchange)) {
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

const Context & Instrument::GetContext() const {
	return const_cast<Instrument *>(this)->GetContext();
}

Context & Instrument::GetContext() {
	return m_pimpl->m_context;
}

//////////////////////////////////////////////////////////////////////////
