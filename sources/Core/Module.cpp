/**************************************************************************
 *   Created: 2012/09/23 14:31:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Module.hpp"
#include "Service.hpp"

using namespace Trader;
using namespace Trader::Lib;

Module::Module(const std::string &tag)
		: m_tag(tag) {
	//...//
}

Module::~Module() {
	//...//
}

const std::string & Module::GetTag() const throw() {
	return m_tag;
}

void Module::NotifyServiceStart(const Service &service) {
	Log::Error(
		"\"%1%\" subscribed to \"%2%\", but can't work with it"
			" (hasn't implementation of NotifyServiceStart).",
		*this,
		service);
 	throw MethodDoesNotImplementedError(
		"Module subscribed to service, but can't work with it");
}

void Module::OnNewTrade(
					const Trader::Security &,
					const boost::posix_time::ptime &,
					Trader::Security::ScaledPrice,
					Trader::Security::Qty,
					bool) {
	Log::Error(
		"\"%1%\" subscribed to new trades, but can't work with it"
			" (hasn't implementation of OnNewTrade).",
		*this);
	throw MethodDoesNotImplementedError(
		"Module subscribed to new trades, but can't work with it");
}

void Module::OnServiceDataUpdate(const Trader::Service &service) {
	Log::Error(
		"\"%1%\" subscribed to \"%2%\", but can't work with it"
			" (hasn't implementation of OnServiceDataUpdate).",
		*this,
		service);
 	throw MethodDoesNotImplementedError(
 		"Module subscribed to service, but can't work with it");
}

//////////////////////////////////////////////////////////////////////////

std::ostream & std::operator <<(std::ostream &oss, const Module &module) {
	oss
		<< module.GetTypeName()
		<< '.' << module.GetName()
		<< '.' << module.GetTag();
	return oss;
}

//////////////////////////////////////////////////////////////////////////
