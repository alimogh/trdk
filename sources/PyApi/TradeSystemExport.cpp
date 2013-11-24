/**************************************************************************
 *   Created: 2013/11/24 19:27:25
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TradeSystemExport.hpp"
#include "Core/TradeSystem.hpp"

namespace py = boost::python;
using namespace trdk;
using namespace trdk::PyApi;

void TradeSystemExport::ExportClass(const char *className) {
	typedef py::class_<TradeSystemExport> Export;
	Export(className, py::no_init)
		.add_property("cashBalance", &TradeSystemExport::GetCashBalance);
}

double TradeSystemExport::GetCashBalance() const {
	return m_tradeSystem->GetCashBalance();
}
