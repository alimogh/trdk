/**************************************************************************
 *   Created: 2013/09/06 00:14:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "OrderParamsExport.hpp"
#include "BaseExport.hpp"

using namespace trdk;
using namespace trdk::PyApi;
using namespace trdk::Lib;

namespace py = boost::python;
namespace pt = boost::posix_time;

OrderParamsExport::OrderParamsExport(PyObject *self)
		: Detail::PythonToCoreTransit<OrderParamsExport>(self) {
	//...//
}

namespace boost { namespace python {

	template<>
	struct has_back_reference<OrderParamsExport> : public mpl::true_ {
		//...//
	};

} }

void OrderParamsExport::ExportClass(const char *className) {
	typedef py::class_<
			OrderParamsExport,
			Detail::PythonToCoreTransitHolder<OrderParamsExport>,
			boost::noncopyable>
		Export;
	Export(className, py::init<>())
		.add_property(
			"displaySize",
			&OrderParamsExport::GetDisplaySize,
			&OrderParamsExport::SetDisplaySize)
		.add_property(
			"goodTillTime",
			&OrderParamsExport::GetGoodTillTime,
			&OrderParamsExport::SetGoodTillTime)
		.add_property(
			"goodInSeconds",
			&OrderParamsExport::GetGoodInSeconds,
			&OrderParamsExport::SetGoodInSeconds);
}

const OrderParams & OrderParamsExport::GetOrderParams() const {
	return m_orderParams;
}

py::object OrderParamsExport::GetDisplaySize() const {
	return m_orderParams.displaySize
		?	PyApi::Export(*m_orderParams.displaySize)
		:	py::object();
}

void OrderParamsExport::SetDisplaySize(const py::object &newDisplaySize) {
	if (newDisplaySize) {
		m_orderParams.displaySize = py::extract<Qty>(newDisplaySize);
	} else {
		m_orderParams.displaySize.reset();
	}
}

py::object OrderParamsExport::GetGoodTillTime() const {
	return m_orderParams.goodTillTime
		?	PyApi::Export(ConvertToTimeT(*m_orderParams.goodTillTime))
		:	py::object();
}

void OrderParamsExport::SetGoodTillTime(const py::object &newTime) {
	if (newTime) {
		py::extract<double> floatValue(newTime);
		const int64_t timestamp = floatValue.check()
			?	int64_t(floatValue)
			:	py::extract<int64_t>(newTime);
		m_orderParams.goodTillTime = pt::from_time_t(timestamp);
	} else {
		m_orderParams.goodTillTime.reset();
	}
}

py::object OrderParamsExport::GetGoodInSeconds() const {
	return m_orderParams.goodInSeconds
		?	PyApi::Export(*m_orderParams.goodInSeconds)
		:	py::object();
}

void OrderParamsExport::SetGoodInSeconds(const py::object &newPeriod) {
	if (newPeriod) {
		const py::extract<double> floatValue(newPeriod);
		m_orderParams.goodInSeconds = floatValue.check()
			?	int64_t(floatValue)
			:	py::extract<int64_t>(newPeriod);
	} else {
		m_orderParams.goodInSeconds.reset();
	}
}
