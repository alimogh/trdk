/**************************************************************************
 *   Created: 2013/01/10 16:52:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Strategy.hpp"

namespace Trader { namespace PyApi {

	//////////////////////////////////////////////////////////////////////////

	class PositionExport {

	public:

		//! C-tor.
		/*  @param positionRef	Reference to position, must be alive all time
		 *						while export object exists.
		 */
		explicit PositionExport(Trader::Position &positionRef);

	public:

		static void Export(const char *className);

	public:

		Position & GetPosition();
		const Position & GetPosition() const;

	public:

		bool IsCompleted() const;
		bool IsOpened() const;
		bool IsClosed() const;

		bool HasActiveOrders() const;
		bool HasActiveOpenOrders() const;
		bool HasActiveCloseOrders() const;

	public:

		const char * GetTypeStr() const;

		Qty GetPlanedQty() const;
		Qty GetNotOpenedQty() const;
		Qty GetActiveQty() const;

		ScaledPrice GetOpenStartPrice() const;
		OrderId GetOpenOrderId() const;
		Qty GetOpenedQty() const;
		ScaledPrice GetOpenPrice() const;

		OrderId GetCloseOrderId() const;
		ScaledPrice GetCloseStartPrice() const;
		ScaledPrice GetClosePrice() const;
		Qty GetClosedQty() const;

		ScaledPrice GetCommission() const;

	public:

		OrderId OpenAtMarketPrice();
		OrderId Open(ScaledPrice);
		OrderId OpenAtMarketPriceWithStopPrice(ScaledPrice stopPrice);
		OrderId OpenOrCancel(ScaledPrice);

		OrderId CloseAtMarketPrice();
		OrderId Close(ScaledPrice);
		OrderId CloseAtMarketPriceWithStopPrice(ScaledPrice stopPrice);
		OrderId CloseOrCancel(ScaledPrice);

		bool CancelAtMarketPrice();

		bool CancelAllOrders();

	private:

		Position *m_position;

	};

	//////////////////////////////////////////////////////////////////////////

	class LongPositionExport : public PositionExport {
	public:
		//! C-tor.
		/*  @param positionRef	Reference to position, must be alive all time
		 *						while export object exists.
		 */
		explicit LongPositionExport(Trader::Position &positionRef);
	public:
		static void Export(const char *className);
	};

	//////////////////////////////////////////////////////////////////////////

	class ShortPositionExport : public PositionExport {
	public:
		//! C-tor.
		/*  @param positionRef	Reference to position, must be alive all time
		 *						while export object exists.
		 */
		explicit ShortPositionExport(Trader::Position &positionRef);
	public:
		static void Export(const char *className);
	};

	//////////////////////////////////////////////////////////////////////////

	namespace Detail {

		template<typename CorePostion>
		struct CorePositionToExport {
			//...//
		};

		template<>
		struct CorePositionToExport<Trader::LongPosition> {
			typedef LongPositionExport Export;
		};

		template<>
		struct CorePositionToExport<Trader::ShortPosition> {
			typedef ShortPositionExport Export;
		};

	}

	//////////////////////////////////////////////////////////////////////////

	boost::python::object Extract(Trader::Position &);
	Trader::Position & Extract(const boost::python::object &);

	//////////////////////////////////////////////////////////////////////////

} }
