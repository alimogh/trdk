/**************************************************************************
 *   Created: 2012/11/21 20:15:29
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Core/Position.hpp"
#include "Import.hpp"
#include "Export.hpp"
#include "Strategy.hpp"
#include "Service.hpp"

using namespace Trader::PyApi::Import;
namespace py = boost::python;
namespace pt = boost::posix_time;

//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////

Service::Service(Trader::Service &service)
		: SecurityAlgoExport(service) {
	//...//
}

void Service::Export(const char *className) {
	typedef py::class_<
			PyApi::Service,
			py::bases<SecurityAlgoExport>,
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

