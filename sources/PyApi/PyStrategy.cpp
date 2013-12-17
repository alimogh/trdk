/**************************************************************************
 *   Created: 2012/08/06 13:10:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "PyStrategy.hpp"
#include "StrategyExport.hpp"
#include "PositionExport.hpp"
#include "ServiceExport.hpp"
#include "Script.hpp"
#include "BaseExport.hpp"
#include "Core/StrategyPositionReporter.hpp"

namespace py = boost::python;
namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::PyApi;
using namespace trdk::PyApi::Detail;

namespace {

	struct Params : private boost::noncopyable {

		Context &context;
		const std::string &name;
		const std::string &tag;
		const IniSectionRef &configuration;
	
		explicit Params(
					Context &context,
					const std::string &name,
					const std::string &tag,
					const IniSectionRef &configuration)
				: context(context),
				name(name),
				tag(tag),
				configuration(configuration) {
			//...//
		}

	};

}

PyApi::Strategy::Strategy(uintptr_t params, StrategyExport &strategyExport)
		: trdk::Strategy(
			reinterpret_cast<Params *>(params)->context,
			reinterpret_cast<Params *>(params)->name,
			reinterpret_cast<Params *>(params)->tag),
		m_strategyExport(strategyExport) {
	DoSettingsUpdate(reinterpret_cast<Params *>(params)->configuration);
}

PyApi::Strategy::~Strategy() {
	//...//
}

boost::shared_ptr<trdk::Strategy> PyApi::Strategy::CreateClientInstance(
			Context &context,
			const std::string &tag,
			const IniSectionRef &configuration) {
	auto clientClass = Script::Load(configuration).GetClass(
		configuration,
		context,
		"Failed to find trdk.Strategy implementation");
	try {
		const std::string className
			= py::extract<std::string>(clientClass.attr("__name__"));
		const Params params(
			context,
			className,
			tag,
			configuration);
		auto pyObject = clientClass(reinterpret_cast<uintptr_t>(&params));
		StrategyExport &strategyExport
			= py::extract<StrategyExport &>(pyObject);
		return strategyExport.GetStrategy().TakeExportObjectOwnership();
	} catch (const py::error_already_set &) {
		throw GetPythonClientException(
			"Failed to create instance of trdk.Strategy");
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
	trdk::Strategy::Register(position);
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
	trdk::Strategy::Unregister(position);
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
	GetContext().GetLog().Trading(
		GetTag(),
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

void PyApi::Strategy::UpdateAlogImplSettings(const IniSectionRef &ini) {
	DoSettingsUpdate(ini);
}

void PyApi::Strategy::DoSettingsUpdate(const IniSectionRef &ini) {
	UpdateAlgoSettings(*this, ini);
}

bool PyApi::Strategy::CallVirtualMethod(
			const char *name,
			const boost::function<void (const py::override &)> &call)
		const {
	return GetExport().CallVirtualMethod(name, call);
}

std::string PyApi::Strategy::GetRequiredSuppliers() const {
	
	std::string result = Base::GetRequiredSuppliers();

	CallVirtualMethod(
		"getRequiredSuppliers",
		[&](const py::override &f) {
			const py::str callResult = f();
			const std::string scriptRequest
				= py::extract<std::string>(callResult);
			if (!result.empty() && !scriptRequest.empty()) {
				result.push_back(',');
			}
			result += scriptRequest;
		});

	return result;

}

pt::ptime PyApi::Strategy::OnSecurityStart(trdk::Security &security) {
	const bool isExists = CallVirtualMethod(
		"onSecurityStart",
		[&](const py::override &f) {
			f(PyApi::Export(security));
		});
	if (isExists) {
		return pt::not_a_date_time;
	} else{
		return Base::OnSecurityStart(security);
	}
}

void PyApi::Strategy::OnServiceStart(const trdk::Service &service) {
	const bool isExists = CallVirtualMethod(
		"onServiceStart",
		[&](const py::override &f) {
			f(PyApi::Export(service));
		});
	if (!isExists) {
		Base::OnServiceStart(service);
	}
}

void PyApi::Strategy::OnLevel1Update(Security &security) {
	const bool isExists = CallVirtualMethod(
		"onLevel1Update",
		[&](const py::override &f) {
			f(PyApi::Export(security));
		});
	if (!isExists) {
		Base::OnLevel1Update(security);
	}
}

void PyApi::Strategy::OnNewTrade(
			Security &security,
			const pt::ptime &time,
			ScaledPrice price,
			Qty qty,
			OrderSide side) {
	const bool isExists = CallVirtualMethod(
		"onNewTrade",
		[&](const py::override &f) {
			f(
				PyApi::Export(security),
				PyApi::Export(time),
				PyApi::Export(price),
				PyApi::Export(qty),
				PyApi::Export(side));
		});
	if (!isExists) {
		Base::OnNewTrade(security, time, price, qty, side);
	}
}

void PyApi::Strategy::OnServiceDataUpdate(const trdk::Service &service) {
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

void PyApi::Strategy::OnBrokerPositionUpdate(
			Security &security,
			Qty qty,
			bool isInitial) {
	const bool isExists = CallVirtualMethod(
		"onBrokerPositionUpdate",
		[&](const py::override &f) {
			f(
				PyApi::Export(security),
				PyApi::Export(qty),
				PyApi::Export(isInitial));
		});
	if (!isExists) {
		Base::OnBrokerPositionUpdate(security, qty, isInitial);
	}
}

//////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
	boost::shared_ptr<trdk::Strategy> CreateStrategy(
				Context &context,
				const std::string &tag,
				const IniSectionRef &configuration) {
		return PyApi::Strategy::CreateClientInstance(
			context,
			tag,
			configuration);
	}
#else
	extern "C" boost::shared_ptr<trdk::Strategy> CreateStrategy(
				Context &context,
				const std::string &tag,
				const IniSectionRef &configuration) {
		return PyApi::Strategy::CreateClientInstance(
			context,
			tag,
			configuration);
	}
#endif

//////////////////////////////////////////////////////////////////////////
