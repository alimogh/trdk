/**************************************************************************
 *   Created: 2013/01/16 01:16:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "BaseExport.hpp"
#include "PositionExport.hpp"
#include "ServiceExport.hpp"
#include "StrategyExport.hpp"
#include "SecurityExport.hpp"
#include "Detail.hpp"

namespace py = boost::python;
namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::PyApi;
using namespace trdk::PyApi::Detail;

//////////////////////////////////////////////////////////////////////////

BOOST_PYTHON_MODULE(trdk) {

	using namespace trdk::PyApi;

	//! @todo: export __all__

	py::object module = py::scope();
	module.attr("__path__") = "trdk";

	SecurityInfoExport::ExportClass("SecurityInfo");
	SecurityExport::ExportClass("Security");

	PositionExport::ExportClass("Position");
	ShortPositionExport::ExportClass("ShortPosition");
	LongPositionExport::ExportClass("LongPosition");

	ModuleExport::ExportClass("Module");

	StrategyExport::ExportClass("Strategy");

	ServiceInfoExport::ExportClass("ServiceInfoExport");
	ServiceExport::ExportClass("Service");
	{
		//! @todo: export __all__
		py::object servicesModule(
			py::handle<>(py::borrowed(PyImport_AddModule("trdk.services"))));
		py::scope().attr("services") = servicesModule;
		py::scope servicesScope = servicesModule;
		BarServiceExport::ExportClass("BarService");
	}

}

void PyApi::ExportApi() {
	if (PyImport_AppendInittab("trdk", inittrdk) == -1) {
		throw Error("Failed to export Python API.");
	}
}

//////////////////////////////////////////////////////////////////////////

namespace {

	template<typename T>
	struct TypeTrait {
		//...//
	};

	template<>
	struct TypeTrait<boost::int32_t> {
		static char * GetName() {
			return "int32";
		}
	};

	template<>
	struct TypeTrait<boost::int64_t> {
		static char * GetName() {
			return "int64";
		}
	};

	template<typename T>
	py::object ExportType(const T &val) {
		try {
			return py::object(val);
		} catch (const py::error_already_set &) {
			LogPythonClientException();
			boost::format error("Failed to export %1%");
			error % TypeTrait<T>::GetName();
			throw Error(error.str().c_str());
		}
	}

}

py::object PyApi::Export(boost::int64_t val) {
	return ExportType(val);
}

py::object PyApi::Export(boost::int32_t val) {
	return ExportType(val);
}

namespace {
	const pt::ptime epochStart = pt::from_time_t(0);
}
py::object PyApi::Export(const pt::ptime &time) {
	AssertNe(epochStart, time);
	try {
		return py::object((time - epochStart).total_seconds());
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Failed to export posix time");
	}
}

pt::ptime PyApi::ExtractPosixTime(const py::object &time) {
	Assert(time);
	try {
		return pt::from_time_t(py::extract<time_t>(time));
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Failed to extract posix time");
	}
}

ScaledPrice PyApi::ExtractScaledPrice(const py::object &price) {
	Assert(price);
	try {
		return py::extract<ScaledPrice>(price);
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Failed to extract trdk::ScaledPrice");
	}
}

Qty PyApi::ExtractQty(const py::object &qty) {
	Assert(qty);
	try {
		return py::extract<Qty>(qty);
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Failed to extract trdk::Qty");
	}
}

py::object PyApi::Export(OrderSide side) {
	try {
		static_assert(numberOfOrderSides == 2, "Changed order side list.");
		switch (side) {
			case trdk::ORDER_SIDE_SELL:
				return py::object('S');
			case trdk::ORDER_SIDE_BUY:
				return py::object('B');
			default:
				AssertNe(int(trdk::numberOfOrderSides), side);
				return py::object(' ');
		}
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Failed to export trdk::OrderSide");
	}
}

OrderSide ExtractOrderSide(const py::object &orderSide) {
	Assert(orderSide);
	try {
		static_assert(numberOfOrderSides == 2, "Changed order side list.");
		switch (py::extract<char>(orderSide)) {
			case 'S':
				return trdk::ORDER_SIDE_SELL;
			case 'B':
				return trdk::ORDER_SIDE_BUY;
			default:
				throw trdk::PyApi::Error(
					"Order side can be 'B' (buy) or 'S' (sell)");
		}
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error(
			"Failed to extract trdk::OrderSide:"
				" order side can be 'B' (buy) or 'S' (sell)");
	}
}

//////////////////////////////////////////////////////////////////////////
