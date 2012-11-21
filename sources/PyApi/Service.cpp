/**************************************************************************
 *   Created: 2012/11/05 14:03:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Service.hpp"

using namespace Trader::Lib;
using namespace Trader::PyApi;
using namespace Trader::PyApi::Detail;

namespace py = boost::python;

namespace {

	struct Params : private boost::noncopyable {
		
		const std::string &tag;
		boost::shared_ptr<Trader::Security> &security;
		const Trader::Lib::IniFileSectionRef &ini;
		std::unique_ptr<Script> &script;
	
		explicit Params(
					const std::string &tag,
					boost::shared_ptr<Trader::Security> &security,
					const Trader::Lib::IniFileSectionRef &ini,
					std::unique_ptr<Script> &script)
				: tag(tag),
				security(security),
				ini(ini),
				script(script) {
			//...//
		}

	};

}

#ifdef BOOST_WINDOWS
#	pragma warning(push)
#	pragma warning(disable: 4355)
#endif

Service::Service(uintmax_t params)
		: Trader::Service(
			reinterpret_cast<Params *>(params)->tag,
			reinterpret_cast<Params *>(params)->security),
		Wrappers::Service(static_cast<Trader::Service &>(*this)),
		m_script(reinterpret_cast<Params *>(params)->script.release()) {
	DoSettingsUpdate(reinterpret_cast<Params *>(params)->ini);
}

#ifdef BOOST_WINDOWS
#	pragma warning(pop)
#endif

Service::~Service() {
	AssertFail("Service::~Service");
}

boost::shared_ptr<Trader::Service> Service::CreateClientInstance(
			const std::string &tag,
			boost::shared_ptr<Trader::Security> security,
			const Trader::Lib::IniFileSectionRef &ini) {
	std::unique_ptr<Script> script(LoadScript(ini));
	auto clientClass = GetPyClass(
		*script,
		ini,
		"Failed to find Trader.Service implementation");
	const Params params(tag, security, ini, script);
	try {
		auto pyObject = clientClass(reinterpret_cast<uintmax_t>(&params));
		Service &service = py::extract<Service &>(pyObject);
		service.m_self = pyObject;
		const py::str namePyObject = service.PyGetName();
		service.m_name = py::extract<std::string>(namePyObject);
		Assert(!service.m_name.empty());
		return service.shared_from_this();
	} catch (const boost::python::error_already_set &) {
		RethrowPythonClientException(
			"Failed to create instance of Trader.Service");
		throw;
	}
}

void Service::UpdateAlogImplSettings(const IniFileSectionRef &ini) {
	DoSettingsUpdate(ini);
}

void Service::DoSettingsUpdate(const IniFileSectionRef &ini) {
	Detail::UpdateAlgoSettings(*this, ini);
}

void Service::NotifyServiceStart(const Trader::Service &service) {
	try {
		Assert(dynamic_cast<const Service *>(&service));
		PyNotifyServiceStart(dynamic_cast<const Service &>(service));
	} catch (const py::error_already_set &) {
		RethrowPythonClientException(
			"Failed to call method Trader.Service.notifyServiceStart");
	}
}

py::str Service::PyGetName() const {
	const auto f = get_override("getName");
	if (f) {
		try {
			return f();
		} catch (const py::error_already_set &) {
			RethrowPythonClientException(
				"Failed to call method Trader.Service.getName");
			throw;
		}
	} else {
		return Wrappers::Service::PyGetName();
	}
}

void Service::PyNotifyServiceStart(py::object service) {
	Assert(service);
	const auto f = get_override("notifyServiceStart");
	if (f) {
		try {
			f(service);
		} catch (const py::error_already_set &) {
			RethrowPythonClientException(
				"Failed to call method Trader.Service.notifyServiceStart");
			throw;
		}
	} else {
		Wrappers::Service::PyNotifyServiceStart(service);
	}
}

//////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
	boost::shared_ptr<Trader::Service> CreateService(
				const std::string &tag,
				boost::shared_ptr<Trader::Security> security,
				const IniFileSectionRef &ini,
				boost::shared_ptr<const Trader::Settings>) {
		return Service::CreateClientInstance(tag, security, ini);
	}
#else
	extern "C" boost::shared_ptr<Trader::Service> CreateService(
				const std::string &tag,
				boost::shared_ptr<Trader::Security> security,
				const IniFileSectionRef &ini,
				boost::shared_ptr<const Trader::Settings>) {
		return Service::CreateClientInstance(tag, security, ini);
	}
#endif

//////////////////////////////////////////////////////////////////////////
