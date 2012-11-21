/**************************************************************************
 *   Created: 2012/08/06 14:51:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Script.hpp"
#include "Import.hpp"
#include "Export.hpp"
#include "Service.hpp"
#include "Strategy.hpp"
#include "Position.hpp"

namespace fs = boost::filesystem;
namespace py = boost::python;

using namespace Trader::Lib;
using namespace Trader::PyApi;

//////////////////////////////////////////////////////////////////////////

namespace {

	void LogInfo(const char *event) {
		Log::Info(event);
	}

	void LogTrading(const char *tag, const char *event) {
		Log::Trading(tag, event);
	}

}

BOOST_PYTHON_MODULE(Trader) {

	using namespace py;
	using namespace Trader::PyApi;

	def("logInfo", &LogInfo);
	def("logTrading", &LogTrading);

	class_<Import::SecurityAlgo, boost::noncopyable>("SecurityAlgo", no_init)
		.add_property(
			"tag",
			&Import::SecurityAlgo::GetTag)
 		.def_readonly(
			"security",
			&Import::SecurityAlgo::security)
 		.def(
			"getName",
			pure_virtual(&Import::SecurityAlgo::CallGetNamePyMethod));

	typedef class_<
			Service,
			bases<Import::SecurityAlgo>,
			boost::shared_ptr<Service>,
			boost::noncopyable>
		ServiceImport;
 	ServiceImport("Service", no_init);

	typedef class_<
			Strategy,
			bases<Import::SecurityAlgo>,
			boost::shared_ptr<Strategy>,
			boost::noncopyable>
		StrategyImport;
	StrategyImport("Strategy", init<uintmax_t>())
		.def(
			"notifyServiceStart",
			&Import::Strategy::CallNotifyServiceStartPyMethod)
		.def(
			"tryToOpenPositions",
			pure_virtual(&Import::Strategy::CallTryToOpenPositionsPyMethod))
		.def(
			"tryToClosePositions",
			pure_virtual(&Import::Strategy::CallTryToClosePositionsPyMethod));

	class_<Export::Security, boost::noncopyable>("Security",  no_init)

		.add_property("symbol", &Export::Security::GetSymbol)
		.add_property("fullSymbol", &Export::Security::GetFullSymbol)
		.add_property("currency", &Export::Security::GetCurrency)

		.add_property("priceScale", &Export::Security::GetPriceScale)
		.def("scalePrice", &Export::Security::ScalePrice)
		.def("descalePrice", &Export::Security::DescalePrice)

		.add_property(
			"lastPriceScaled",
			&Export::Security::GetLastPriceScaled)
		.add_property("lastPrice", &Export::Security::GetLastPrice)
		.add_property("lastSize", &Export::Security::GetLastQty)

		.add_property("askPriceScaled", &Export::Security::GetAskPriceScaled)
		.add_property("askPrice", &Export::Security::GetAskPrice)
		.add_property("askSize", &Export::Security::GetAskQty)

		.add_property("bidPriceScaled", &Export::Security::GetBidPriceScaled)
		.add_property("bidPrice", &Export::Security::GetBidPrice)
		.add_property("bidSize", &Export::Security::GetBidQty)

		.def("cancelOrder", &Export::Security::CancelOrder)
		.def("cancelAllOrders", &Export::Security::CancelAllOrders);

	typedef class_<Import::Position, boost::noncopyable> PositionImport;
	PositionImport("Position", no_init)

		.add_property("type", &Import::Position::GetTypeStr)

		.add_property("hasActiveOrders", &Import::Position::HasActiveOrders)

		.add_property("planedQty", &Import::Position::GetPlanedQty)

		.add_property("openStartPrice", &Import::Position::GetOpenStartPrice)
		.add_property("openOrderId", &Import::Position::GetOpenOrderId)
		.add_property("openedQty", &Import::Position::GetOpenedQty)
		.add_property("openPrice", &Import::Position::GetOpenPrice)

		.add_property("notOpenedQty", &Import::Position::GetNotOpenedQty)
		.add_property("activeQty", &Import::Position::GetActiveQty)

		.add_property("closeOrderId", &Import::Position::GetCloseOrderId)
		.add_property("closeStartPrice", &Import::Position::GetCloseStartPrice)
		.add_property("closePrice", &Import::Position::GetClosePrice)
		.add_property("closedQty", &Import::Position::GetClosedQty)

		.add_property("commission", &Import::Position::GetCommission)

		.def("openAtMarketPrice", &Import::Position::OpenAtMarketPrice)
		.def("open", &Import::Position::Open)
		.def(
			"openAtMarketPriceWithStopPrice",
			&Import::Position::OpenAtMarketPriceWithStopPrice)
		.def("openOrCancel", &Import::Position::OpenOrCancel)

		.def("closeAtMarketPrice", &Import::Position::CloseAtMarketPrice)
		.def("close", &Import::Position::Close)
		.def(
			"closeAtMarketPriceWithStopPrice",
			&Import::Position::CloseAtMarketPriceWithStopPrice)
		.def("closeOrCancel", &Import::Position::CloseOrCancel)

		.def("cancelAtMarketPrice", &Import::Position::CancelAtMarketPrice)
		.def("cancelAllOrders", &Import::Position::CancelAllOrders);

	typedef init<
			Export::Security &,
			int /*qty*/,
			double /*startPrice*/,
			const std::string &>
		PositionImportInit;

	typedef class_<
			LongPosition,
			bases<Import::Position>,
			boost::shared_ptr<LongPosition>,
			boost::noncopyable>
		LongPositionImport;
	LongPositionImport("LongPosition", PositionImportInit());

	typedef class_<
			ShortPosition,
			bases<Import::Position>,
			boost::shared_ptr<ShortPosition>,
			boost::noncopyable>
		ShortPositionImport;
	ShortPositionImport("ShortPosition", PositionImportInit());

	//! @todo Move to detail namespace 
	typedef class_<Export::Service, boost::noncopyable>("CoreService", no_init)
		.add_property("tag", &Export::Service::GetTag)
 		.def_readonly("security", &Export::Service::security)
 		.def("getName", &Export::Service::GetName);

	typedef class_<
			Export::Services::BarService,
			bases<Export::Service>,
			boost::noncopyable>
		BarServiceExport;
	//! @todo Move to Services namespace 
	BarServiceExport("BarService", no_init)
		.add_property("barSize", &Export::Services::BarService::GetBarSize)
		.add_property(
			"barSizeInMinutes",
			&Export::Services::BarService::GetBarSizeInMinutes)
		.add_property(
			"barSizeInHours",
			&Export::Services::BarService::GetBarSizeInHours)
		.add_property(
			"barSizeInDays",
			&Export::Services::BarService::GetBarSizeInDays)
		.add_property("size", &Export::Services::BarService::GetSize)
		.add_property("isEmpty", &Export::Services::BarService::IsEmpty)
 		.def("getBarByIndex", &Export::Services::BarService::GetBarByIndex)
 		.def(
			"getBarByReversedIndex",
			&Export::Services::BarService::GetBarByReversedIndex);

}

