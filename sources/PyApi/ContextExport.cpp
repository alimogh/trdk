/**************************************************************************
 *   Created: 2013/11/24 19:17:18
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "ContextExport.hpp"
#include "TradeSystemExport.hpp"
#include "Core/Context.hpp"

namespace py = boost::python;
using namespace trdk;
using namespace trdk::PyApi;

////////////////////////////////////////////////////////////////////////////////

ContextExport::ParamsExport::ParamsExport(Context &context)
		: m_context(&context) {
	//...//
}

void ContextExport::ParamsExport::ExportClass(const char *className) {
	typedef py::class_<ParamsExport> Export;
	Export(className, py::no_init)
		.def("__getattr__", &ParamsExport::GetAttr)
		.def("__setattr__", &ParamsExport::SetAttr)
		.def("getRevision", &ParamsExport::GetRevision);
}

std::string ContextExport::ParamsExport::GetAttr(
			const std::string &keyName)
		const {
	//! @todo not optimal python check "hassattr by exception"
	return m_context->GetParams()[keyName];
}

void ContextExport::ParamsExport::SetAttr(
			const std::string &key,
			const std::string &value) {
	return m_context->GetParams().Update(key, value);
}

Context::Params::Revision ContextExport::ParamsExport::GetRevision() const {
	return m_context->GetParams().GetRevision();
}

////////////////////////////////////////////////////////////////////////////////

void ContextExport::ExportClass(const char *className) {
	typedef py::class_<ContextExport> Export;
	const auto contextScope
		= Export(className, py::no_init)
			.add_property("params", &ContextExport::GetParams)
			.add_property("tradeSystem", &ContextExport::GetTradeSystem);
	ParamsExport::ExportClass("Params");
}

ContextExport::ParamsExport ContextExport::GetParams() {
	return ParamsExport(*m_context);
}

TradeSystemExport ContextExport::GetTradeSystem() const {
	return TradeSystemExport(m_context->GetTradeSystem());
}

////////////////////////////////////////////////////////////////////////////////
