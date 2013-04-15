/**************************************************************************
 *   Created: 2013/01/10 16:52:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Position.hpp"
#include "StrategyExport.hpp"
#include "Strategy.hpp"
#include "PythonToCoreTransit.hpp"

namespace boost { namespace python {

	template<typename PyApiImpl>
	struct has_back_reference<trdk::PyApi::SidePositionExport<PyApiImpl>>
			: public mpl::true_ {
		//...//
	};

} }

namespace trdk { namespace PyApi {

	//////////////////////////////////////////////////////////////////////////

	class PositionExport {

	public:

		explicit PositionExport(const boost::shared_ptr<trdk::Position> &);

	public:

		static void ExportClass(const char *className);

	public:

		trdk::Position & GetPosition();
		const trdk::Position & GetPosition() const;

		void ResetRefHolder() throw();

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

		boost::shared_ptr<Position> m_positionRefHolder;
		Position *m_position;

	};

	//////////////////////////////////////////////////////////////////////////

#	ifdef BOOST_WINDOWS
#		pragma warning(push)
#		pragma warning(disable: 4355)
#	endif

	template<typename PyApiImpl>
	class SidePositionExport
		: public PositionExport,
		public Detail::PythonToCoreTransit<SidePositionExport<PyApiImpl>>,
		public boost::enable_shared_from_this<SidePositionExport<PyApiImpl>> {

	public:

		explicit SidePositionExport(typename PyApiImpl::Impl &positionRef)
				: PositionExport(positionRef.shared_from_this()) {
			//...//
		}

		//! Creates new instance.
		explicit SidePositionExport(
					PyObject *self,
					StrategyExport &strategy,
					Qty qty,
					ScaledPrice startPrice)
				: PositionExport(
					boost::shared_ptr<PyApiImpl>(
						new PyApiImpl(
							strategy.GetStrategy(),
							qty,
							startPrice,
							*this))),
				PythonToCoreTransit<SidePositionExport>(self) {
			//...//
		}

	public:
		
		static void ExportClass(const char *className) {
			namespace py = boost::python;
			typedef py::class_<
					SidePositionExport,
					py::bases<PositionExport>,
					Detail::PythonToCoreTransitHolder<SidePositionExport>,
					boost::noncopyable>
				Export;
			typedef py::init<
					StrategyExport &,
					Qty /*qty*/,
					ScaledPrice /*startPrice*/>
				Init;
			Export(className, Init());
		}
	
	public:
	
		typename PyApiImpl::Impl & GetPosition() {
			return *boost::polymorphic_downcast<PyApiImpl::Impl *>(
				&PyApi::Position::GetPosition());
		}
		
		const typename PyApiImpl::Impl & GetPosition() const {
			return const_cast<SidePositionExport *>(this)->GetPosition();
		}
	
	};

	typedef SidePositionExport<PyApi::LongPosition> LongPositionExport;
	typedef SidePositionExport<PyApi::ShortPosition> ShortPositionExport;

#	ifdef BOOST_WINDOWS
#		pragma warning(pop)
#	endif

	//////////////////////////////////////////////////////////////////////////

	boost::python::object Export(trdk::Position &);
	trdk::Position & ExtractPosition(const boost::python::object &);

	//////////////////////////////////////////////////////////////////////////

} }
