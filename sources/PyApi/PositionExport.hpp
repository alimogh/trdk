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

#include "PyPosition.hpp"
#include "StrategyExport.hpp"
#include "PyStrategy.hpp"
#include "PythonToCoreTransit.hpp"
#include "OrderParamsExport.hpp"

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
		bool IsError() const;
		bool IsCanceled() const;

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

		boost::python::object GetSecurity();

	public:

		void RestoreOpenState();
		void RestoreOpenStateWithOpenOrderId(trdk::OrderId openOrderId);

		OrderId OpenAtMarketPrice();
		OrderId OpenAtMarketPriceWithParams(const OrderParamsExport &);
		OrderId Open(ScaledPrice);
		OrderId OpenWithParams(ScaledPrice, const OrderParamsExport &);
		OrderId OpenAtMarketPriceWithStopPrice(ScaledPrice stopPrice);
		OrderId OpenAtMarketPriceWithStopPriceWithParams(
					ScaledPrice stopPrice,
					const OrderParamsExport &);
		OrderId OpenOrCancel(ScaledPrice);

		OrderId CloseAtMarketPrice();
		OrderId CloseAtMarketPriceWithParams(const OrderParamsExport &);
		OrderId Close(ScaledPrice);
		OrderId CloseWithParams(ScaledPrice, const OrderParamsExport &);
		OrderId CloseAtMarketPriceWithStopPrice(ScaledPrice stopPrice);
		OrderId CloseAtMarketPriceWithStopPriceWithParams(
					ScaledPrice stopPrice,
					const OrderParamsExport &);
		OrderId CloseOrCancel(ScaledPrice);

		bool CancelAtMarketPrice();
		bool CancelAtMarketPriceWithParams(const OrderParamsExport &);

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
					SecurityExport &security,
					Qty qty,
					ScaledPrice startPrice)
				: PositionExport(
					boost::shared_ptr<PyApiImpl>(
						new PyApiImpl(
							strategy.GetStrategy(),
							security.GetSecurity(),
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
					SecurityExport &,
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
