/**************************************************************************
 *   Created: 2012/08/06 14:51:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "ScriptEngine.hpp"
#include "AlgoWrapper.hpp"

namespace fs = boost::filesystem;
namespace python = boost::python;

using namespace Trader;
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

	python::def("logInfo", &LogInfo);
	python::def("logTrading", &LogTrading);

	python::class_<Wrappers::AlgoWrap, boost::noncopyable>("Algo")

		.def_readonly("security", &Wrappers::AlgoWrap::security)

		.def("tryToOpenPositions", python::pure_virtual(&Wrappers::Algo::TryToOpenPositions))
		.def("tryToClosePositions", python::pure_virtual(&Wrappers::Algo::TryToClosePositions));


	python::class_<Wrappers::Security, boost::noncopyable>("Security",  python::no_init)

		.add_property("symbol", &Wrappers::Security::GetSymbol)
		.add_property("fullSymbol", &Wrappers::Security::GetFullSymbol)
		.add_property("currency", &Wrappers::Security::GetCurrency)

		.add_property("priceScale", &Wrappers::Security::GetPriceScale)
		.def("scalePrice", &Wrappers::Security::ScalePrice)
		.def("descalePrice", &Wrappers::Security::DescalePrice)

		.add_property("lastPriceScaled", &Wrappers::Security::GetLastPriceScaled)
		.add_property("lastPrice", &Wrappers::Security::GetLastPrice)
		.add_property("lastSize", &Wrappers::Security::GetLastSize)

		.add_property("askPriceScaled", &Wrappers::Security::GetAskPriceScaled)
		.add_property("askPrice", &Wrappers::Security::GetAskPrice)
		.add_property("askSize", &Wrappers::Security::GetAskSize)

		.add_property("bidPriceScaled", &Wrappers::Security::GetBidPriceScaled)
		.add_property("bidPrice", &Wrappers::Security::GetBidPrice)
		.add_property("bidSize", &Wrappers::Security::GetBidSize)

		.def("cancelOrder", &Wrappers::Security::CancelOrder)
		.def("cancelAllOrders", &Wrappers::Security::CancelAllOrders);

	python::class_<Wrappers::LongPosition, boost::noncopyable> (
			"LongPosition",
			python::init<PyApi::Wrappers::Security &, int /*qty*/, double /*startPrice*/>())

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

		.def("openAtMarketPrice", &Wrappers::LongPosition::OpenAtMarketPrice)
		.def("open", &Wrappers::LongPosition::Open)
		.def("openAtMarketPriceWithStopPrice", &Wrappers::LongPosition::OpenAtMarketPriceWithStopPrice)
		.def("openOrCancel", &Wrappers::LongPosition::OpenOrCancel)

		.def("closeAtMarketPrice", &Wrappers::LongPosition::CloseAtMarketPrice)
		.def("close", &Wrappers::LongPosition::Close)
		.def("closeAtMarketPriceWithStopPrice", &Wrappers::LongPosition::CloseAtMarketPriceWithStopPrice)
		.def("closeOrCancel", &Wrappers::LongPosition::CloseOrCancel)

		.def("cancelAtMarketPrice", &Wrappers::LongPosition::CancelAtMarketPrice)
		.def("cancelAllOrders", &Wrappers::LongPosition::CancelAllOrders);

	python::class_<Wrappers::ShortPosition, boost::noncopyable>(
			"ShortPosition",
			python::init<PyApi::Wrappers::Security &, int /*qty*/, double /*startPrice*/>())

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

		.def("openAtMarketPrice", &Wrappers::ShortPosition::OpenAtMarketPrice)
		.def("open", &Wrappers::ShortPosition::Open)
		.def("openAtMarketPriceWithStopPrice", &Wrappers::ShortPosition::OpenAtMarketPriceWithStopPrice)
		.def("openOrCancel", &Wrappers::ShortPosition::OpenOrCancel)

		.def("closeAtMarketPrice", &Wrappers::ShortPosition::CloseAtMarketPrice)
		.def("close", &Wrappers::ShortPosition::Close)
		.def("closeAtMarketPriceWithStopPrice", &Wrappers::ShortPosition::CloseAtMarketPriceWithStopPrice)
		.def("closeOrCancel", &Wrappers::ShortPosition::CloseOrCancel)

		.def("cancelAtMarketPrice", &Wrappers::ShortPosition::CancelAtMarketPrice)
		.def("cancelAllOrders", &Wrappers::ShortPosition::CancelAllOrders);

}

