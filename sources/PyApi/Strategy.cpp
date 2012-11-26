/**************************************************************************
 *   Created: 2012/08/06 13:10:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Strategy.hpp"
#include "Core/StrategyPositionReporter.hpp"
#include "Core/PositionBundle.hpp"

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
		Import::Strategy(static_cast<Trader::Strategy &>(*this)),
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
		"Failed to find Trader.Strategy implementation");
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
		RethrowPythonClientException(
			"Failed to create instance of Trader.Strategy");
		throw;
	}
}

void Strategy::NotifyServiceStart(const Trader::Service &service) {
	try {
// 		const Service *const pyService
// 			= dynamic_cast<const Service *>(&service);
// 		if (pyService) {
// 			PyNotifyServiceStart(*pyService);
// 		} else {
		py::object object;
		if (dynamic_cast<const Trader::Services::BarService *>(&service)) {
			//! @todo fixme
			auto *serviceExport = new Export::Services::BarService(
				*boost::polymorphic_downcast<
						const Trader::Services::BarService *>(
					&service));
			object = py::object(boost::cref(*serviceExport));
		} else if (dynamic_cast<const Trader::Services::BarStatService *>(&service)) {
			//! @todo fixme
			auto *serviceExport = new Export::Services::BarStatService(
				*boost::polymorphic_downcast<
						const Trader::Services::BarStatService *>(
					&service));
			object = py::object(boost::cref(*serviceExport));
		}
		if (object) {
			CallNotifyServiceStartPyMethod(object);
			m_pyCache[&service] = object;
		} else {
			Trader::Strategy::NotifyServiceStart(service);
		}
	} catch (const py::error_already_set &) {
		RethrowPythonClientException(
			"Failed to call method Trader.Strategy.notifyServiceStart");
	}
}

boost::shared_ptr<PositionBandle> Strategy::TryToOpenPositions() {
	boost::shared_ptr<PositionBandle> result;
	try {
		auto pyPosition = CallTryToOpenPositionsPyMethod();
		if (!pyPosition) {
			return result;
		}
		Import::Position &position = py::extract<Import::Position &>(pyPosition);
		position.Bind(pyPosition);
		result.reset(new PositionBandle);
		result->Get().push_back(position.GetPosition().shared_from_this());
	} catch (const py::error_already_set &) {
		RethrowPythonClientException(
			"Failed to call method Trader.Strategy.tryToOpenPositions");
		throw;
	}
	return result;
}

void Strategy::TryToClosePositions(PositionBandle &positions) {
	foreach (auto p, positions.Get()) {
		if (!p->IsOpened() || p->IsClosed() || p->IsCanceled()) {
 			continue;
 		}
		Assert(dynamic_cast<Import::Position *>(&*p));
		Assert(
			dynamic_cast<LongPosition *>(&*p)
			|| dynamic_cast<ShortPosition *>(&*p));
		CallTryToClosePositionsPyMethod(dynamic_cast<Import::Position &>(*p));
	}
}

void Strategy::ReportDecision(const Trader::Position &position) const {
	Log::Trading(
		GetTag().c_str(),
		"%1% %2% open-try cur-ask-bid=%3%/%4% limit-used=%5% qty=%6%",
		position.GetSecurity().GetSymbol(),
		position.GetTypeStr(),
		position.GetSecurity().GetAskPrice(1),
		position.GetSecurity().GetBidPrice(1),
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

bool Strategy::OnNewTrade(
			const boost::posix_time::ptime &time,
			Trader::ScaledPrice price,
			Trader::Qty qty,
			Trader::OrderSide side) {
	boost::shared_ptr<PositionBandle> result;
	try {
		return CallOnNewTradePyMethod(
			Detail::Time::Convert(time),
			py::object(price),
			py::object(qty),
			Detail::OrderSide::Convert(side));
	} catch (const py::error_already_set &) {
		RethrowPythonClientException(
			"Failed to call method Trader.Strategy.onNewTrade");
		throw;
	}
}

bool Strategy::OnServiceDataUpdate(const Trader::Service &service) {
	Assert(m_pyCache.find(&service) != m_pyCache.end());
	return CallOnServiceDataUpdatePyMethod(m_pyCache[&service]);
}

py::str Strategy::CallGetNamePyMethod() const {
	const auto f = get_override("getName");
	if (f) {
		try {
			return f();
		} catch (const py::error_already_set &) {
			RethrowPythonClientException(
				"Failed to call method Trader.Strategy.getName");
			throw;
		}
	} else {
		return Import::Strategy::CallGetNamePyMethod();
	}
}

void Strategy::CallNotifyServiceStartPyMethod(const py::object &service) {
	Assert(service);
	const auto f = get_override("notifyServiceStart");
	if (f) {
		try {
			f(service);
		} catch (const py::error_already_set &) {
			RethrowPythonClientException(
				"Failed to call method Trader.Strategy.notifyServiceStart");
			throw;
		}
	} else {
		Import::Strategy::CallNotifyServiceStartPyMethod(service);
	}
}

py::object Strategy::CallTryToOpenPositionsPyMethod() {
	const auto f = get_override("tryToOpenPositions");
	if (f) {
		try {
			return f();
		} catch (const py::error_already_set &) {
			RethrowPythonClientException(
				"Failed to call method Trader.Strategy.tryToOpenPositions");
			throw;
		}
	} else {
		return Import::Strategy::CallTryToOpenPositionsPyMethod();
	}
}

void Strategy::CallTryToClosePositionsPyMethod(const py::object &positions) {
	Assert(positions);
	const auto f = get_override("tryToClosePositions");
	if (f) {
		try {
			f(positions);
		} catch (const py::error_already_set &) {
			RethrowPythonClientException(
				"Failed to call method Trader.Strategy.tryToClosePositions");
			throw;
		}
	} else {
		Import::Strategy::CallTryToClosePositionsPyMethod(positions);
	}
}

bool Strategy::CallOnNewTradePyMethod(
			const boost::python::object &time,
			const boost::python::object &price,
			const boost::python::object &qty,
			const boost::python::object &side) {
	Assert(time);
	Assert(price);
	Assert(qty);
	Assert(side);
	const auto f = get_override("onNewTrade");
	if (f) {
		try {
			return f(time, price, qty, side);
		} catch (const py::error_already_set &) {
			RethrowPythonClientException(
				"Failed to call method Trader.Strategy.onNewTrade");
			throw;
		}
	} else {
		return Import::Strategy::CallOnNewTradePyMethod(time, price, qty, side);
	}
}

bool Strategy::CallOnServiceDataUpdatePyMethod(
			const boost::python::object &service) {
	Assert(service);
	const auto f = get_override("onServiceDataUpdate");
	if (f) {
		try {
			return f(service);
		} catch (const py::error_already_set &) {
			RethrowPythonClientException(
				"Failed to call method Trader.Strategy.onServiceDataUpdate");
			throw;
		}
	} else {
		return Import::Strategy::CallOnServiceDataUpdatePyMethod(service);
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
