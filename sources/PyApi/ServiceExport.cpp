/**************************************************************************
 *   Created: 2013/01/15 13:56:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "ServiceExport.hpp"
#include "Service.hpp"
#include "BaseExport.hpp"
#include "ObjectCache.hpp"

namespace py = boost::python;
namespace pt = boost::posix_time;

using namespace Trader;
using namespace Trader::Services;
using namespace Trader::PyApi;
using namespace Trader::PyApi::Detail;

//////////////////////////////////////////////////////////////////////////

const Trader::Service & PyApi::ExtractService(const py::object &service) {
	try {
		Assert(service);
		ServiceExport &serviceExport = py::extract<ServiceExport &>(service);
		return serviceExport.GetService();
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Failed to extract service");
	}
}

namespace {

	ObjectCache<const Trader::Service> objectCache;

}

py::object PyApi::Export(const Trader::Service &service) {
	auto *const pyService = dynamic_cast<const PyApi::Service *>(&service);
	if (pyService) {
		return pyService->GetExport().GetSelf();
	}
	{
		auto cached = objectCache.Find(service);
		if (cached) {
			return cached;
		}
	}
	try {
		if (dynamic_cast<const BarServiceExport::Implementation *>(&service)) {
			return objectCache.Get<BarServiceExport>(service);
		}
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Failed to export service");
	}
	throw Error("Failed to export service: Unknown service type");
}

//////////////////////////////////////////////////////////////////////////

ServiceInfoExport::ServiceInfoExport(const Trader::Service &service)
		: SecurityAlgoExport(service),
		m_service(&service),
		m_securityExport(GetService().GetSecurity()) {
	//...//
}

ServiceInfoExport::ServiceInfoExport(
			const boost::shared_ptr<const Trader::Service> &service)
		: SecurityAlgoExport(*service),
		m_serviceRefHolder(service),
		m_service(&*m_serviceRefHolder),
		m_securityExport(GetService().GetSecurity()) {
	//...//
}

void ServiceInfoExport::Export(const char *className) {
	typedef py::class_<
			ServiceInfoExport,
			py::bases<SecurityAlgoExport>>
		Export;
	Export(className, py::no_init)
		.def_readonly("security", &ServiceInfoExport::m_securityExport);
}

void ServiceInfoExport::ResetRefHolder() throw() {
	Assert(m_serviceRefHolder);
	m_serviceRefHolder.reset();
}

//////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
#	pragma warning(push)
#	pragma warning(disable: 4355)
#endif

ServiceExport::ServiceExport(PyObject *self, uintptr_t instanceParam)
		: ServiceInfoExport(
			boost::shared_ptr<const Trader::Service>(
				new PyApi::Service(instanceParam, *this))),
		Detail::PythonToCoreTransit<ServiceExport>(self) {
	//...//
}

#ifdef BOOST_WINDOWS
#	pragma warning(pop)
#endif

namespace boost { namespace python {

	template<>
	struct has_back_reference<ServiceExport> : public mpl::true_ {
		//...//
	};

} }

void ServiceExport::Export(const char *className) {
	typedef py::class_<
			ServiceExport,
			py::bases<ServiceInfoExport>,
			boost::noncopyable>
		Export;
	Export(className, py::init<boost::uintmax_t>());
}

Trader::Service & ServiceExport::GetService() {
	return const_cast<Trader::Service &>(ServiceInfoExport::GetService());
}

py::override ServiceExport::GetOverride(const char *name) const {
	return get_override(name);
}

//////////////////////////////////////////////////////////////////////////

BarServiceExport::BarExport::BarExport(
			const BarService &service,
			const BarService::Bar &bar)
		: m_service(&service),
		m_bar(&bar) {
	//...//
}

void BarServiceExport::BarExport::Export(const char *className) {
	py::class_<BarExport>(className, py::no_init)
		.add_property("time", &BarExport::GetTime)
		.add_property("size", &BarExport::GetSize)
		.add_property("openPrice", &BarExport::GetOpenPrice)
		.add_property("closePrice", &BarExport::GetClosePrice)
		.add_property("highPrice", &BarExport::GetHighPrice)
		.add_property("lowPrice", &BarExport::GetLowPrice)
		.add_property("volume", &BarExport::GetVolume);
}

time_t BarServiceExport::BarExport::GetTime() const {
	return Util::ConvertToTimeT(m_bar->time);
}

size_t BarServiceExport::BarExport::GetSize() const {
	return m_service->GetBarSize().total_seconds();
}

boost::intmax_t BarServiceExport::BarExport::GetOpenPrice() const {
	return m_bar->openPrice;
}

boost::intmax_t BarServiceExport::BarExport::GetClosePrice() const {
	return m_bar->closePrice;
}
			
boost::intmax_t BarServiceExport::BarExport::GetHighPrice() const {
	return m_bar->highPrice;
}

boost::intmax_t BarServiceExport::BarExport::GetLowPrice() const {
	return m_bar->lowPrice;
}
			
boost::intmax_t BarServiceExport::BarExport::GetVolume() const {
	return m_bar->volume;
}

void BarServiceExport::StatExport::Export(const char *className) {
	py::class_<StatExport>(className, py::no_init);
}
        
BarServiceExport::PriceStatExport::PriceStatExport(
			const boost::shared_ptr<const BarService::ScaledPriceStat> &stat)
		: m_stat(stat) {
	//...//
}

void BarServiceExport::PriceStatExport::Export(const char *className) {
	typedef py::class_<
			PriceStatExport,
			py::bases<StatExport>>
		Export;
	Export(className, py::no_init)
		.add_property("max", &PriceStatExport::GetMax)
		.add_property("min", &PriceStatExport::GetMin);
}

BarServiceExport::PriceStatExport::ValueType
BarServiceExport::PriceStatExport::GetMax()
		const {
	return m_stat->GetMax();
}

BarServiceExport::PriceStatExport::ValueType
BarServiceExport::PriceStatExport::GetMin()
		const {
	return m_stat->GetMin();
}

BarServiceExport::QtyStatExport::QtyStatExport(
			const boost::shared_ptr<const BarService::QtyStat> &stat)
		: m_stat(stat) {
	//...//
}

void BarServiceExport::QtyStatExport::Export(const char *className) {
	typedef py::class_<
			QtyStatExport,
			py::bases<StatExport>>
		Export;
	Export(className, py::no_init)
		.add_property("max", &QtyStatExport::GetMax)
		.add_property("min", &QtyStatExport::GetMin);
}

BarServiceExport::QtyStatExport::ValueType
BarServiceExport::QtyStatExport::GetMax()
		const {
	return m_stat->GetMax();
}

BarServiceExport::QtyStatExport::ValueType
BarServiceExport::QtyStatExport::GetMin()
		const {
	return m_stat->GetMin();
}

BarServiceExport::BarServiceExport(const BarService &barService)
		: ServiceInfoExport(barService) {
	//...//
}

void BarServiceExport::Export(const char *className) {

	typedef py::class_<
			BarServiceExport,
			py::bases<ServiceInfoExport>>
		Export;

	const py::scope barServiceClass = Export(className, py::no_init)
					
		.add_property("barSize", &BarServiceExport::GetBarSize)
		.add_property("size", &BarServiceExport::GetSize)
		.add_property("isEmpty", &BarServiceExport::IsEmpty)
 					
		.def("getBarByIndex", &BarServiceExport::GetBarByIndex)
 		.def("getBarByReversedIndex", &BarServiceExport::GetBarByReversedIndex)

 		.def("getOpenPriceStat", &BarServiceExport::GetOpenPriceStat)
 		.def("getClosePriceStat", &BarServiceExport::GetClosePriceStat)
 		.def("getHighPriceStat", &BarServiceExport::GetHighPriceStat)
 		.def("getHighPriceStat", &BarServiceExport::GetHighPriceStat)
 		.def("getVolumeStat", &BarServiceExport::GetVolumeStat);

	BarExport::Export("Bar");

	StatExport::Export("Stat");
	PriceStatExport::Export("PriceStat");
	QtyStatExport::Export("QtyStat");

}

py::str BarServiceExport::GetName() const {
	return GetService().GetName().c_str();
}
		
size_t BarServiceExport::GetBarSize() const {
	return GetService().GetBarSize().total_seconds();
}
		
size_t BarServiceExport::GetSize() const {
	return GetService().GetSize();
}
		
bool BarServiceExport::IsEmpty() const {
	return GetService().IsEmpty();
}
 		
BarServiceExport::BarExport BarServiceExport::GetBarByIndex(
			size_t index)
		const {
	return BarExport(GetService(), GetService().GetBar(index));
}
 		
BarServiceExport::BarExport BarServiceExport::GetBarByReversedIndex(
			size_t index)
		const {
	return BarExport(GetService(), GetService().GetBarByReversedIndex(index));
}

BarServiceExport::PriceStatExport BarServiceExport::GetOpenPriceStat(
			size_t numberOfBars)
		const {
	return PriceStatExport(GetService().GetOpenPriceStat(numberOfBars));
}
        
BarServiceExport::PriceStatExport BarServiceExport::GetClosePriceStat(
			size_t numberOfBars)
		const {
	return PriceStatExport(GetService().GetClosePriceStat(numberOfBars));
}
        
BarServiceExport::PriceStatExport BarServiceExport::GetHighPriceStat(
			size_t numberOfBars)
		const {
	return PriceStatExport(GetService().GetHighPriceStat(numberOfBars));
}
        
BarServiceExport::PriceStatExport BarServiceExport::GetLowPriceStat(
			size_t numberOfBars)
		const {
	return PriceStatExport(GetService().GetLowPriceStat(numberOfBars));
}
        
BarServiceExport::QtyStatExport BarServiceExport::GetVolumeStat(
			size_t numberOfBars)
		const {
	return QtyStatExport(GetService().GetVolumeStat(numberOfBars));
}

const BarService & BarServiceExport::GetService() const {
	return *boost::polymorphic_downcast<const BarService *>(
		&ServiceInfoExport::GetService());
}

//////////////////////////////////////////////////////////////////////////
