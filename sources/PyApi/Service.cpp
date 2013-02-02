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
#include "Script.hpp"
#include "BaseExport.hpp"

using namespace Trader;
using namespace Trader::Lib;
using namespace Trader::PyApi;
using namespace Trader::PyApi::Detail;

namespace py = boost::python;
namespace pt = boost::posix_time;

namespace {

	struct Params : private boost::noncopyable {

		Context &context;
		const std::string &name;
		const std::string &tag;
		const Security &security;
		const IniFileSectionRef &configuration;
	
		explicit Params(
					Context &context,
					const std::string &name,
					const std::string &tag,
					const Security &security,
					const IniFileSectionRef &configuration)
				: context(context),
				name(name),
				tag(tag),
				security(security),
				configuration(configuration) {
			//...//
		}

	};

}

PyApi::Service::Service(uintptr_t params, ServiceExport &serviceExport)
		: Base(
			reinterpret_cast<Params *>(params)->context,
			reinterpret_cast<Params *>(params)->name,
			reinterpret_cast<Params *>(params)->tag,
			reinterpret_cast<Params *>(params)->security),
		m_serviceExport(serviceExport) {
	DoSettingsUpdate(reinterpret_cast<Params *>(params)->configuration);
}

PyApi::Service::~Service() {
	//...//
}

boost::shared_ptr<PyApi::Service> PyApi::Service::TakeExportObjectOwnership() {
	Assert(!m_serviceExportRefHolder);
	m_serviceExportRefHolder = m_serviceExport.shared_from_this();
	m_serviceExport.MoveRefToCore();
	return m_serviceExport.ReleaseRefHolder();
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
			Context &context,
			const std::string &tag,
			Security &security,
			const IniFileSectionRef &configuration) {
	auto clientClass = Script::Load(configuration).GetClass(
		configuration,
		context,
		"Failed to find trader.Service implementation");
	try {
		const std::string className
			= py::extract<std::string>(clientClass.attr("__name__"));
		const Params params(
			context,
			className,
			tag,
			security,
			configuration);
		auto pyObject = clientClass(reinterpret_cast<uintptr_t>(&params));
		ServiceExport &serviceExport = py::extract<ServiceExport &>(pyObject);
		return serviceExport.GetService().TakeExportObjectOwnership();
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

bool PyApi::Service::CallVirtualMethod(
			const char *name,
			const boost::function<void (const py::override &)> &call)
		const {
	return GetExport().CallVirtualMethod(name, call);
}

void PyApi::Service::OnServiceStart(const Trader::Service &service) {
	const bool isExists = CallVirtualMethod(
		"onServiceStart",
		[&](const py::override &f) {
			f(PyApi::Export(service));
		});
	if (!isExists) {
		Base::OnServiceStart(service);
	}
}

bool PyApi::Service::OnLevel1Update(const Security &security) {
	bool stateChanged = false;
	const bool isExists = CallVirtualMethod(
		"onLevel1Update",
		[&](const py::override &f) {
			stateChanged = f(security);
		});
	if (!isExists) {
		stateChanged = Base::OnLevel1Update(security);
	}
	return stateChanged;
}

bool PyApi::Service::OnNewTrade(
			const Security &security,
			const pt::ptime &time,
			ScaledPrice price,
			Qty qty,
			OrderSide side) {
	bool stateChanged = false;
	const bool isExists = CallVirtualMethod(
		"onNewTrade",
		[&](const py::override &f) {
			stateChanged = f(
				PyApi::Export(security),
				PyApi::Export(time),
				PyApi::Export(price),
				PyApi::Export(qty),
				PyApi::Export(side));
		});
	if (!isExists) {
		stateChanged = Base::OnNewTrade(security, time, price, qty, side);
	}
	return stateChanged;
}

bool PyApi::Service::OnServiceDataUpdate(const Trader::Service &service) {
	bool stateChanged = false;
	const bool isExists = CallVirtualMethod(
		"onServiceDataUpdate",
		[&](const py::override &f) {
			stateChanged = f(PyApi::Export(service));
		});
	if (!isExists) {
		stateChanged = Base::OnServiceDataUpdate(service);
	}
	return stateChanged;
}

//////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
	boost::shared_ptr<Trader::Service> CreateService(
				Context &context,
				const std::string &tag,
				Security &security,
				const IniFileSectionRef &configuration) {
		return PyApi::Service::CreateClientInstance(
			context,
			tag,
			security,
			configuration);
	}
#else
	extern "C" boost::shared_ptr<Trader::Service> CreateService(
				Context &context,
				const std::string &tag,
				Security &security,
				const IniFileSectionRef &configuration) {
		return PyApi::Service::CreateClientInstance(
			context,
			tag,
			security,
			configuration);
	}
#endif

//////////////////////////////////////////////////////////////////////////
