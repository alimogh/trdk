/**************************************************************************
 *   Created: 2012/07/09 18:12:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Algo.hpp"
#include "Security.hpp"
#include "PositionReporter.hpp"

Algo::Algo(boost::shared_ptr<DynamicSecurity> security)
		: m_security(security),
		m_positionReporter(nullptr) {
	//...//
}

Algo::~Algo() {
	delete m_positionReporter;
}

Security::Qty Algo::CalcQty(Security::Price price) const {
	return std::max<Security::Qty>(
		1,
		Security::Qty(10000.0 / GetSecurity()->Descale(price)));
}

boost::shared_ptr<const DynamicSecurity> Algo::GetSecurity() const {
	return const_cast<Algo *>(this)->GetSecurity();
}

boost::shared_ptr<DynamicSecurity> Algo::GetSecurity() {
	return m_security;
}

PositionReporter & Algo::GetPositionReporter() {
	if (!m_positionReporter) {
		m_positionReporter = CreatePositionReporter().release();
	}
	return *m_positionReporter;
}

boost::posix_time::ptime Algo::GetLastDataTime() {
	return boost::posix_time::not_a_date_time;
}
