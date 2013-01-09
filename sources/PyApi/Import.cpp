/**************************************************************************
 *   Created: 2012/11/21 20:15:29
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Import.hpp"
#include "Export.hpp"
#include "Strategy.hpp"
#include "Service.hpp"
#include "Position.hpp"

using namespace Trader::PyApi::Import;
namespace py = boost::python;
namespace pt = boost::posix_time;

//////////////////////////////////////////////////////////////////////////

void Position::Export(const char *className) {

	py::class_<Position, boost::noncopyable>(className, py::no_init)

		.add_property("type", &Position::GetTypeStr)

		.add_property("isStarted", &Position::IsStarted)
		.add_property("isCompleted", &Position::IsCompleted)

		.add_property("isOpened", &Position::IsOpened)
		.add_property("isClosed", &Position::IsClosed)

		.add_property("hasActiveOrders", &Position::HasActiveOrders)

		.add_property("planedQty", &Position::GetPlanedQty)

		.add_property("openStartPrice", &Position::GetOpenStartPrice)
		.add_property("openOrderId", &Position::GetOpenOrderId)
		.add_property("openedQty", &Position::GetOpenedQty)
		.add_property("openPrice", &Position::GetOpenPrice)

		.add_property("notOpenedQty", &Position::GetNotOpenedQty)
		.add_property("activeQty", &Position::GetActiveQty)

		.add_property("closeOrderId", &Position::GetCloseOrderId)
		.add_property("closeStartPrice", &Position::GetCloseStartPrice)
		.add_property("closePrice", &Position::GetClosePrice)
		.add_property("closedQty", &Position::GetClosedQty)

		.add_property("commission", &Position::GetCommission)

		.def("openAtMarketPrice", &Position::OpenAtMarketPrice)
		.def("open", &Position::Open)
		.def(
			"openAtMarketPriceWithStopPrice",
			&Position::OpenAtMarketPriceWithStopPrice)
		.def("openOrCancel", &Position::OpenOrCancel)

		.def("closeAtMarketPrice", &Position::CloseAtMarketPrice)
		.def("close", &Position::Close)
		.def(
			"closeAtMarketPriceWithStopPrice",
			&Position::CloseAtMarketPriceWithStopPrice)
		.def("closeOrCancel", &Position::CloseOrCancel)

		.def("cancelAtMarketPrice", &Position::CancelAtMarketPrice)
		.def("cancelAllOrders", &Position::CancelAllOrders);
		
}

namespace {
	typedef py::init<
			Trader::PyApi::Strategy &,
			int /*qty*/,
			double /*startPrice*/>
		PositionImportInit;
}

void LongPosition::Export(const char *className) {
	typedef py::class_<
			PyApi::LongPosition,
			py::bases<Import::Position>,
			boost::shared_ptr<PyApi::LongPosition>,
			boost::noncopyable>
		LongPositionImport;
	LongPositionImport(className, PositionImportInit());
}

void ShortPosition::Export(const char *className) {
	typedef py::class_<
			PyApi::ShortPosition,
			py::bases<Import::Position>,
			boost::shared_ptr<PyApi::ShortPosition>,
			boost::noncopyable>
		ShortPositionImport;
	ShortPositionImport(className, PositionImportInit());
}

//////////////////////////////////////////////////////////////////////////

void SecurityAlgo::CallOnServiceStartPyMethod(
			const py::object &servicePyObject) {
	Assert(servicePyObject);
	try {
		 py::extract<PyApi::Service> getPyServiceImpl(servicePyObject);
		if (getPyServiceImpl.check()) {
			m_algo.Trader::SecurityAlgo::OnServiceStart(getPyServiceImpl());
		} else {
			const Export::Service &service
				= py::extract<const Export::Service &>(servicePyObject);
			m_algo.Trader::SecurityAlgo::OnServiceStart(
				service.GetService());
		}
	} catch (const py::error_already_set &) {
		Detail::LogPythonClientException();
		throw Trader::PyApi::Error(
			"Failed to convert object to Trader::Service");
	}
}

//////////////////////////////////////////////////////////////////////////

Service::Service(Trader::Service &service)
		: SecurityAlgo(service) {
	//...//
}

void Service::Export(const char *className) {
	typedef py::class_<
			PyApi::Service,
			py::bases<SecurityAlgo>,
			boost::shared_ptr<PyApi::Service>,
			boost::noncopyable>
		ServiceImport;
 	ServiceImport(className, py::no_init);
}

bool Service::CallOnLevel1UpdatePyMethod() {
	return GetService().OnLevel1Update();
}

