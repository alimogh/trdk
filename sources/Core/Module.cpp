/**************************************************************************
 *   Created: 2012/09/23 14:31:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Module.hpp"

using namespace Trader;

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
