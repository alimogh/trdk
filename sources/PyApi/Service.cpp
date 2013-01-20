/**************************************************************************
 *   Created: 2012/11/05 14:03:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Service.hpp"
#include "ServiceExport.hpp"
#include "BaseExport.hpp"

using namespace Trader;
using namespace Trader::Lib;
using namespace Trader::PyApi;
using namespace Trader::PyApi::Detail;

namespace py = boost::python;
namespace pt = boost::posix_time;

namespace {

	struct Params : private boost::noncopyable {
		
		const std::string &name;
		const std::string &tag;
		const boost::shared_ptr<Security> &security;
		const IniFileSectionRef &ini;
		const boost::shared_ptr<const Settings> &settings;
	
		explicit Params(
					const std::string &name,
					const std::string &tag,
					const boost::shared_ptr<Security> &security,
					const IniFileSectionRef &ini,
					const boost::shared_ptr<const Settings> &settings)
				: name(name),
				tag(tag),
				security(security),
				ini(ini),
				settings(settings) {
			//...//
		}

	};

}

PyApi::Service::Service(uintptr_t params, ServiceExport &serviceExport)
		: Base(
			reinterpret_cast<Params *>(params)->name,
			reinterpret_cast<Params *>(params)->tag,
			reinterpret_cast<Params *>(params)->security,
			reinterpret_cast<Params *>(params)->settings),
		m_serviceExport(serviceExport) {
	DoSettingsUpdate(reinterpret_cast<Params *>(params)->ini);
}

PyApi::Service::~Service() {
	AssertFail("Service::~Service");
}

void PyApi::Service::TakeExportObjectOwnership() {
	Assert(!m_serviceExportRefHolder);
	m_serviceExportRefHolder = m_serviceExport.shared_from_this();
	m_serviceExport.MoveRefToCore();
	m_serviceExport.ResetRefHolder();
}

ServiceExport & PyApi::Service::GetExport() {
	Assert(m_serviceExportRefHolder);
	return m_serviceExport;
}

const ServiceExport & PyApi::Service::GetExport() const {
	Assert(m_serviceExportRefHolder);
	return const_cast<PyApi::Service *>(this)->GetExport();
}

boost::shared_ptr<Trader::Service> PyApi::Service::CreateClientInstance(
			const std::string &tag,
			boost::shared_ptr<Security> security,
			const IniFileSectionRef &ini,
			const boost::shared_ptr<const Settings> &settings) {
	std::unique_ptr<Script> script(LoadScript(ini));
	auto clientClass = GetPyClass(
		*script,
		ini,
		"Failed to find trader.Service implementation");
	try {
		const std::string className
			= py::extract<std::string>(clientClass.attr("__name__"));
		const Params params(
			className,
			tag,
			security,
			ini,
			settings);
		auto pyObject = clientClass(reinterpret_cast<uintptr_t>(&params));
		ServiceExport &service = py::extract<ServiceExport &>(pyObject);
		service.MoveRefToCore();
		return service.GetService().shared_from_this();
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Failed to create instance of trader.Service");
	}
}

void PyApi::Service::UpdateAlogImplSettings(const IniFileSectionRef &ini) {
	DoSettingsUpdate(ini);
}

void PyApi::Service::DoSettingsUpdate(const IniFileSectionRef &ini) {
	UpdateAlgoSettings(*this, ini);
}

py::override PyApi::Service::GetOverride(const char *) const {
	return GetExport().GetOverride("onServiceStart");
}

void PyApi::Service::OnServiceStart(const Trader::Service &service) {
	const auto f = GetOverride("onServiceStart");
	if (!f) {
		Base::OnServiceStart(service);
		return;
	}
	try {
		f(PyApi::Export(service));
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Failed to call method trader.Service.onServiceStart");
	}
}

bool PyApi::Service::OnLevel1Update() {
	const auto f = GetOverride("onLevel1Update");
	if (!f) {
		return Base::OnLevel1Update();
	}
	try {
		return f();
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Failed to call method trader.Service.onLevel1Update");
	}
}

bool PyApi::Service::OnNewTrade(
			const pt::ptime &time,
			ScaledPrice price,
			Qty qty,
			OrderSide side) {
	const auto f = GetOverride("onNewTrade");
	if (!f) {
		return Base::OnNewTrade(time, price, qty, side);
	}
	try {
		return f(
			PyApi::Export(time),
			PyApi::Export(price),
			PyApi::Export(qty),
			PyApi::Export(side));
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Failed to call method trader.Service.onNewTrade");
	}
}

bool PyApi::Service::OnServiceDataUpdate(const Trader::Service &service) {
	const auto f = GetOverride("onServiceDataUpdate");
	if (!f) {
		return Base::OnServiceDataUpdate(service);
	}
	try {
		return f(PyApi::Export(service));
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Failed to call method trader.Service.onServiceDataUpdate");
	}
}

//////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
	boost::shared_ptr<Trader::Service> CreateService(
				const std::string &tag,
				boost::shared_ptr<Security> security,
				const IniFileSectionRef &ini,
				boost::shared_ptr<const Settings> settings) {
		return PyApi::Service::CreateClientInstance(
			tag,
			security,
			ini,
			settings);
	}
#else
	extern "C" boost::shared_ptr<Trader::Service> CreateService(
				const std::string &tag,
				boost::shared_ptr<Security> security,
				const IniFileSectionRef &ini,
				boost::shared_ptr<const Settings>) {
		return PyApi::Service::CreateClientInstance(
			tag,
			security,
			ini,
			settings);
	}
#endif

//////////////////////////////////////////////////////////////////////////
