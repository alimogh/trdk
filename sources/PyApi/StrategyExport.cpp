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
#include "BaseExport.hpp"

using namespace Trader;
using namespace Trader::PyApi;
using namespace Trader::PyApi::Detail;

namespace py = boost::python;
namespace pt = boost::posix_time;

//////////////////////////////////////////////////////////////////////////

StrategyInfoExport::StrategyInfoExport(
			const boost::shared_ptr<PyApi::Strategy> &strategy)
		: SecurityAlgoExport(*strategy),
		m_strategyRefHolder(strategy),
		m_strategy(&*m_strategyRefHolder) {
	//...//
}

PyApi::Strategy & StrategyInfoExport::GetStrategy() {
	return *m_strategy;
}

const PyApi::Strategy & StrategyInfoExport::GetStrategy() const {
	return const_cast<StrategyInfoExport *>(this)->GetStrategy();
}

void StrategyInfoExport::ResetRefHolder() throw() {
	Assert(m_strategyRefHolder);
	m_strategyRefHolder.reset();
}

//////////////////////////////////////////////////////////////////////////

StrategyExport::PositionListExport::IteratorExport::IteratorExport(
			const Trader::Strategy::PositionList::Iterator &iterator)
		: iterator_adaptor(iterator) {
	//...//
}

py::object StrategyExport::PositionListExport::IteratorExport::dereference()
		const {
	return PyApi::Export(*base_reference());
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

#ifdef BOOST_WINDOWS
#	pragma warning(push)
#	pragma warning(disable: 4355)
#endif

StrategyExport::StrategyExport(PyObject *self, uintptr_t instanceParam)
		: StrategyInfoExport(
			boost::shared_ptr<PyApi::Strategy>(
				new PyApi::Strategy(instanceParam, *this))),
		Detail::PythonToCoreTransit<StrategyExport>(self),
		m_securityExport(GetStrategy().GetSecurity())  {
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

void StrategyExport::Export(const char *className) {
	
	typedef py::class_<
			StrategyExport,
			py::bases<SecurityAlgoExport>,
			Detail::PythonToCoreTransitHolder<StrategyExport>,
			boost::noncopyable>
		Export;

	const py::scope strategyClass = Export(className, py::init<uintptr_t>())
		.def_readonly("security", &StrategyExport::m_securityExport)
		.add_property("positions", &StrategyExport::GetPositions);

	PositionListExport::Export("PositionList");

}

StrategyExport::PositionListExport StrategyExport::GetPositions() {
	return PositionListExport(GetStrategy().GetPositions());
}

//////////////////////////////////////////////////////////////////////////
