/**************************************************************************
 *   Created: 2013/01/15 13:56:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "ServiceExport.hpp"
#include "PyService.hpp"
#include "BaseExport.hpp"
#include "ObjectCache.hpp"

namespace py = boost::python;
namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Services;
using namespace trdk::PyApi;
using namespace trdk::PyApi::Detail;

//////////////////////////////////////////////////////////////////////////

const trdk::Service & PyApi::ExtractService(const py::object &service) {
	try {
		Assert(service);
		ServiceExport &serviceExport = py::extract<ServiceExport &>(service);
		return serviceExport.GetService();
	} catch (const py::error_already_set &) {
		throw GetPythonClientException("Failed to extract service");
	}
}

namespace {

	ObjectCache<const trdk::Service> objectCache;

}

py::object PyApi::Export(const trdk::Service &service) {
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
		throw GetPythonClientException("Failed to export service");
	}
	throw Error("Failed to export service: Unknown service type");
}

//////////////////////////////////////////////////////////////////////////

ServiceInfoExport::ServiceInfoExport(const trdk::Service &service)
		: ModuleExport(service) {
	//...//
}

ServiceInfoExport::ServiceInfoExport(
			const boost::shared_ptr<PyApi::Service> &service)
		: ModuleExport(*service),
		m_serviceRefHolder(service) {
	//...//
}

void ServiceInfoExport::ExportClass(const char *className) {
	
	typedef py::class_<
			ServiceInfoExport,
			py::bases<ModuleExport>>
		Export;
	const py::scope serviceInfoClass = Export(className, py::no_init)
		.add_property("securities", &ServiceInfoExport::GetSecurities);
	
	ServiceSecurityListExport::ExportClass("SecurityList");

}

boost::shared_ptr<PyApi::Service> ServiceInfoExport::ReleaseRefHolder()
		throw() {
	Assert(m_serviceRefHolder);
	const auto serviceRefHolder = m_serviceRefHolder;
	m_serviceRefHolder.reset();
	return serviceRefHolder;
}

const trdk::Service & ServiceInfoExport::GetService() const {
	return *boost::polymorphic_downcast<const trdk::Service *>(&GetModule());
}

ServiceSecurityListExport ServiceInfoExport::GetSecurities() const {
	return ServiceSecurityListExport(GetService());
}

//////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
#	pragma warning(push)
#	pragma warning(disable: 4355)
#endif

ServiceExport::ServiceExport(PyObject *self, uintptr_t instanceParam)
		: ServiceInfoExport(
			boost::shared_ptr<PyApi::Service>(
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

void ServiceExport::ExportClass(const char *className) {
	typedef py::class_<
			ServiceExport,
			py::bases<ServiceInfoExport>,
			Detail::PythonToCoreTransitHolder<ServiceExport>,
			boost::noncopyable>
		Export;
	Export(className, py::init<boost::uintmax_t>());
}

const PyApi::Service & ServiceExport::GetService() const {
	return const_cast<ServiceExport *>(this)->GetService();
}

PyApi::Service & ServiceExport::GetService() {
	return *boost::polymorphic_downcast<PyApi::Service *>(
		&const_cast<trdk::Service &>(ServiceInfoExport::GetService()));
}

//////////////////////////////////////////////////////////////////////////

BarServiceExport::BarExport::BarExport(
			const BarService &service,
			const BarService::Bar &bar)
		: m_service(&service),
		m_bar(&bar) {
	//...//
}

void BarServiceExport::BarExport::ExportClass(const char *className) {
	py::class_<BarExport>(className, py::no_init)
		.add_property("time", &BarExport::GetTime)
		.add_property("size", &BarExport::GetSize)
		.add_property("maxAskPrice", &BarExport::GetMaxAskPrice)
		.add_property("minBidPrice", &BarExport::GetMinBidPrice)
		.add_property("openPrice", &BarExport::GetOpenPrice)
		.add_property("closePrice", &BarExport::GetClosePrice)
		.add_property("highPrice", &BarExport::GetHighPrice)
		.add_property("lowPrice", &BarExport::GetLowPrice)
		.add_property("tradingVolume", &BarExport::GetTradingVolume);
}

time_t BarServiceExport::BarExport::GetTime() const {
	return ConvertToTimeT(m_bar->time);
}

size_t BarServiceExport::BarExport::GetSize() const {
	return m_service->GetBarSize().total_seconds();
}

boost::intmax_t BarServiceExport::BarExport::GetMaxAskPrice() const {
	return m_bar->maxAskPrice;
}

boost::intmax_t BarServiceExport::BarExport::GetMinBidPrice() const {
	return m_bar->minBidPrice;
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
			
boost::intmax_t BarServiceExport::BarExport::GetTradingVolume() const {
	return m_bar->tradingVolume;
}

void BarServiceExport::StatExport::ExportClass(const char *className) {
	py::class_<StatExport>(className, py::no_init);
}
        
BarServiceExport::PriceStatExport::PriceStatExport(
			const boost::shared_ptr<const BarService::ScaledPriceStat> &stat)
		: m_stat(stat) {
	//...//
}

void BarServiceExport::PriceStatExport::ExportClass(const char *className) {
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

void BarServiceExport::QtyStatExport::ExportClass(const char *className) {
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

void BarServiceExport::ExportClass(const char *className) {

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
 		.def("getTradingVolumeStat", &BarServiceExport::GetTradingVolumeStat);

	BarExport::ExportClass("Bar");

	StatExport::ExportClass("Stat");
	PriceStatExport::ExportClass("PriceStat");
	QtyStatExport::ExportClass("QtyStat");

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
        
BarServiceExport::QtyStatExport BarServiceExport::GetTradingVolumeStat(
			size_t numberOfBars)
		const {
	return QtyStatExport(GetService().GetTradingVolumeStat(numberOfBars));
}

const BarService & BarServiceExport::GetService() const {
	return *boost::polymorphic_downcast<const BarService *>(
		&ServiceInfoExport::GetService());
}

//////////////////////////////////////////////////////////////////////////
