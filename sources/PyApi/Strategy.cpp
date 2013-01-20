/**************************************************************************
 *   Created: 2012/08/06 13:10:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "PositionExport.hpp"
#include "Position.hpp"
#include "ServiceExport.hpp"
#include "Strategy.hpp"
#include "BaseExport.hpp"
#include "Core/StrategyPositionReporter.hpp"

namespace py = boost::python;
namespace pt = boost::posix_time;

using namespace Trader;
using namespace Trader::Lib;
using namespace Trader::PyApi;
using namespace Trader::PyApi::Detail;

namespace {

	struct Params : private boost::noncopyable {
		
		const std::string &name;
		const std::string &tag;
		const boost::shared_ptr<Security> &security;
		const IniFileSectionRef &ini;
		const boost::shared_ptr<const Settings> &settings;
		std::unique_ptr<Script> &script;
	
		explicit Params(
					const std::string &name,
					const std::string &tag,
					const boost::shared_ptr<Security> &security,
					const IniFileSectionRef &ini,
					const boost::shared_ptr<const Settings> &settings,
					std::unique_ptr<Script> &script)
				: name(name),
				tag(tag),
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

PyApi::Strategy::Strategy(uintmax_t params)
		: Trader::Strategy(
			reinterpret_cast<Params *>(params)->name,
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


PyApi::Strategy::~Strategy() {
	AssertFail("Strategy::~Strategy");
}

boost::shared_ptr<Trader::Strategy> PyApi::Strategy::CreateClientInstance(
			const std::string &tag,
			boost::shared_ptr<Security> security,
			const IniFileSectionRef &ini,
			boost::shared_ptr<const Settings> settings) {
	std::unique_ptr<Script> script(LoadScript(ini));
	auto clientClass = GetPyClass(
		*script,
		ini,
		"Failed to find trader.Strategy implementation");
	const std::string className
		= py::extract<std::string>(clientClass.attr("__name__"));
	const Params params(
		className,
		tag,
		security,
		ini,
		settings,
		script);
	try {
		auto pyObject = clientClass(reinterpret_cast<uintmax_t>(&params));
		Strategy &strategy = py::extract<Strategy &>(pyObject);
		strategy.m_self = pyObject;
		return strategy.shared_from_this();
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Failed to create instance of trader.Strategy");
	}
}

void PyApi::Strategy::OnServiceStart(const Trader::Service &service) {
	const auto f = get_override("onServiceStart");
	if (!f) {
		Trader::Strategy::OnServiceStart(service);
		return;
	}
	try {
		f(PyApi::Export(service));
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Failed to call method trader.Strategy.onServiceStart");
	}
}

namespace {

	template<typename PyPosition>
	void MovePositionRefToCore(Position &position) throw() {
		PyPosition *const pyPosition = dynamic_cast<PyPosition *>(&position);
		if (!pyPosition) {
			return;
		}
		pyPosition->TakeExportObjectOwnership();
	}

}

void PyApi::Strategy::Register(Position &position) {
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
	void MovePositionRefToPython(Position &position) throw() {
		PyPosition *const pyPosition = dynamic_cast<PyPosition *>(&position);
		if (!pyPosition) {
			return;
		}
		pyPosition->GetExport().MoveRefToPython();
	}

}

void PyApi::Strategy::Unregister(Position &position) throw() {
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

void PyApi::Strategy::ReportDecision(const Position &position) const {
	GetLog().Trading(
		"%1% %2% open-try cur-ask-bid=%3%/%4% limit-used=%5% qty=%6%",
		boost::make_tuple(
			boost::cref(position.GetSecurity().GetSymbol()),
			boost::cref(position.GetTypeStr()),
			position.GetSecurity().GetAskPrice(),
			position.GetSecurity().GetBidPrice(),
			position.GetSecurity().DescalePrice(position.GetOpenStartPrice()),
			position.GetPlanedQty()));
}

std::auto_ptr<PositionReporter> PyApi::Strategy::CreatePositionReporter() const {
	typedef StrategyPositionReporter<Strategy> Reporter;
	std::auto_ptr<Reporter> result(new Reporter);
	result->Init(*this);
	return std::auto_ptr<PositionReporter>(result);
}

void PyApi::Strategy::UpdateAlogImplSettings(const IniFileSectionRef &ini) {
	DoSettingsUpdate(ini);
}

void PyApi::Strategy::DoSettingsUpdate(const IniFileSectionRef &ini) {
	UpdateAlgoSettings(*this, ini);
}

void PyApi::Strategy::OnLevel1Update() {
	const auto f = get_override("onLevel1Update");
	if (!f) {
		Trader::Strategy::OnLevel1Update();
		return;
	}
	try {
		f();
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Failed to call method trader.Strategy.onLevel1Update");
	}
}

void PyApi::Strategy::OnNewTrade(
			const pt::ptime &time,
			ScaledPrice price,
			Qty qty,
			OrderSide side) {
	const auto f = get_override("onNewTrade");
	if (!f) {
		Trader::Strategy::OnNewTrade(time, price, qty, side);
		return;
	}
	try {
		f(
			PyApi::Export(time),
			PyApi::Export(price),
			PyApi::Export(qty),
			PyApi::Export(side));
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Failed to call method trader.Strategy.onNewTrade");
	}
}

void PyApi::Strategy::OnServiceDataUpdate(const Trader::Service &service) {
	const auto f = get_override("onServiceDataUpdate");
	if (!f) {
		Trader::Strategy::OnServiceDataUpdate(service);
		return;
	}
	try {
		f(PyApi::Export(service));
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error(
			"Failed to call method trader.Strategy.onServiceDataUpdate");
	}
}

void PyApi::Strategy::OnPositionUpdate(Position &position) {
	const auto f = get_override("onPositionUpdate");
	if (!f) {
		Trader::Strategy::OnPositionUpdate(position);
		return;
	}
	try {
		f(PyApi::Export(position));
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error(
			"Failed to call method trader.Strategy.onPositionUpdate");
	}
}

//////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
	boost::shared_ptr<Trader::Strategy> CreateStrategy(
				const std::string &tag,
				boost::shared_ptr<Security> security,
				const IniFileSectionRef &ini,
				boost::shared_ptr<const Settings> settings) {
		return PyApi::Strategy::CreateClientInstance(
			tag,
			security,
			ini,
			settings);
	}
#else
	extern "C" boost::shared_ptr<Trader::Strategy> CreateStrategy(
				const std::string &tag,
				boost::shared_ptr<Security> security,
				const IniFileSectionRef &ini,
				boost::shared_ptr<const Settings> settings) {
		return PyApi::Strategy::CreateClientInstance(
			tag,
			security,
			ini,
			settings);
	}
#endif

//////////////////////////////////////////////////////////////////////////
