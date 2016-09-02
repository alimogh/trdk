/**************************************************************************
 *   Created: 2013/09/08 03:52:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Types.hpp"

using namespace trdk;
using namespace trdk::Lib;

////////////////////////////////////////////////////////////////////////////////

namespace {

	template<typename Param>
	void DumpOrderParam(
				const char *paramName,
				const Param &paramVal,
				size_t &count,
				std::ostream &os) {
		if (!paramVal) {
			return;
		}
		if (count > 0) {
			os << ", ";
		}
		os << paramName << "=\"" << *paramVal << '"';
		++count;
	}

}

std::ostream & trdk::operator <<(std::ostream &os, const OrderParams &params) {
	size_t count = 0;
	DumpOrderParam("displaySize", params.displaySize, count, os);
	DumpOrderParam("goodTillTime", params.goodTillTime, count, os);
	DumpOrderParam("goodInSeconds", params.goodInSeconds, count, os);
	return os;
}

////////////////////////////////////////////////////////////////////////////////

const char * trdk::ConvertToPch(const Level1TickType &tickType) {
	static_assert(numberOfLevel1TickTypes == 7, "List changed.");
	switch (tickType) {
		default:
			AssertEq(LEVEL1_TICK_LAST_PRICE, tickType);
			return "UNKNOWN";
		case LEVEL1_TICK_LAST_PRICE:
			return "last price";
		case LEVEL1_TICK_LAST_QTY:
			return "last qty";
		case LEVEL1_TICK_BID_PRICE:
			return "bid price";
		case LEVEL1_TICK_BID_QTY:
			return "bid qty";
		case LEVEL1_TICK_ASK_PRICE:
			return "ask price";
		case LEVEL1_TICK_ASK_QTY:
			return "ask qty";
		case LEVEL1_TICK_TRADING_VOLUME:
			return "trading volume";
	}
}

////////////////////////////////////////////////////////////////////////////////

TradingMode trdk::ConvertTradingModeFromString(const std::string &mode) {
	
	static_assert(numberOfTradingModes == 3, "List changed.");

	if (boost::iequals(mode, ConvertToString(TRADING_MODE_PAPER))) {
		return TRADING_MODE_PAPER;
	} else if (boost::iequals(mode, ConvertToString(TRADING_MODE_LIVE))) {
		return TRADING_MODE_LIVE;
	} else {
		boost::format message(
			"Wrong trading mode \"%1%\", allowed: \"%2%\" and \"%3%\".");
		message
			% mode
			% ConvertToString(TRADING_MODE_PAPER)
			% ConvertToString(TRADING_MODE_LIVE);
		throw trdk::Lib::Exception(message.str().c_str());
	}

}

namespace {

	const char * ConvertToPch(const TradingMode &mode) {
		static_assert(numberOfTradingModes == 3, "List changed.");
		switch (mode) {
			case TRADING_MODE_LIVE:
				return "live";
			case TRADING_MODE_PAPER:
				return "paper";
			case TRADING_MODE_BACKTESTING:
				return "backtesting";
			default:
				AssertEq(TRADING_MODE_LIVE, mode);
				return "UNKNOWN";
		}
	}

	const std::string tradingModeLive = ConvertToPch(TRADING_MODE_LIVE);
	const std::string tradingModePaper = ConvertToPch(TRADING_MODE_PAPER);
	const std::string tradingModeBacktesting
		= ConvertToPch(TRADING_MODE_BACKTESTING);
	const std::string tradingModeUnknown = "UNKNOWN";

}

const std::string & trdk::ConvertToString(const TradingMode &mode) {
	static_assert(numberOfTradingModes, "List changed.");
	switch (mode) {
		case TRADING_MODE_LIVE:
			return tradingModeLive;
		case TRADING_MODE_PAPER:
			return tradingModePaper;
		case TRADING_MODE_BACKTESTING:
			return tradingModeBacktesting;
		default:
			AssertEq(TRADING_MODE_LIVE, mode);
			return tradingModeUnknown;
	}
}

//////////////////////////////////////////////////////////////////////////
