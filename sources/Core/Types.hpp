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

	typedef boost::int32_t Qty;
	
	typedef boost::int64_t ScaledPrice;

	typedef boost::uint64_t OrderId;

	enum OrderSide {
		ORDER_SIDE_BUY,
		ORDER_SIDE_BID = ORDER_SIDE_BUY,
		ORDER_SIDE_SELL,
		ORDER_SIDE_OFFER = ORDER_SIDE_SELL,
		ORDER_SIDE_ASK = ORDER_SIDE_SELL,
		numberOfOrderSides
	};

	//! Extended order parameters.
	struct OrderParams {

		//! Account.
		boost::optional<std::string> account;
		
		//! Display size for Iceberg orders.
		boost::optional<trdk::Qty> displaySize;
		
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

	template<Level1TickType tickType>
	struct Level1TickValuePolicy {
		//...//
	};

	struct Level1TickValue {

		Level1TickType type;

		union Value {
			trdk::ScaledPrice lastPrice;
			trdk::Qty lastQty;
			trdk::ScaledPrice bidPrice;
			trdk::Qty bidQty;
			trdk::ScaledPrice askPrice;
			trdk::Qty askQty;
			trdk::Qty tradingVolume;
		} value;

	private:

		explicit Level1TickValue(Level1TickType type)
				: type(type) {
			//...//
		}

	public:


		template<trdk::Level1TickType type>
		static Level1TickValue Create(
					typename trdk::Level1TickValuePolicy<type>::ValueType
						value) {
			Level1TickValue result(type);
			trdk::Level1TickValuePolicy<type>::Get(result.value)
				= value;
			return result;
		}

		intmax_t Get() const;

	};

	template<>
	struct Level1TickValuePolicy<trdk::LEVEL1_TICK_LAST_PRICE> {
		typedef trdk::ScaledPrice ValueType;
		static ValueType Get(const trdk::Level1TickValue::Value &value) {
			return Get(const_cast<trdk::Level1TickValue::Value &>(value));
		}
		static ValueType & Get(trdk::Level1TickValue::Value &value) {
			return value.lastPrice;
		}
	};
	template<>
	struct Level1TickValuePolicy<trdk::LEVEL1_TICK_LAST_QTY> {
		typedef trdk::Qty ValueType;
		static ValueType Get(const trdk::Level1TickValue::Value &value) {
			return Get(const_cast<trdk::Level1TickValue::Value &>(value));
		}
		static ValueType & Get(trdk::Level1TickValue::Value &value) {
			return value.lastQty;
		}
	};
	template<>
	struct Level1TickValuePolicy<trdk::LEVEL1_TICK_BID_PRICE> {
		typedef trdk::ScaledPrice ValueType;
		static ValueType Get(const trdk::Level1TickValue::Value &value) {
			return Get(const_cast<trdk::Level1TickValue::Value &>(value));
		}
		static ValueType & Get(trdk::Level1TickValue::Value &value) {
			return value.bidPrice;
		}
	};
	template<>
	struct Level1TickValuePolicy<trdk::LEVEL1_TICK_BID_QTY> {
		typedef trdk::Qty ValueType;
		static ValueType Get(const trdk::Level1TickValue::Value &value) {
			return Get(const_cast<trdk::Level1TickValue::Value &>(value));
		}
		static ValueType & Get(trdk::Level1TickValue::Value &value) {
			return value.bidQty;
		}
	};
	template<>
	struct Level1TickValuePolicy<trdk::LEVEL1_TICK_ASK_PRICE> {
		typedef trdk::ScaledPrice ValueType;
		static ValueType Get(const trdk::Level1TickValue::Value &value) {
			return Get(const_cast<trdk::Level1TickValue::Value &>(value));
		}
		static ValueType & Get(trdk::Level1TickValue::Value &value) {
			return value.askPrice;
		}
	};
	template<>
	struct Level1TickValuePolicy<trdk::LEVEL1_TICK_ASK_QTY> {
		typedef trdk::Qty ValueType;
		static ValueType Get(const trdk::Level1TickValue::Value &value) {
			return Get(const_cast<trdk::Level1TickValue::Value &>(value));
		}
		static ValueType & Get(trdk::Level1TickValue::Value &value) {
			return value.askQty;
		}
	};
	template<>
	struct Level1TickValuePolicy<trdk::LEVEL1_TICK_TRADING_VOLUME> {
		typedef trdk::Qty ValueType;
		static ValueType Get(const trdk::Level1TickValue::Value &value) {
			return Get(const_cast<trdk::Level1TickValue::Value &>(value));
		}
		static ValueType & Get(trdk::Level1TickValue::Value &value) {
			return value.tradingVolume;
		}
	};

	inline intmax_t Level1TickValue::Get() const {
		static_assert(
			numberOfLevel1TickTypes == 7,
			"Changed Level 1 Tick type list.");
		switch (type) {
			case LEVEL1_TICK_LAST_PRICE:
				{
					typedef trdk::Level1TickValuePolicy<
							LEVEL1_TICK_LAST_PRICE>
						Policy;
					return Policy::Get(value);
				}
			case LEVEL1_TICK_LAST_QTY:
				{
					typedef trdk::Level1TickValuePolicy<
							LEVEL1_TICK_LAST_QTY>
						Policy;
					return Policy::Get(value);
				}
			case LEVEL1_TICK_BID_PRICE:
				{
					typedef trdk::Level1TickValuePolicy<
							LEVEL1_TICK_BID_PRICE>
						Policy;
					return Policy::Get(value);
				}
			case LEVEL1_TICK_BID_QTY:
				{
					typedef trdk::Level1TickValuePolicy<
							LEVEL1_TICK_BID_QTY>
						Policy;
					return Policy::Get(value);
				}
			case LEVEL1_TICK_ASK_PRICE:
				{
					typedef trdk::Level1TickValuePolicy<
							LEVEL1_TICK_ASK_PRICE>
						Policy;
					return Policy::Get(value);
				}
			case LEVEL1_TICK_ASK_QTY:
				{
					typedef trdk::Level1TickValuePolicy<
							LEVEL1_TICK_ASK_QTY>
						Policy;
					return Policy::Get(value);
				}
			case LEVEL1_TICK_TRADING_VOLUME:
				{
					typedef trdk::Level1TickValuePolicy<
							LEVEL1_TICK_TRADING_VOLUME>
						Policy;
					return Policy::Get(value);
				}
			default:
				AssertEq(LEVEL1_TICK_LAST_PRICE, type);
				return 0;
		}
	}


}

////////////////////////////////////////////////////////////////////////////////

namespace trdk {

	enum StopMode {
		STOP_MODE_IMMEDIATELY,
		STOP_MODE_GRACEFULLY_ORDERS,
		STOP_MODE_GRACEFULLY_POSITIONS,
		STOP_MODE_UNKNOWN,
		numberOfStopModes = STOP_MODE_UNKNOWN
	};

}

////////////////////////////////////////////////////////////////////////////////
