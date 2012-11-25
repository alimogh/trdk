/**************************************************************************
 *   Created: 2012/11/21 20:15:29
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Import.hpp"
#include "Service.hpp"

using namespace Trader::PyApi::Import;
namespace py = boost::python;
namespace pt = boost::posix_time;

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
