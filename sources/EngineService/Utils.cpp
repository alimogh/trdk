/**************************************************************************
 *   Created: 2015/07/21 03:27:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Common/Assert.hpp"
#include "Common/DisableBoostWarningsBegin.h"
#	include <boost/cstdint.hpp>
#	include <boost/optional.hpp>
#	include <boost/date_time/posix_time/ptime.hpp> // for "Core/Types.hpp"
#	include <boost/uuid/uuid.hpp>
#include "Common/DisableBoostWarningsEnd.h"
#include "Utils.hpp"
#include "Common/Exception.hpp"
#include "Common/Assert.hpp"

using namespace trdk;

EngineService::TradingMode EngineService::Convert(
		const trdk::TradingMode &source) {
	static_assert(numberOfTradingModes == 2, "List changed.");
	switch (source) {
		default:
			AssertEq(trdk::TRADING_MODE_LIVE, source);
			throw trdk::Lib::Exception("Unknown Trading Mode");
		case trdk::TRADING_MODE_LIVE:
			return EngineService::TRADING_MODE_LIVE;
		case trdk::TRADING_MODE_PAPER:
			return EngineService::TRADING_MODE_PAPER;
	}
}

void EngineService::ConvertToUuid(const boost::uuids::uuid &source, Uuid &dest) {
	AssertEq(16, source.size());
	dest.set_data(&source, source.size());
}

boost::uuids::uuid EngineService::ConvertFromUuid(const Uuid &source) {
	const auto &data = source.data();
	if (data.size() != 16) {
		throw trdk::Lib::Exception("Wrong bytes number for Uuid");
	}
	boost::uuids::uuid result;
	AssertEq(16, result.size());
	memcpy(
		&result,
		reinterpret_cast<const boost::uuids::uuid::value_type *>(data.c_str()),
		result.size());
	return result;
}
