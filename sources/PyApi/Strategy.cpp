/**************************************************************************
 *   Created: 2012/08/06 13:10:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Position.hpp"
#include "Strategy.hpp"
#include "Import.hpp"
#include "Core/StrategyPositionReporter.hpp"

using namespace Trader::Lib;
using namespace Trader::PyApi;
using namespace Trader::PyApi::Detail;

namespace py = boost::python;

namespace {

	struct Params : private boost::noncopyable {
		
		const std::string &tag;
		boost::shared_ptr<Trader::Security> &security;
		const Trader::Lib::IniFileSectionRef &ini;
		boost::shared_ptr<const Trader::Settings> &settings;
		std::unique_ptr<Script> &script;
	
		explicit Params(
					const std::string &tag,
					boost::shared_ptr<Trader::Security> &security,
					const Trader::Lib::IniFileSectionRef &ini,
					boost::shared_ptr<const Trader::Settings> &settings,
					std::unique_ptr<Script> &script)
				: tag(tag),
				security(security),
				ini(ini),
				settings(settings),
				script(script) {
			//...//
		}

	};

}

#ifdef BOOST_WINDOWS
#	pragma warning(push)
#	pragma warning(disable: 4355)
#endif

Strategy::Strategy(uintmax_t params)
		: Trader::Strategy(
			reinterpret_cast<Params *>(params)->tag,
			reinterpret_cast<Params *>(params)->security,
			reinterpret_cast<Params *>(params)->settings),
		StrategyExport(static_cast<Trader::Strategy &>(*this)),
		m_script(reinterpret_cast<Params *>(params)->script.release()) {
	DoSettingsUpdate(reinterpret_cast<Params *>(params)->ini);
}

#ifdef BOOST_WINDOWS
#	pragma warning(pop)
#endif

Strategy::~Strategy() {
	AssertFail("Strategy::~Strategy");
}

boost::shared_ptr<Trader::Strategy> Strategy::CreateClientInstance(
			const std::string &tag,
			boost::shared_ptr<Trader::Security> security,
			const Trader::Lib::IniFileSectionRef &ini,
			boost::shared_ptr<const Trader::Settings> settings) {
	std::unique_ptr<Script> script(LoadScript(ini));
	auto clientClass = GetPyClass(
		*script,
		ini,
		"Failed to find trader.Strategy implementation");
	const Params params(tag, security, ini, settings, script);
	try {
		auto pyObject = clientClass(reinterpret_cast<uintmax_t>(&params));
		Strategy &strategy = py::extract<Strategy &>(pyObject);
		strategy.m_self = pyObject;
		const py::str namePyObject = strategy.CallGetNamePyMethod();
		strategy.m_name = py::extract<std::string>(namePyObject);
		Assert(!strategy.m_name.empty());
		return strategy.shared_from_this();
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Trader::PyApi::Error(
			"Failed to create instance of trader.Strategy");
	}
}

void Strategy::OnServiceStart(const Trader::Service &service) {
	try {
// 		const Service *const pyService
// 			= dynamic_cast<const Service *>(&service);
// 		if (pyService) {
// 			PyOnServiceStart(*pyService);
// 		} else {
		py::object object;
		if (dynamic_cast<const Trader::Services::BarService *>(&service)) {
			//! @todo fixme
			auto *serviceExport = new Export::Services::BarService(
				*boost::polymorphic_downcast<
						const Trader::Services::BarService *>(
					&service));
			object = py::object(boost::cref(*serviceExport));
		}
		if (object) {
			CallOnServiceStartPyMethod(object);
			m_pyCache[&service] = object;
		} else {
			Trader::Strategy::OnServiceStart(service);
		}
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Trader::PyApi::Error(
			"Failed to call method trader.Strategy.onServiceStart");
	}
}

namespace {

	template<typename PyPosition>
	void MovePositionRefToCore(Trader::Position &position) throw() {
		PyPosition *const pyPosition = dynamic_cast<PyPosition *>(&position);
		if (!pyPosition) {
			return;
		}
		pyPosition->MoveRefToCore();
	}

}

void Strategy::Register(Trader::Position &position) {
	Trader::Strategy::Register(position);
	static_assert(
		Position::numberOfTypes == 2,
		"Position type list changed.");
	switch (position.GetType()) {
 		case Position::TYPE_LONG:
			MovePositionRefToCore<LongPosition>(position);
			break;
 		case Position::TYPE_SHORT:
 			MovePositionRefToCore<ShortPosition>(position);
			break;
		default:
			AssertNe(int(Position::numberOfTypes), int(position.GetType()));
			break;
	}
}

namespace {

	template<typename PyPosition>
	void MovePositionRefToPython(Trader::Position &position) throw() {
		PyPosition *const pyPosition = dynamic_cast<PyPosition *>(&position);
		if (!pyPosition) {
			return;
		}
		pyPosition->MoveRefToPython();
	}

}

void Strategy::Unregister(Trader::Position &position) throw() {
	Trader::Strategy::Unregister(position);
	static_assert(
		Position::numberOfTypes == 2,
		"Position type list changed.");
	switch (position.GetType()) {
 		case Position::TYPE_LONG:
			MovePositionRefToPython<LongPosition>(position);
			break;
 		case Position::TYPE_SHORT:
 			MovePositionRefToPython<ShortPosition>(position);
			break;
		default:
			AssertNe(int(Position::numberOfTypes), int(position.GetType()));
			break;
	}
}

void Strategy::ReportDecision(const Trader::Position &position) const {
	Log::Trading(
		GetTag().c_str(),
		"%1% %2% open-try cur-ask-bid=%3%/%4% limit-used=%5% qty=%6%",
		position.GetSecurity().GetSymbol(),
		position.GetTypeStr(),
		position.GetSecurity().GetAskPrice(),
		position.GetSecurity().GetBidPrice(),
		position.GetSecurity().DescalePrice(position.GetOpenStartPrice()),
		position.GetPlanedQty());
}

std::auto_ptr<Trader::PositionReporter> Strategy::CreatePositionReporter() const {
	typedef Trader::StrategyPositionReporter<Strategy> Reporter;
	std::auto_ptr<Reporter> result(new Reporter);
	result->Init(*this);
	return std::auto_ptr<Trader::PositionReporter>(result);
}

void Strategy::UpdateAlogImplSettings(const IniFileSectionRef &ini) {
	DoSettingsUpdate(ini);
}

void Strategy::DoSettingsUpdate(const IniFileSectionRef &ini) {
	UpdateAlgoSettings(*this, ini);
}

void Strategy::OnLevel1Update() {
	CallOnLevel1UpdatePyMethod();
}

void Strategy::OnNewTrade(
			const boost::posix_time::ptime &time,
			Trader::ScaledPrice price,
			Trader::Qty qty,
			Trader::OrderSide side) {
	try {
		return CallOnNewTradePyMethod(
			Detail::Time::Convert(time),
			py::object(price),
			py::object(qty),
			Detail::OrderSide::Convert(side));
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Trader::PyApi::Error(
			"Failed to call method trader.Strategy.onNewTrade");
	}
}

void Strategy::OnServiceDataUpdate(const Trader::Service &service) {
	Assert(m_pyCache.find(&service) != m_pyCache.end());
	CallOnServiceDataUpdatePyMethod(m_pyCache[&service]);
}

void Strategy::OnPositionUpdate(Trader::Position &position) {
	Assert(m_pyCache.find(&position) != m_pyCache.end());
	CallOnPositionUpdatePyMethod(m_pyCache[&position]);
}

py::str Strategy::CallGetNamePyMethod() const {
	const auto f = get_override("getName");
	if (f) {
		try {
			return f();
		} catch (const py::error_already_set &) {
			LogPythonClientException();
			throw Trader::PyApi::Error(
				"Failed to call method trader.Strategy.getName");
		}
	} else {
		return StrategyExport::CallGetNamePyMethod();
	}
}

void Strategy::CallOnServiceStartPyMethod(const py::object &service) {
	Assert(service);
	const auto f = get_override("onServiceStart");
	if (f) {
		try {
			f(service);
		} catch (const py::error_already_set &) {
			LogPythonClientException();
			throw Trader::PyApi::Error(
				"Failed to call method trader.Strategy.onServiceStart");
		}
	} else {
		StrategyExport::CallOnServiceStartPyMethod(service);
	}
}

void Strategy::CallOnLevel1UpdatePyMethod() {
	const auto f = get_override("onLevel1Update");
	if (f) {
		try {
			f();
		} catch (const py::error_already_set &) {
			LogPythonClientException();
			throw Trader::PyApi::Error(
				"Failed to call method trader.Strategy.onLevel1Update");
		}
	} else {
		StrategyExport::CallOnLevel1UpdatePyMethod();
	}
}

void Strategy::CallOnNewTradePyMethod(
			const py::object &time,
			const py::object &price,
			const py::object &qty,
			const py::object &side) {
	Assert(time);
	Assert(price);
	Assert(qty);
	Assert(side);
	const auto f = get_override("onNewTrade");
	if (f) {
		try {
			f(time, price, qty, side);
		} catch (const py::error_already_set &) {
			LogPythonClientException();
			throw Trader::PyApi::Error(
				"Failed to call method trader.Strategy.onNewTrade");
		}
	} else {
		StrategyExport::CallOnNewTradePyMethod(time, price, qty, side);
	}
}

void Strategy::CallOnServiceDataUpdatePyMethod(
			const py::object &service) {
	Assert(service);
	const auto f = get_override("onServiceDataUpdate");
	if (f) {
		try {
			f(service);
		} catch (const py::error_already_set &) {
			LogPythonClientException();
			throw Trader::PyApi::Error(
				"Failed to call method trader.Strategy.onServiceDataUpdate");
		}
	} else {
		StrategyExport::CallOnServiceDataUpdatePyMethod(service);
	}
}

void Strategy::CallOnPositionUpdatePyMethod(
				const py::object &position) {
	Assert(position);
	const auto f = get_override("onPositionUpdate");
	if (f) {
		try {
			f(position);
		} catch (const py::error_already_set &) {
			LogPythonClientException();
			throw Trader::PyApi::Error(
				"Failed to call method trader.Strategy.onPositionUpdate");
		}
	} else {
		StrategyExport::CallOnPositionUpdatePyMethod(position);
	}
}

//////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
	boost::shared_ptr<Trader::Strategy> CreateStrategy(
				const std::string &tag,
				boost::shared_ptr<Trader::Security> security,
				const IniFileSectionRef &ini,
				boost::shared_ptr<const Trader::Settings> settings) {
		return Strategy::CreateClientInstance(tag, security, ini, settings);
	}
#else
	extern "C" boost::shared_ptr<Trader::Strategy> CreateStrategy(
				const std::string &tag,
				boost::shared_ptr<Trader::Security> security,
				const IniFileSectionRef &ini,
				boost::shared_ptr<const Trader::Settings> settings) {
		return Strategy::CreateClientInstance(tag, security, ini, settings);
	}
#endif

//////////////////////////////////////////////////////////////////////////
