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
#include "Position.hpp"
#include "Service.hpp"

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

void Strategy::NotifyServiceStart(const Service &service) {
	Log::Error(
		"Strategy has been configured to work with service"
			" \"%1%\" (tag \"%2%\"), but can't work with it.",
		service.GetName(),
		service.GetTag());
	throw Trader::Lib::Exception(
		"Strategy has been configured to work with service,"
			" but can't work with it");
}

PositionReporter & Strategy::GetPositionReporter() {
	if (!m_positionReporter) {
		m_positionReporter = CreatePositionReporter().release();
	}
	return *m_positionReporter;
}
