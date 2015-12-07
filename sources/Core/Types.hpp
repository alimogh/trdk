/**************************************************************************
 *   Created: 2012/11/18 11:09:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Api.h"

////////////////////////////////////////////////////////////////////////////////

namespace trdk {

	typedef trdk::Lib::Numeric<double> Qty;
	
	typedef boost::int32_t ScaledPrice;

	typedef boost::uint64_t OrderId;

	enum OrderSide {
		ORDER_SIDE_BUY,
		ORDER_SIDE_BID = ORDER_SIDE_BUY,
		ORDER_SIDE_LONG = ORDER_SIDE_BUY,
		ORDER_SIDE_SELL,
		ORDER_SIDE_OFFER = ORDER_SIDE_SELL,
		ORDER_SIDE_ASK = ORDER_SIDE_SELL,
		ORDER_SIDE_SHORT = ORDER_SIDE_SELL,
		numberOfOrderSides
	};

	enum TimeInForce {
		// Good Till Day.
		TIME_IN_FORCE_DAY,
		// Good Till Cancel.
		TIME_IN_FORCE_GTC,
		// At the Opening.
		TIME_IN_FORCE_OPG,
		// Immediate or Cancel.
		TIME_IN_FORCE_IOC,
		// Fill or Kill.
		TIME_IN_FORCE_FOK,
		numberOfTimeInForces
	};

	//! Extended order parameters.
	struct OrderParams {

		//! Account.
		boost::optional<std::string> account;
		
		//! Display size for Iceberg orders.
		boost::optional<trdk::Qty> displaySize;

		//! Minimum trade quantity. Must be at most the order quantity.
		/** For cache pair could be in different currency.
		  */
		boost::optional<trdk::Qty> minTradeQty;
		
		//! Good Till Time.
		/** Absolute value in Coordinated Universal Time (UTC). Incompatible
		  * with goodInSeconds.
		  * @sa trdk::OrderParams::goodInSeconds
		  */
		boost::optional<boost::posix_time::ptime> goodTillTime;
		//! Good next N seconds.
		/** Incompatible with goodTillTime.
		  * @sa trdk::OrderParams::goodTillTime
		  */
		boost::optional<uintmax_t> goodInSeconds;

		//! Order ID to replace.
		boost::optional<uintmax_t> orderIdToReplace;

		//! Order sent not by strategy.
		bool isManualOrder;

		//! Defines order quantity precision.
		/** If set - order quantity will be rounded to this precision.
		  */
		boost::optional<uint8_t> qtyPrecision;

		explicit OrderParams()
			: isManualOrder(false) {
			//...//
		}

	};

}

namespace std {

	TRDK_CORE_API std::ostream & operator <<(
			std::ostream &,
			const trdk::OrderParams &);

}

////////////////////////////////////////////////////////////////////////////////

namespace trdk {

	enum Level1TickType {
		LEVEL1_TICK_LAST_PRICE,
		LEVEL1_TICK_LAST_QTY,
		LEVEL1_TICK_BID_PRICE,
		LEVEL1_TICK_BID_QTY,
		LEVEL1_TICK_ASK_PRICE,
		LEVEL1_TICK_ASK_QTY,
		LEVEL1_TICK_TRADING_VOLUME,
		numberOfLevel1TickTypes
	};

	struct Level1TickValue {

	private:

		explicit Level1TickValue(Level1TickType type, double value)
			: m_type(type),
			m_value(value) {
		}

	public:


		template<trdk::Level1TickType type, typename Value>
		static Level1TickValue Create(const Value &value) {
			return Level1TickValue(type, value);
		}

		const Level1TickType & GetType() const {
			return m_type;
		}

		double GetValue() const {
			return m_value;
		}

	private:

		Level1TickType m_type;
		double m_value;

	};

}

////////////////////////////////////////////////////////////////////////////////

namespace trdk {

	enum TradingMode {
		TRADING_MODE_PAPER,
		TRADING_MODE_LIVE,
		numberOfTradingModes,
	};
	TRDK_CORE_API trdk::TradingMode ConvertTradingModeFromString(
			const std::string &);
	TRDK_CORE_API const std::string & ConvertToString(
			const trdk::TradingMode &);

	enum StopMode {
		STOP_MODE_IMMEDIATELY,
		STOP_MODE_GRACEFULLY_ORDERS,
		STOP_MODE_GRACEFULLY_POSITIONS,
		STOP_MODE_UNKNOWN,
		numberOfStopModes = STOP_MODE_UNKNOWN
	};

}

namespace std {

	inline std::ostream & operator <<(
			std::ostream &oss,
			const trdk::TradingMode &mode) {
		oss << trdk::ConvertToString(mode);
		return oss;
	}

}

////////////////////////////////////////////////////////////////////////////////

namespace trdk {

	typedef std::vector<std::pair<trdk::Lib::Currency, double>> FinancialResult;

}

//////////////////////////////////////////////////////////////////////////
