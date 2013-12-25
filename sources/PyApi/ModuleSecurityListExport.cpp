/**************************************************************************
 *   Created: 2013/05/31 08:48:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "ModuleSecurityListExport.hpp"
#include "SecurityExport.hpp"
#include "Core/Consumer.hpp"
#include "Core/Settings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::PyApi;
namespace py = boost::python;

////////////////////////////////////////////////////////////////////////////////

template<typename Module, typename IteratorImplementation>
ModuleSecurityListExport<Module, IteratorImplementation>::IteratorExport::IteratorExport(
			const IteratorImplementation &iterator)
		: iterator_adaptor(iterator) {
	//...//
}

template<typename Module, typename IteratorImplementation>
py::object
ModuleSecurityListExport<Module, IteratorImplementation>::IteratorExport::dereference()
		const {
	return PyApi::Export(*base_reference());
}

template<typename Module, typename IteratorImplementation>
ModuleSecurityListExport<Module, IteratorImplementation>::ModuleSecurityListExport(
			Module &module)
		: m_module(&module) {
	//...//
}

template<typename Module, typename IteratorImplementation>
size_t ModuleSecurityListExport<Module, IteratorImplementation>::GetSize()
		const {
	return m_module->GetSecurities().GetSize();
}

template<typename Module, typename IteratorImplementation>
typename ModuleSecurityListExport<Module, IteratorImplementation>::iterator
ModuleSecurityListExport<Module, IteratorImplementation>::begin() {
	return IteratorExport(m_module->GetSecurities().GetBegin());
}

template<typename Module, typename IteratorImplementation>
typename ModuleSecurityListExport<Module, IteratorImplementation>::iterator
ModuleSecurityListExport<Module, IteratorImplementation>::end() {
	return IteratorExport(m_module->GetSecurities().GetEnd());
}

template<typename Module, typename IteratorImplementation>
py::object
ModuleSecurityListExport<Module, IteratorImplementation>::FindBySymbol(
			const py::str &symbol) {
	const auto pos = m_module->GetSecurities().Find(
		Symbol(
			py::extract<std::string>(symbol),
			m_module->GetContext().GetSettings().GetDefaultExchange(),
			m_module->GetContext().GetSettings().GetDefaultPrimaryExchange(),
			m_module->GetContext().GetSettings().GetDefaultCurrency()));
	if (pos == m_module->GetSecurities().GetEnd()) {
		return py::object();
	}
	return PyApi::Export(*pos);
}

template<typename Module, typename IteratorImplementation>
py::object
ModuleSecurityListExport<Module, IteratorImplementation>::FindBySymbolAndExchange(
			const py::str &symbol,
			const py::str &exchange) {
	const auto pos = m_module->GetSecurities().Find(
		Symbol(
			py::extract<std::string>(symbol),
			py::extract<std::string>(exchange),
			m_module->GetContext().GetSettings().GetDefaultPrimaryExchange(),
			m_module->GetContext().GetSettings().GetDefaultCurrency()));
	if (pos == m_module->GetSecurities().GetEnd()) {
		return py::object();
	}
	return PyApi::Export(*pos);
}

template<typename Module, typename IteratorImplementation>
py::object
ModuleSecurityListExport<Module, IteratorImplementation>::FindBySymbolAndPrimaryExchange(
			const py::str &symbol,
			const py::str &exchange,
			const py::str &primaryExchange) {
	const auto pos = m_module->GetSecurities().Find(
		Symbol(
			py::extract<std::string>(symbol),
			py::extract<std::string>(exchange),
			py::extract<std::string>(primaryExchange),
			m_module->GetContext().GetSettings().GetDefaultCurrency()));
	if (pos == m_module->GetSecurities().GetEnd()) {
		return py::object();
	}
	return PyApi::Export(*pos);
}

////////////////////////////////////////////////////////////////////////////////

namespace {

	template<typename List>
	void ExportListClass(const char *className) {
		py::class_<List>(className, py::no_init)
			.def("__iter__", py::iterator<List>())
			.def("count", &List::GetSize)
			.def("find", &List::FindBySymbol)
			.def("find", &List::FindBySymbolAndExchange)
			.def("find", &List::FindBySymbolAndPrimaryExchange);
	}

}

////////////////////////////////////////////////////////////////////////////////

ConsumerSecurityListExport::ConsumerSecurityListExport(Consumer &module)
		: ModuleSecurityListExport(module) {
	//...//
}

void ConsumerSecurityListExport::ExportClass(const char *className) {
	ExportListClass<ConsumerSecurityListExport>(className);
}

////////////////////////////////////////////////////////////////////////////////

ServiceSecurityListExport::ServiceSecurityListExport(
			const trdk::Service &module) 
		: ModuleSecurityListExport(module) {
	//...//
}

void ServiceSecurityListExport::ExportClass(const char *className) {
	ExportListClass<ServiceSecurityListExport>(className);
}

////////////////////////////////////////////////////////////////////////////////

namespace {
	template<typename T>
	void InstantiateTemplateClassMethods() {
		T::Module *const module = nullptr;
		T t(*module);
		t.GetSize();
		t.begin();
		t.end();
	}
	void InstantiateConsumerClassMethods() {
		InstantiateTemplateClassMethods<ConsumerSecurityListExport>();
	}
	void InstantiateServiceClassMethods() {
		InstantiateTemplateClassMethods<ServiceSecurityListExport>();
	}
}

////////////////////////////////////////////////////////////////////////////////
