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
			Trader::PyApi::Export::Security &,
			int /*qty*/,
			double /*startPrice*/,
			const std::string &>
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

void SecurityAlgo::CallNotifyServiceStartPyMethod(
			const py::object &servicePyObject) {
	Assert(servicePyObject);
	try {
		 py::extract<PyApi::Service> getPyServiceImpl(servicePyObject);
		if (getPyServiceImpl.check()) {
			m_algo.Trader::SecurityAlgo::NotifyServiceStart(getPyServiceImpl());
		} else {
			const Export::Service &service
				= py::extract<const Export::Service &>(servicePyObject);
			m_algo.Trader::SecurityAlgo::NotifyServiceStart(
				service.GetService());
		}
	} catch (const py::error_already_set &) {
		Detail::RethrowPythonClientException(
			"Failed to convert object to Trader.Service");
		throw;
	}
}

bool SecurityAlgo::CallOnNewTradePyMethod(
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
		Detail::RethrowPythonClientException(
			"Failed to convert price to Trader.ScaledPrice");
		throw;
	}
	Trader::Qty qty;
	try {
		qty = py::extract<decltype(qty)>(pyQtyObject);
	} catch (const py::error_already_set &) {
		Detail::RethrowPythonClientException(
			"Failed to convert qty to Trader.Qty");
		throw;
	}
	const Trader::OrderSide side = Detail::OrderSide::Convert(pySideObject);
	return m_algo.OnNewTrade(time, price, qty, side);
}

bool SecurityAlgo::CallOnServiceDataUpdatePyMethod(
			const py::object &servicePyObject) {
	Assert(servicePyObject);
	try {
		 py::extract<PyApi::Service> getPyServiceImpl(servicePyObject);
		if (getPyServiceImpl.check()) {
			return m_algo.Trader::SecurityAlgo::OnServiceDataUpdate(getPyServiceImpl());
		} else {
			const Export::Service &service
				= py::extract<const Export::Service &>(servicePyObject);
			return m_algo.Trader::SecurityAlgo::OnServiceDataUpdate(
				service.GetService());
		}
	} catch (const py::error_already_set &) {
		Detail::RethrowPythonClientException(
			"Failed to convert object to Trader.Service");
		throw;
	}
}

//////////////////////////////////////////////////////////////////////////

void Service::Export(const char *className) {
	typedef py::class_<
			PyApi::Service,
			py::bases<SecurityAlgo>,
			boost::shared_ptr<PyApi::Service>,
			boost::noncopyable>
		ServiceImport;
 	ServiceImport(className, py::no_init);
}

//////////////////////////////////////////////////////////////////////////

void Strategy::Export(const char *className) {
	typedef py::class_<
			PyApi::Strategy,
			py::bases<SecurityAlgo>,
			boost::shared_ptr<PyApi::Strategy>,
			boost::noncopyable>
		StrategyImport;
	StrategyImport(className, py::init<uintmax_t>())
		.def(
			"notifyServiceStart",
			&Strategy::CallNotifyServiceStartPyMethod)
		.def(
			"tryToOpenPositions",
			pure_virtual(&Strategy::CallTryToOpenPositionsPyMethod))
		.def(
			"tryToClosePositions",
			pure_virtual(&Strategy::CallTryToClosePositionsPyMethod));
}

//////////////////////////////////////////////////////////////////////////
