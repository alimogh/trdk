/**************************************************************************
 *   Created: 2012/07/09 18:12:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Strategy.hpp"
#include "PositionReporter.hpp"

using namespace Trader;
using namespace Trader::Lib;

Strategy::Strategy(const std::string &tag, boost::shared_ptr<Security> security)
		: SecurityAlgo(tag, security),
		m_positionReporter(nullptr) {
	//...//
}

Strategy::~Strategy() {
	delete m_positionReporter;
}

PositionReporter & Strategy::GetPositionReporter() {
	if (!m_positionReporter) {
		m_positionReporter = CreatePositionReporter().release();
	}
	return *m_positionReporter;
}
