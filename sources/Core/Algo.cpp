/**************************************************************************
 *   Created: 2012/07/09 18:12:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Algo.hpp"

Algo::Algo(boost::shared_ptr<const DynamicSecurity> security)
		: m_security(security) {
	//...//
}

Algo::~Algo() {
	//...//
}