namespace {

	void InitPython() {

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;

		static volatile long isInited = false;
		static Mutex mutex;

		if (isInited) {
			return;
		}
		
		const Lock lock(mutex);
		if (isInited) {
			return;
		}
		
		Py_Initialize();
		if (PyImport_AppendInittab("Trader", initTrader) == -1) {
			throw Trader::PyApi::Error(
				"Failed to add trader module to the interpreter's builtin modules");
		}
		Verify(Interlocking::Exchange(isInited, true) == false);

	}

}

//////////////////////////////////////////////////////////////////////////

Script::Script(const boost::filesystem::path &filePath)
		: m_filePath(filePath) {

	InitPython();

	m_main = py::import("__main__");
	m_global = m_main.attr("__dict__");

	try {
		py::exec_file(filePath.string().c_str(), m_global, m_global);
	} catch (const std::exception &ex) {
		throw Error(
			(boost::format("Failed to load script from %1%: \"%2%\"")
					% filePath
					% ex.what())
				.str().c_str());
	} catch (const py::error_already_set &) {
		Detail::RethrowPythonClientException(
			(boost::format("Failed to compile Python script from %1%")
					% filePath)
				.str().c_str());
	}

}

void Script::Exec(const std::string &code) {
	try {
		py::exec(code.c_str(), m_global, m_global);
	} catch (const py::error_already_set &) {
		Detail::RethrowPythonClientException("Failed to execute Python code");
	}
}

//////////////////////////////////////////////////////////////////////////
