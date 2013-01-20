/**************************************************************************
 *   Created: 2013/01/10 16:52:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Strategy.hpp"
#include "Position.hpp"
#include "PythonToCoreTransit.hpp"

namespace Trader { namespace PyApi {

	template<typename PyApiImpl>
	class SidePositionExport;

	class Strategy;

} }

namespace boost { namespace python {

	template<typename PyApiImpl>
	struct has_back_reference<Trader::PyApi::SidePositionExport<PyApiImpl>>
			: public mpl::true_ {
		//...//
	};

} }

namespace Trader { namespace PyApi {

	//////////////////////////////////////////////////////////////////////////

	class PositionExport {

	public:

		explicit PositionExport(const boost::shared_ptr<Trader::Position> &);

	public:

		static void Export(const char *className);

	public:

		Trader::Position & GetPosition();
		const Trader::Position & GetPosition() const;

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
		public boost::python::wrapper<SidePositionExport<PyApiImpl>>,
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
					PyApi::Strategy &strategy,
					int qty,
					double startPrice)
				: PositionExport(
					boost::shared_ptr<PyApiImpl>(
						new PyApiImpl(
							strategy.GetStrategy(),
							qty,
							strategy.GetStrategy()
								.GetSecurity()
								.ScalePrice(startPrice),
							*this))),
				PythonToCoreTransit<SidePositionExport>(self) {
			//...//
		}

	public:
		
		static void Export(const char *className) {
			namespace py = boost::python;
			typedef py::class_<
					SidePositionExport,
					py::bases<PositionExport>,
					Detail::PythonToCoreTransitHolder<SidePositionExport>,
					boost::noncopyable>
				Export;
			typedef py::init<
					PyApi::Strategy &,
					int /*qty*/,
					double /*startPrice*/>
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
		
		boost::python::override GetOverride(const char *name) const {
			return get_override(name);
		}
	
	};

	typedef SidePositionExport<PyApi::LongPosition> LongPositionExport;
	typedef SidePositionExport<PyApi::ShortPosition> ShortPositionExport;

#	ifdef BOOST_WINDOWS
#		pragma warning(pop)
#	endif

	//////////////////////////////////////////////////////////////////////////

	boost::python::object Export(Trader::Position &);
	Trader::Position & ExtractPosition(const boost::python::object &);

	//////////////////////////////////////////////////////////////////////////

} }