namespace {

	void InitPython() {

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;

		static volatile long isInited = false;
		static Mutex mutex;

		if (!isInited) {
			const Lock lock(mutex);
			if (!isInited) {
				Py_Initialize();
				if (PyImport_AppendInittab("Trader", initTrader) == -1) {
					throw ScriptEngine::Error(
						"Failed to add trader module to the interpreter's builtin modules");
				}
				Verify(Interlocking::Exchange(isInited, true) == false);
			}
		}

	}

}

//////////////////////////////////////////////////////////////////////////

ScriptEngine::Error::Error(const char *what) throw()
		: Exception(what) {
	//...//
}

ScriptEngine::ExecError::ExecError(const char *what) throw()
		: Error(what) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

ScriptEngine::ScriptEngine(
			const boost::filesystem::path &filePath,
			const std::string &stamp,
			const std::string &algoClassName,
			const ::Algo &algo,
			boost::shared_ptr<Security> security)
		: m_filePath(filePath),
		m_stamp(stamp) {

	InitPython();

	m_main = python::import("__main__");
	m_global = m_main.attr("__dict__");

	try {
		python::exec_file(filePath.string().c_str(), m_global, m_global);
	} catch (const std::exception &ex) {
		throw Exception(
			(boost::format("Failed to load Python script from %1%: \"%2%\"")
					% filePath
					% ex.what())
				.str().c_str());
	} catch (const python::error_already_set &) {
		PyErr_Print();
		throw Exception(
			(boost::format("Failed to compile Python script from %1%")
					% filePath)
				.str().c_str());
	}

	try {

		m_algoPy = m_global[algoClassName]();
		{
			PyApi::Wrappers::Algo &algo = python::extract<PyApi::Wrappers::Algo &>(m_algoPy);
			m_algo = &algo;
		}

		m_algo->security.Init(algo, security);

	} catch (const python::error_already_set &) {
		PyErr_Print();
		throw Exception(
			(boost::format("Failed to create Python algo object \"%1%\"")
					% algoClassName)
				.str().c_str());
	}

}

void ScriptEngine::Exec(const std::string &code) {
	try {
		python::exec(code.c_str(), m_global, m_global);
	} catch (const python::error_already_set &) {
		PyErr_Print();
		throw Exception("Failed to exec Python code");
	}
}

boost::shared_ptr< ::Position> ScriptEngine::TryToOpenPositions() {
	try {
		python::object result = m_algo->TryToOpenPositions();
		if (!result) {
			return boost::shared_ptr< ::Position>();
		}
		{
			python::extract<PyApi::Wrappers::ShortPosition &> shortPos(result);
			if (shortPos.check()) {
				PyApi::Wrappers::ShortPosition &ref = shortPos;
				return ref.GetPosition();
			}
		}
		{
			python::extract<PyApi::Wrappers::LongPosition &> longPos(result);
			if (longPos.check()) {
				PyApi::Wrappers::LongPosition &ref = longPos;
				return ref.GetPosition();
			}
		}
		throw Exception("Object with unknown type returned by Algo::TryToOpenPositions");
	} catch (const python::error_already_set &) {
		PyErr_Print();
		throw Exception("Failed to call Python object method Algo::TryToOpenPositions");
	}
}

void ScriptEngine::TryToClosePositions(boost::shared_ptr<Position> position) {
	std::unique_ptr<Wrappers::LongPosition> longPos;
	std::unique_ptr<Wrappers::ShortPosition> shortPos;
	python::object pyPosition;
	switch (position->GetType()) {
		case Position::TYPE_LONG:
			longPos.reset(new Wrappers::LongPosition(position));
			pyPosition = python::object(boost::cref(*longPos));
			break;
		case Position::TYPE_SHORT:
			shortPos.reset(new Wrappers::ShortPosition(position));
			pyPosition = python::object(boost::cref(*shortPos));
			break;
		default:
			AssertFail("Unknown position type.");
			return;
	}
	try {
		m_algo->TryToClosePositions(pyPosition);
	} catch (const python::error_already_set &) {
		PyErr_Print();
		throw Exception("Failed to call Python object method Algo::TryToClosePositions");
	}
}
