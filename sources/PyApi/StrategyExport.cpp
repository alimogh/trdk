/**************************************************************************
 *   Created: 2013/01/10 21:17:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "StrategyExport.hpp"
#include "PyStrategy.hpp"
#include "ContextExport.hpp"
#include "PositionExport.hpp"
#include "PyPosition.hpp"
#include "PyService.hpp"
#include "BaseExport.hpp"
#include "Core/Settings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::PyApi;
using namespace trdk::PyApi::Detail;

namespace py = boost::python;
namespace pt = boost::posix_time;

//////////////////////////////////////////////////////////////////////////

StrategyInfoExport::StrategyInfoExport(
			const boost::shared_ptr<PyApi::Strategy> &strategy)
		: ModuleExport(*strategy),
		m_strategy(&*strategy),
		m_strategyRefHolder(strategy) {
	Assert(m_strategy == &ModuleExport::GetModule());
}

PyApi::Strategy & StrategyInfoExport::GetStrategy() {
	Assert(m_strategy == &ModuleExport::GetModule());
	return *m_strategy;
}

const PyApi::Strategy & StrategyInfoExport::GetStrategy() const {
	return const_cast<StrategyInfoExport *>(this)->GetStrategy();
}

boost::shared_ptr<PyApi::Strategy> StrategyInfoExport::ReleaseRefHolder()
		throw() {
	Assert(m_strategyRefHolder);
	const auto strategyRefHolder = m_strategyRefHolder;
	m_strategyRefHolder.reset();
	return strategyRefHolder;
}

//////////////////////////////////////////////////////////////////////////

StrategyExport::PositionListExport::IteratorExport::IteratorExport(
			const trdk::Strategy::PositionList::Iterator &iterator)
		: iterator_adaptor(iterator) {
	//...//
}

py::object StrategyExport::PositionListExport::IteratorExport::dereference()
		const {
	return PyApi::Export(*base_reference());
}

//////////////////////////////////////////////////////////////////////////

StrategyExport::PositionListExport::PositionListExport(
			trdk::Strategy::PositionList &list)
		: m_list(&list) {
	//...//
}

void StrategyExport::PositionListExport::ExportClass(const char *className) {
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

#ifdef BOOST_WINDOWS
#	pragma warning(push)
#	pragma warning(disable: 4355)
#endif

StrategyExport::StrategyExport(PyObject *self, uintptr_t instanceParam)
		: StrategyInfoExport(
			boost::shared_ptr<PyApi::Strategy>(
				new PyApi::Strategy(instanceParam, *this))),
		Detail::PythonToCoreTransit<StrategyExport>(self) {
	//...//
}

#ifdef BOOST_WINDOWS
#	pragma warning(pop)
#endif

namespace boost { namespace python {

	template<>
	struct has_back_reference<StrategyExport> : public mpl::true_ {
		//...//
	};

} }

void StrategyExport::ExportClass(const char *className) {
	
	typedef py::class_<
			StrategyExport,
			py::bases<ModuleExport>,
			Detail::PythonToCoreTransitHolder<StrategyExport>,
			boost::noncopyable>
		Export;

	const py::scope strategyClass = Export(className, py::init<uintptr_t>())
		.add_property("context", &StrategyExport::GetContext)
		.add_property("positions", &StrategyExport::GetPositions)
		.add_property("securities", &StrategyExport::GetSecurities)
		.def("findSecurity", &StrategyExport::FindSecurityBySymbol)
		.def("findSecurity", &StrategyExport::FindSecurityBySymbolAndExchange)
		.def(
			"findSecurity",
			&StrategyExport::FindSecurityBySymbolAndPrimaryExchange);

	PositionListExport::ExportClass("PositionList");
	ConsumerSecurityListExport::ExportClass("SecurityList");

}

ContextExport StrategyExport::GetContext() {
	return ContextExport(GetStrategy().GetContext());
}

py::object StrategyExport::FindSecurityBySymbol(const py::str &symbol) {
	const Symbol key(
		py::extract<std::string>(symbol),
		GetModule().GetContext().GetSettings().GetDefaultExchange(),
		GetModule().GetContext().GetSettings().GetDefaultPrimaryExchange());
	try {
		return PyApi::Export(GetStrategy().GetContext().GetSecurity(key));
	} catch (const Context::UnknownSecurity &) {
		return py::object();
	}
}

py::object StrategyExport::FindSecurityBySymbolAndExchange(
			const py::str &symbol,
			const py::str &exchange) {
	const Symbol key(
		py::extract<std::string>(symbol),
		py::extract<std::string>(exchange),
		GetModule().GetContext().GetSettings().GetDefaultPrimaryExchange());
	try {
		return PyApi::Export(GetStrategy().GetContext().GetSecurity(key));
	} catch (const Context::UnknownSecurity &) {
		return py::object();
	}
}

py::object StrategyExport::FindSecurityBySymbolAndPrimaryExchange(
			const py::str &symbol,
			const py::str &exchange,
			const py::str &primaryExchange) {
	try {
		return PyApi::Export(
			GetStrategy()
				.GetContext()
				.GetSecurity(
					Symbol(
						py::extract<std::string>(symbol),
						py::extract<std::string>(exchange),
						py::extract<std::string>(primaryExchange))));
	} catch (const Context::UnknownSecurity &) {
		return py::object();
	}
}

StrategyExport::PositionListExport StrategyExport::GetPositions() {
	return PositionListExport(GetStrategy().GetPositions());
}

ConsumerSecurityListExport StrategyExport::GetSecurities() {
	return ConsumerSecurityListExport(GetStrategy());
}

//////////////////////////////////////////////////////////////////////////
