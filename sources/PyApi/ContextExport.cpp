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

void ContextExport::ExportClass(const char *className) {
	typedef py::class_<ContextExport> Export;
	Export(className, py::no_init)
		.add_property("tradeSystem", &ContextExport::GetTradeSystem);
}

TradeSystemExport ContextExport::GetTradeSystem() const {
	return TradeSystemExport(m_context->GetTradeSystem());
}
