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

StrategyExport::StrategyExport(Trader::Strategy &strategy)
		: SecurityAlgoExport(strategy),
		m_strategy(&strategy),
		m_securityExport(strategy.GetSecurity()) {
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
		.def_readonly("security", &StrategyExport::m_securityExport)
		.add_property("positions", &StrategyExport::GetPositions);

	PositionListExport::Export("PositionList");

}

StrategyExport::PositionListExport StrategyExport::GetPositions() {
	return PositionListExport(GetStrategy().GetPositions());
}

Trader::Strategy & StrategyExport::GetStrategy() {
	return *m_strategy;
}
const Trader::Strategy & StrategyExport::GetStrategy() const {
	return const_cast<StrategyExport *>(this)->GetStrategy();
}

//////////////////////////////////////////////////////////////////////////
