/**************************************************************************
 *   Created: 2016/08/26 23:43:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Security.hpp"
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::DdfPlus;

namespace pt = boost::posix_time;

namespace {

	DdfPlus::Security::SupportedLevel1Types GetSupportedLevel1Types() {
		DdfPlus::Security::SupportedLevel1Types result;
		result.set(LEVEL1_TICK_LAST_PRICE);
		result.set(LEVEL1_TICK_LAST_QTY);
		result.set(LEVEL1_TICK_BID_PRICE);
		result.set(LEVEL1_TICK_BID_QTY);
		result.set(LEVEL1_TICK_ASK_PRICE);
		result.set(LEVEL1_TICK_ASK_QTY);
		return result;
	}

}

DdfPlus::Security::Security(
		Context &context,
		const Symbol &symbol,
		const trdk::MarketDataSource &source)
	: trdk::Security(
		context,
		symbol,
		source,
		true,
		GetSupportedLevel1Types()) {
	//...//
}

std::string DdfPlus::Security::GenerateDdfPlusCode() const {

	std::ostringstream result;

	switch (GetSymbol().GetSecurityType()) {
		
		case SECURITY_TYPE_FUTURES:
		
			if (GetSymbol().IsExplicit()) {
				AssertFail(
					"Explicit symbol for futures contracts is not supported.");
				throw Exception(
					"Explicit symbol for futures contracts is not supported");
			} else if (
					GetExpiration().year > 2019
					|| GetExpiration().year < 2010) {
				throw MethodDoesNotImplementedError(
					"Work with features from < 2010 or > 2019"
						" is not supported");
			}
		
			result
				<< GetSymbol().GetSymbol()
				<< char(GetExpiration().code)
				<< (GetExpiration().year - 2010);
		
			break;
		
		default:
		
			AssertFail("Security type is not supported.");
			throw Exception("Security type is not supported");
	
	}

	return result.str();

}

void DdfPlus::Security::AddLevel1Tick(const Level1TickValue &&tick) {
	m_ticksBuffer.emplace_back(std::move(tick));
}

void DdfPlus::Security::Flush(
		const pt::ptime &time,
		const TimeMeasurement::Milestones &timeMeasurement) {
	Base::AddLevel1Tick(time, m_ticksBuffer, timeMeasurement);
}
