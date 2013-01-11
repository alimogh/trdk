/**************************************************************************
 *   Created: 2013/01/10 21:17:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "StrategyExport.hpp"
#include "Strategy.hpp"
#include "PositionExport.hpp"
#include "Position.hpp"
#include "Service.hpp"
#include "Detail.hpp"

using namespace Trader;
using namespace Trader::PyApi;
using namespace Trader::PyApi::Detail;

namespace py = boost::python;
namespace pt = boost::posix_time;

//////////////////////////////////////////////////////////////////////////

StrategyExport::PositionListExport::IteratorExport::IteratorExport(
			const Trader::Strategy::PositionList::Iterator &iterator)
		: iterator_adaptor(iterator) {
	//...//
}

namespace {

	template<typename PyApiPosition, typename CorePosition>
	py::object GetPyPosition(Position &source) {
		static_assert(
			boost::is_base_of<CorePosition, PyApiPosition>::value,
			"CorePosition must be base of PyApiPosition.");
		Assert(
			dynamic_cast<PyApiPosition *>(&source)
			|| dynamic_cast<CorePosition *>(&source));
		PyApiPosition *const pyApiPosition
			= dynamic_cast<PyApiPosition *>(&source);
		if (!pyApiPosition) {
			CorePositionToExport<CorePosition>::Export positionExport(
				dynamic_cast<CorePosition &>(source));
			return py::object(boost::ref(positionExport));
		} else {
			return pyApiPosition->GetSelf();
		}
	}

}

py::object StrategyExport::PositionListExport::IteratorExport::dereference()
		const {
 	Position &position = *base_reference();
 	static_assert(Position::numberOfTypes == 2, "Position type list changed.");
 	switch (position.GetType()) {
  		case Position::TYPE_LONG:
  			return GetPyPosition<PyApi::LongPosition, Trader::LongPosition>(
				position);
  		case Position::TYPE_SHORT:
  			return GetPyPosition<PyApi::ShortPosition, Trader::ShortPosition>(
				position);
 		default:
 			AssertNe(int(Position::numberOfTypes), int(position.GetType()));
 			throw Lib::Exception("Unknown position type");
 	}
}

//////////////////////////////////////////////////////////////////////////

StrategyExport::PositionListExport::PositionListExport(
			Trader::Strategy::PositionList &list)
		: m_list(&list) {
	//...//
}

void StrategyExport::PositionListExport::Export(const char *className) {
	py::class_<PositionListExport>(className, py::no_init)
	    .def("__iter__", py::iterator<PositionListExport>())
		.def("count", &PositionListExport::GetSize);
}

size_t StrategyExport::PositionListExport::GetSize() const {
	return m_list->GetSize();
}

StrategyExport::PositionListExport::iterator
StrategyExport::PositionListExport::begin() {
	return IteratorExport(m_list->GetBegin());
}

StrategyExport::PositionListExport::iterator
StrategyExport::PositionListExport::end() {
	return IteratorExport(m_list->GetEnd());
}

//////////////////////////////////////////////////////////////////////////

StrategyExport::StrategyExport(Trader::Strategy &strategy)
		: SecurityAlgoExport(strategy) {
	//...//
}

void StrategyExport::Export(const char *className) {
	
	typedef py::class_<
			PyApi::Strategy,
			py::bases<SecurityAlgoExport>,
			boost::shared_ptr<PyApi::Strategy>,
			boost::noncopyable>
		StrategyImport;

	const py::scope strategyClass = StrategyImport(
			className,
			py::init<uintmax_t>())
		.add_property("positions", &StrategyExport::GetPositions);

	PositionListExport::Export("PositionList");

}

StrategyExport::PositionListExport StrategyExport::GetPositions() {
	return PositionListExport(GetStrategy().GetPositions());
}

void StrategyExport::CallOnLevel1UpdatePyMethod() {
	GetStrategy().OnLevel1Update();
}

void StrategyExport::CallOnNewTradePyMethod(
			const py::object &pyTimeObject,
			const py::object &pyPriceObject,
			const py::object &pyQtyObject,
			const py::object &pySideObject) {
	Assert(pyTimeObject);
	Assert(pyPriceObject);
	Assert(pyQtyObject);
	Assert(pySideObject);
	const pt::ptime time = Detail::Time::Convert(pyTimeObject);
	ScaledPrice price;
	try {
		price = py::extract<decltype(price)>(pyPriceObject);
	} catch (const py::error_already_set &) {
		Detail::LogPythonClientException();
		throw PyApi::Error(
			"Failed to convert price to Trader::ScaledPrice");
	}
	Qty qty;
	try {
		qty = py::extract<decltype(qty)>(pyQtyObject);
	} catch (const py::error_already_set &) {
		Detail::LogPythonClientException();
		throw PyApi::Error("Failed to convert qty to Trader::Qty");
	}
	const OrderSide side = Detail::OrderSide::Convert(pySideObject);
	GetStrategy().OnNewTrade(time, price, qty, side);
}

void StrategyExport::CallOnServiceDataUpdatePyMethod(
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
		throw PyApi::Error(
			"Failed to convert object to Trader::Service");
	}
}

void StrategyExport::CallOnPositionUpdatePyMethod(const py::object &/*position*/) {
	//! @todo: not implemented method! translate call to the core implementation!
	AssertFail("StrategyExport::CallOnPositionUpdatePyMethod");
}

Trader::Strategy & StrategyExport::GetStrategy() {
	return Get<Trader::Strategy>();
}

const Trader::Strategy & StrategyExport::GetStrategy() const {
	return Get<Trader::Strategy>();
}

//////////////////////////////////////////////////////////////////////////
