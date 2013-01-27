/**************************************************************************
 *   Created: 2012/08/06 13:10:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Strategy.hpp"
#include "StrategyExport.hpp"
#include "PositionExport.hpp"
#include "ServiceExport.hpp"
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

PyApi::Strategy::Strategy(uintptr_t params, StrategyExport &strategyExport)
		: Trader::Strategy(
			reinterpret_cast<Params *>(params)->name,
			reinterpret_cast<Params *>(params)->tag,
			reinterpret_cast<Params *>(params)->security,
			reinterpret_cast<Params *>(params)->settings),
		m_strategyExport(strategyExport),
		m_script(reinterpret_cast<Params *>(params)->script.release()) {
	DoSettingsUpdate(reinterpret_cast<Params *>(params)->ini);
}

PyApi::Strategy::~Strategy() {
	//...//
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
	try {
		const std::string className
			= py::extract<std::string>(clientClass.attr("__name__"));
		const Params params(
			className,
			tag,
			security,
			ini,
			settings,
			script);
		auto pyObject = clientClass(reinterpret_cast<uintptr_t>(&params));
		StrategyExport &strategyExport
			= py::extract<StrategyExport &>(pyObject);
		return strategyExport.GetStrategy().TakeExportObjectOwnership();
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Failed to create instance of trader.Strategy");
	}
}

boost::shared_ptr<PyApi::Strategy> PyApi::Strategy::TakeExportObjectOwnership() {
	Assert(!m_strategyExportRefHolder);
	m_strategyExportRefHolder = m_strategyExport.shared_from_this();
	m_strategyExport.MoveRefToCore();
	return m_strategyExport.ReleaseRefHolder();
}

StrategyExport & PyApi::Strategy::GetExport() {
	Assert(m_strategyExportRefHolder);
	return m_strategyExport;
}

const StrategyExport & PyApi::Strategy::GetExport() const {
	Assert(m_strategyExportRefHolder);
	return const_cast<PyApi::Strategy *>(this)->GetExport();
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

bool PyApi::Strategy::CallVirtualMethod(
			const char *name,
			const boost::function<void (const py::override &)> &call)
		const {
	return GetExport().CallVirtualMethod(name, call);
}

void PyApi::Strategy::OnServiceStart(const Trader::Service &service) {
	const bool isExists = CallVirtualMethod(
		"onServiceStart",
		[&](const py::override &f) {
			f(PyApi::Export(service));
		});
	if (!isExists) {
		Base::OnServiceStart(service);
	}
}

void PyApi::Strategy::OnLevel1Update() {
	const bool isExists = CallVirtualMethod(
		"onLevel1Update",
		[&](const py::override &f) {
			f();
		});
	if (!isExists) {
		Base::OnLevel1Update();
	}
}

void PyApi::Strategy::OnNewTrade(
			const pt::ptime &time,
			ScaledPrice price,
			Qty qty,
			OrderSide side) {
	const bool isExists = CallVirtualMethod(
		"onNewTrade",
		[&](const py::override &f) {
			f(
				PyApi::Export(time),
				PyApi::Export(price),
				PyApi::Export(qty),
				PyApi::Export(side));
		});
	if (!isExists) {
		Base::OnNewTrade(time, price, qty, side);
	}
}

void PyApi::Strategy::OnServiceDataUpdate(const Trader::Service &service) {
	const bool isExists = CallVirtualMethod(
		"onServiceDataUpdate",
		[&](const py::override &f) {
			f(PyApi::Export(service));
		});
	if (!isExists) {
		Base::OnServiceDataUpdate(service);
	}
}

void PyApi::Strategy::OnPositionUpdate(Position &position) {
	const bool isExists = CallVirtualMethod(
		"onPositionUpdate",
		[&](const py::override &f) {
			f(PyApi::Export(position));
		});
	if (!isExists) {
		Base::OnPositionUpdate(position);
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
