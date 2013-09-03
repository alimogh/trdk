/**************************************************************************
 *   Created: 2012/11/05 14:03:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Service.hpp"
#include "ServiceExport.hpp"
#include "Script.hpp"
#include "BaseExport.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::PyApi;
using namespace trdk::PyApi::Detail;

namespace py = boost::python;
namespace pt = boost::posix_time;

namespace {

	struct Params : private boost::noncopyable {

		Context &context;
		const std::string &name;
		const std::string &tag;
		const IniFileSectionRef &configuration;
	
		explicit Params(
					Context &context,
					const std::string &name,
					const std::string &tag,
					const IniFileSectionRef &configuration)
				: context(context),
				name(name),
				tag(tag),
				configuration(configuration) {
			//...//
		}

	};

}

PyApi::Service::Service(uintptr_t params, ServiceExport &serviceExport)
		: Base(
			reinterpret_cast<Params *>(params)->context,
			reinterpret_cast<Params *>(params)->name,
			reinterpret_cast<Params *>(params)->tag),
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

boost::shared_ptr<trdk::Service> PyApi::Service::CreateClientInstance(
			Context &context,
			const std::string &tag,
			const IniFileSectionRef &configuration) {
	auto clientClass = Script::Load(configuration).GetClass(
		configuration,
		context,
		"Failed to find trdk.Service implementation");
	try {
		const std::string className
			= py::extract<std::string>(clientClass.attr("__name__"));
		const Params params(
			context,
			className,
			tag,
			configuration);
		auto pyObject = clientClass(reinterpret_cast<uintptr_t>(&params));
		ServiceExport &serviceExport = py::extract<ServiceExport &>(pyObject);
		return serviceExport.GetService().TakeExportObjectOwnership();
	} catch (const py::error_already_set &) {
		throw GetPythonClientException(
			"Failed to create instance of trdk.Service");
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

std::string PyApi::Service::GetRequiredSuppliers() const {
	
	std::string result = Base::GetRequiredSuppliers();

	CallVirtualMethod(
		"getRequiredSuppliers",
		[&](const py::override &f) {
			const py::str &callResult = f();
			const std::string &scriptRequest
				= py::extract<std::string>(callResult);
			if (!result.empty() && !scriptRequest.empty()) {
				result.push_back(',');
			}
			result += scriptRequest;
		});

	return result;

}

pt::ptime PyApi::Service::OnSecurityStart(const trdk::Security &security) {
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

void PyApi::Service::OnServiceStart(const trdk::Service &service) {
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
	bool isStateChanged = false;
	const bool isExists = CallVirtualMethod(
		"onLevel1Update",
		[&](const py::override &f) {
			isStateChanged = f(security);
		});
	if (!isExists) {
		isStateChanged = Base::OnLevel1Update(security);
	}
	return isStateChanged;
}

bool PyApi::Service::OnNewTrade(
			const Security &security,
			const pt::ptime &time,
			ScaledPrice price,
			Qty qty,
			OrderSide side) {
	bool isStateChanged = false;
	const bool isExists = CallVirtualMethod(
		"onNewTrade",
		[&](const py::override &f) {
			isStateChanged = f(
				PyApi::Export(security),
				PyApi::Export(time),
				PyApi::Export(price),
				PyApi::Export(qty),
				PyApi::Export(side));
		});
	if (!isExists) {
		isStateChanged = Base::OnNewTrade(security, time, price, qty, side);
	}
	return isStateChanged;
}

bool PyApi::Service::OnServiceDataUpdate(const trdk::Service &service) {
	bool isStateChanged = false;
	const bool isExists = CallVirtualMethod(
		"onServiceDataUpdate",
		[&](const py::override &f) {
			isStateChanged = f(PyApi::Export(service));
		});
	if (!isExists) {
		isStateChanged = Base::OnServiceDataUpdate(service);
	}
	return isStateChanged;
}

bool PyApi::Service::OnBrokerPositionUpdate(
			Security &security,
			Qty qty,
			bool isInitial) {
	bool isStateChanged = false;
	const bool isExists = CallVirtualMethod(
		"onBrokerPositionUpdate",
		[&](const py::override &f) {
			isStateChanged = f(
				PyApi::Export(security),
				PyApi::Export(qty),
				PyApi::Export(isInitial));
		});
	if (!isExists) {
		Base::OnBrokerPositionUpdate(security, qty, isInitial);
	}
	return isStateChanged;
}

//////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
	boost::shared_ptr<trdk::Service> CreateService(
				Context &context,
				const std::string &tag,
				const IniFileSectionRef &configuration) {
		return PyApi::Service::CreateClientInstance(
			context,
			tag,
			configuration);
	}
#else
	extern "C" boost::shared_ptr<trdk::Service> CreateService(
				Context &context,
				const std::string &tag,
				const IniFileSectionRef &configuration) {
		return PyApi::Service::CreateClientInstance(
			context,
			tag,
			security,
			configuration);
	}
#endif

//////////////////////////////////////////////////////////////////////////
