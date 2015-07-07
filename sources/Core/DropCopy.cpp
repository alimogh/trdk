/**************************************************************************
 *   Created: 2015/07/12 19:06:40
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "DropCopy.hpp"
#include "Context.hpp"
#include "EventsLog.hpp"

using namespace trdk;

//////////////////////////////////////////////////////////////////////////

class DropCopy::Implementation : private boost::noncopyable {

public:

	Context &m_context;
	ModuleEventsLog m_log;

public:

	Implementation(Context &context, const std::string &tag)
		: m_context(context),
		m_log(tag, m_context.GetLog()) {
		//...//
	}

};

//////////////////////////////////////////////////////////////////////////

DropCopy::DropCopy(Context &context, const std::string &tag)
	: m_pimpl(new Implementation(context, tag)) {
	//...//
}

DropCopy::~DropCopy() {
	delete m_pimpl;
}

Context & DropCopy::GetContext() {
	return m_pimpl->m_context;
}

ModuleEventsLog & DropCopy::GetLog() const throw() {
	return m_pimpl->m_log;
}

//////////////////////////////////////////////////////////////////////////