bool Service::CallOnNewTradePyMethod(
			const py::object &pyTimeObject,
			const py::object &pyPriceObject,
			const py::object &pyQtyObject,
			const py::object &pySideObject) {
	Assert(pyTimeObject);
	Assert(pyPriceObject);
	Assert(pyQtyObject);
	Assert(pySideObject);
	const pt::ptime time = Detail::Time::Convert(pyTimeObject);
	Trader::ScaledPrice price;
	try {
		price = py::extract<decltype(price)>(pyPriceObject);
	} catch (const py::error_already_set &) {
		Detail::LogPythonClientException();
		throw Trader::PyApi::Error(
			"Failed to convert price to Trader::ScaledPrice");
	}
	Trader::Qty qty;
	try {
		qty = py::extract<decltype(qty)>(pyQtyObject);
	} catch (const py::error_already_set &) {
		Detail::LogPythonClientException();
		throw Trader::PyApi::Error(
			"Failed to convert qty to Trader::Qty");
	}
	const Trader::OrderSide side = Detail::OrderSide::Convert(pySideObject);
	return GetService().OnNewTrade(time, price, qty, side);
}

bool Service::CallOnServiceDataUpdatePyMethod(
			const py::object &servicePyObject) {
	Assert(servicePyObject);
	try {
		 py::extract<PyApi::Service> getPyServiceImpl(servicePyObject);
		if (getPyServiceImpl.check()) {
			return GetService().Trader::Service::OnServiceDataUpdate(getPyServiceImpl());
		} else {
			const Export::Service &service
				= py::extract<const Export::Service &>(servicePyObject);
			return GetService().Trader::Service::OnServiceDataUpdate(
				service.GetService());
		}
	} catch (const py::error_already_set &) {
		Detail::LogPythonClientException();
		throw Trader::PyApi::Error(
			"Failed to convert object to Trader::Service");
	}
}


//////////////////////////////////////////////////////////////////////////

Strategy::PositionList::PositionList(Trader::Strategy::PositionList &list)
		: m_list(&list) {
	//...//
}

void Strategy::PositionList::Export(const char *className) {
	py::class_<PositionList>(className, py::no_init)
	    .def("__iter__", py::iterator<PositionList>())
		.def("count", &PositionList::GetSize);
}

size_t Strategy::PositionList::GetSize() const {
	return m_list->GetSize();
}

Strategy::PositionList::iterator Strategy::PositionList::begin() {
	return m_list->GetBegin();
}

Strategy::PositionList::iterator Strategy::PositionList::end() {
	return m_list->GetEnd();
}

Strategy::Strategy(Trader::Strategy &strategy)
		: SecurityAlgo(strategy) {
	//...//
}

void Strategy::Export(const char *className) {
	
	typedef py::class_<
			PyApi::Strategy,
			py::bases<SecurityAlgo>,
			boost::shared_ptr<PyApi::Strategy>,
			boost::noncopyable>
		StrategyImport;

	const py::scope strategyClass = StrategyImport(
			className,
			py::init<uintmax_t>())
		.add_property("positions", &Strategy::GetPositions);

	PositionList::Export("PositionList");

}

Strategy::PositionList Strategy::GetPositions() {
	return PositionList(GetStrategy().GetPositions());
}

void Strategy::CallOnLevel1UpdatePyMethod() {
	GetStrategy().OnLevel1Update();
}

void Strategy::CallOnNewTradePyMethod(
			const py::object &pyTimeObject,
			const py::object &pyPriceObject,
			const py::object &pyQtyObject,
			const py::object &pySideObject) {
	Assert(pyTimeObject);
	Assert(pyPriceObject);
	Assert(pyQtyObject);
	Assert(pySideObject);
	const pt::ptime time = Detail::Time::Convert(pyTimeObject);
	Trader::ScaledPrice price;
	try {
		price = py::extract<decltype(price)>(pyPriceObject);
	} catch (const py::error_already_set &) {
		Detail::LogPythonClientException();
		throw Trader::PyApi::Error(
			"Failed to convert price to Trader::ScaledPrice");
	}
	Trader::Qty qty;
	try {
		qty = py::extract<decltype(qty)>(pyQtyObject);
	} catch (const py::error_already_set &) {
		Detail::LogPythonClientException();
		throw Trader::PyApi::Error("Failed to convert qty to Trader::Qty");
	}
	const Trader::OrderSide side = Detail::OrderSide::Convert(pySideObject);
	GetStrategy().OnNewTrade(time, price, qty, side);
}

void Strategy::CallOnServiceDataUpdatePyMethod(
			const py::object &servicePyObject) {
	Assert(servicePyObject);
	try {
		 py::extract<PyApi::Service> getPyServiceImpl(servicePyObject);
		if (getPyServiceImpl.check()) {
			GetStrategy().Trader::Strategy::OnServiceDataUpdate(getPyServiceImpl());
		} else {
			const Export::Service &service
				= py::extract<const Export::Service &>(servicePyObject);
			GetStrategy().Trader::Strategy::OnServiceDataUpdate(service.GetService());
		}
	} catch (const py::error_already_set &) {
		Detail::LogPythonClientException();
		throw Trader::PyApi::Error(
			"Failed to convert object to Trader::Service");
	}
}

void Strategy::CallOnPositionUpdatePyMethod(const py::object &/*position*/) {
	//! @todo: not implemented method! translate call to the core implementation!
}

//////////////////////////////////////////////////////////////////////////
