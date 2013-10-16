/**************************************************************************
 *   Created: 2012/07/15 13:22:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "MarketDataSource.hpp"
#include "Context.hpp"
#include "Security.hpp"

using namespace trdk;
using namespace trdk::Lib;

////////////////////////////////////////////////////////////////////////////////

class MarketDataSource::Implementation : private boost::noncopyable {

public:

	typedef boost::unordered_map<
			trdk::Lib::Symbol,
			boost::shared_ptr<Security>>
		Securities;

	Securities m_securities;

};

//////////////////////////////////////////////////////////////////////////

MarketDataSource::Error::Error(const char *what) throw()
		: Base::Error(what) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

MarketDataSource::MarketDataSource()
		: m_pimpl(new Implementation) {
	//...//
}

MarketDataSource::~MarketDataSource() {
	delete m_pimpl;
}

Security & MarketDataSource::GetSecurity(
			Context &context,
			const Symbol &symbol) {
	{
		auto *existSecurity = FindSecurity(symbol);
		if (existSecurity) {
			return *existSecurity;
		}
	}
	boost::shared_ptr<trdk::Security> newSecurity
		= CreateSecurity(context, symbol);
	m_pimpl->m_securities[symbol] = newSecurity;
	context.GetLog().Debug("Loaded security \"%1%\".", *newSecurity);
	return *newSecurity;
}

Security * MarketDataSource::FindSecurity(const Symbol &symbol) {
	const auto &result = m_pimpl->m_securities.find(symbol);
	return result != m_pimpl->m_securities.end()
		?	&*result->second
		:	nullptr;
}

const Security * MarketDataSource::FindSecurity(const Symbol &symbol) const {
	return const_cast<MarketDataSource *>(this)->FindSecurity(symbol);
}

size_t MarketDataSource::GetActiveSecurityCount() const {
	return m_pimpl->m_securities.size();
}

//////////////////////////////////////////////////////////////////////////
