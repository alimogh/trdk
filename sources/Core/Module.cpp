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
		"Module \"%1%\" (tag \"%2%\") has been configured to work with"
			" service \"%3%\" (tag \"%4%\"), but can't work with it.",
		GetName(),
		GetTag(),
		service.GetName(),
		service.GetTag());
	throw Exception(
		"Module has been configured to work with service,"
			" but can't work with it");
}
