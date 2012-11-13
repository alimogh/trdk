/**************************************************************************
 *   Created: 2012/08/06 14:51:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Script.hpp"
#include "Service.hpp"
#include "Strategy.hpp"
#include "Position.hpp"
#include "SecurityWrapper.hpp"

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

	class_<Wrappers::SecurityAlgo, boost::noncopyable>("SecurityAlgo", no_init)
		.add_property("tag", &Wrappers::SecurityAlgo::GetTag)
 		.def_readonly("security", &Wrappers::SecurityAlgo::security)
 		.def("getName", pure_virtual(&Wrappers::SecurityAlgo::PyGetName));

	typedef class_<
			Service,
			bases<Wrappers::SecurityAlgo>,
			boost::shared_ptr<Service>,
			boost::noncopyable>
		PyService;
 	PyService("Service", init<uintmax_t>());

	typedef class_<
			Strategy,
			bases<Wrappers::SecurityAlgo>,
			boost::shared_ptr<Strategy>,
			boost::noncopyable>
		PyStrategy;
	PyStrategy("Strategy", init<uintmax_t>())
		.def("notifyServiceStart", &Wrappers::Strategy::PyNotifyServiceStart)
		.def("tryToOpenPositions", pure_virtual(&Wrappers::Strategy::PyTryToOpenPositions))
		.def("tryToClosePositions", pure_virtual(&Wrappers::Strategy::PyTryToClosePositions));

	class_<Wrappers::Security, boost::noncopyable>("Security",  no_init)

		.add_property("symbol", &Wrappers::Security::GetSymbol)
		.add_property("fullSymbol", &Wrappers::Security::GetFullSymbol)
		.add_property("currency", &Wrappers::Security::GetCurrency)

		.add_property("priceScale", &Wrappers::Security::GetPriceScale)
		.def("scalePrice", &Wrappers::Security::ScalePrice)
		.def("descalePrice", &Wrappers::Security::DescalePrice)

		.add_property("lastPriceScaled", &Wrappers::Security::GetLastPriceScaled)
		.add_property("lastPrice", &Wrappers::Security::GetLastPrice)
		.add_property("lastSize", &Wrappers::Security::GetLastQty)

		.add_property("askPriceScaled", &Wrappers::Security::GetAskPriceScaled)
		.add_property("askPrice", &Wrappers::Security::GetAskPrice)
		.add_property("askSize", &Wrappers::Security::GetAskQty)

		.add_property("bidPriceScaled", &Wrappers::Security::GetBidPriceScaled)
		.add_property("bidPrice", &Wrappers::Security::GetBidPrice)
		.add_property("bidSize", &Wrappers::Security::GetBidQty)

		.def("cancelOrder", &Wrappers::Security::CancelOrder)
		.def("cancelAllOrders", &Wrappers::Security::CancelAllOrders);

	typedef class_<Wrappers::Position, boost::noncopyable> PyPosition;
	PyPosition("Position", no_init)

		.add_property("type", &Wrappers::Position::GetTypeStr)

		.add_property("hasActiveOrders", &Wrappers::Position::HasActiveOrders)

		.add_property("planedQty", &Wrappers::Position::GetPlanedQty)

		.add_property("openStartPrice", &Wrappers::Position::GetOpenStartPrice)
		.add_property("openOrderId", &Wrappers::Position::GetOpenOrderId)
		.add_property("openedQty", &Wrappers::Position::GetOpenedQty)
		.add_property("openPrice", &Wrappers::Position::GetOpenPrice)

		.add_property("notOpenedQty", &Wrappers::Position::GetNotOpenedQty)
		.add_property("activeQty", &Wrappers::Position::GetActiveQty)

		.add_property("closeOrderId", &Wrappers::Position::GetCloseOrderId)
		.add_property("closeStartPrice", &Wrappers::Position::GetCloseStartPrice)
		.add_property("closePrice", &Wrappers::Position::GetClosePrice)
		.add_property("closedQty", &Wrappers::Position::GetClosedQty)

		.add_property("commission", &Wrappers::Position::GetCommission)

		.def("openAtMarketPrice", &Wrappers::Position::OpenAtMarketPrice)
		.def("open", &Wrappers::Position::Open)
		.def("openAtMarketPriceWithStopPrice", &Wrappers::Position::OpenAtMarketPriceWithStopPrice)
		.def("openOrCancel", &Wrappers::Position::OpenOrCancel)

		.def("closeAtMarketPrice", &Wrappers::Position::CloseAtMarketPrice)
		.def("close", &Wrappers::Position::Close)
		.def("closeAtMarketPriceWithStopPrice", &Wrappers::Position::CloseAtMarketPriceWithStopPrice)
		.def("closeOrCancel", &Wrappers::Position::CloseOrCancel)

		.def("cancelAtMarketPrice", &Wrappers::Position::CancelAtMarketPrice)
		.def("cancelAllOrders", &Wrappers::Position::CancelAllOrders);

	typedef init<
			Wrappers::Security &,
			int /*qty*/,
			double /*startPrice*/,
			const std::string &>
		PyPositionInit;

	typedef class_<
			LongPosition,
			bases<Wrappers::Position>,
			boost::shared_ptr<LongPosition>,
			boost::noncopyable>
		PyLongPosition;
	PyLongPosition("LongPosition", PyPositionInit());

	typedef class_<
			ShortPosition,
			bases<Wrappers::Position>,
			boost::shared_ptr<ShortPosition>,
			boost::noncopyable>
		PyShortPosition;
	PyShortPosition("ShortPosition", PyPositionInit());

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
