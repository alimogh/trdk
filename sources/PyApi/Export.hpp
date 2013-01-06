/**************************************************************************
 *   Created: 2012/11/20 20:44:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Services/BarService.hpp"

//////////////////////////////////////////////////////////////////////////

namespace Trader { namespace PyApi { namespace Export {

	//////////////////////////////////////////////////////////////////////////
	
	class ConstSecurity : private boost::noncopyable {

	public:

		explicit ConstSecurity(
					boost::shared_ptr<const Trader::Security> security)
				: m_security(security) {
			//...//
		}

	public:

		boost::python::str GetSymbol() const {
			return m_security->GetSymbol().c_str();
		}

		boost::python::str GetFullSymbol() const {
			return m_security->GetFullSymbol().c_str();
		}

		boost::python::str GetCurrency() const {
			return m_security->GetCurrency();
		}

		Trader::ScaledPrice GetPriceScale() const {
			return m_security->GetPriceScale();
		}
		Trader::ScaledPrice ScalePrice(double price) const {
			return int(m_security->ScalePrice(price));
		}
		double DescalePrice(Trader::ScaledPrice price) const {
			return m_security->DescalePrice(price);
		}

		Trader::ScaledPrice GetLastPriceScaled() const {
			return m_security->GetLastPriceScaled();
		}
		Trader::Qty GetLastQty() const {
			return m_security->GetLastQty();
		}

		Trader::ScaledPrice GetAskPriceScaled() const {
			return m_security->GetAskPriceScaled();
		}
		Trader::Qty GetAskQty() const {
			return m_security->GetAskQty();
		}

		Trader::ScaledPrice GetBidPriceScaled() const {
			return m_security->GetBidPriceScaled();
		}
		Trader::Qty GetBidQty() const {
			return m_security->GetBidQty();
		}

	private:

		const boost::shared_ptr<const Trader::Security> m_security;

	};

	class Security : public Export::ConstSecurity {

	public:

		explicit Security(boost::shared_ptr<Trader::Security> security)
				: ConstSecurity(security),
				m_security(security) {
			//...//
		}

	public:

		static void Export(const char *className) {

			namespace py = boost::python;
			
			py::class_<Security, boost::noncopyable>(className,  py::no_init)

				.add_property("symbol", &Security::GetSymbol)
				.add_property("fullSymbol", &Security::GetFullSymbol)
				.add_property("currency", &Security::GetCurrency)

				.add_property("priceScale", &Security::GetPriceScale)
				.def("scalePrice", &Security::ScalePrice)
				.def("descalePrice", &Security::DescalePrice)

				.add_property("lastPrice", &Security::GetLastPriceScaled)
				.add_property("lastSize", &Security::GetLastQty)

				.add_property("askPrice", &Security::GetAskPriceScaled)
				.add_property("askSize", &Security::GetAskQty)

				.add_property("bidPrice", &Security::GetBidPriceScaled)
				.add_property("bidSize", &Security::GetBidQty)

				.def("cancelOrder", &Security::CancelOrder)
				.def("cancelAllOrders", &Security::CancelAllOrders);

		}

	public:

		boost::shared_ptr<Trader::Security> GetSecurity() {
			return m_security;
		}

	public:

		void CancelOrder(int orderId) {
			m_security->CancelOrder(orderId);
		}

		void CancelAllOrders() {
			m_security->CancelAllOrders();
		}

	private:

		const boost::shared_ptr<Trader::Security> m_security;

	};
	
	//////////////////////////////////////////////////////////////////////////

	class Service : private boost::noncopyable {

	public:

		Export::ConstSecurity security;

	public:

		explicit Service(const Trader::Service &service)
				: security(service.GetSecurity().shared_from_this()),
				m_service(service) {
			//...//
		}

		virtual ~Service() {
			//...//
		}

	public:

		static void Export(const char *className) {
			namespace py = boost::python;
			py::class_<Service, boost::noncopyable>(className, py::no_init)
				.add_property("tag", &Service::GetTag)
 				.def_readonly("security", &Service::security)
 				.def("getName", &Service::GetName);
		}

	public:

		const Trader::Service & GetService() const {
			return m_service;
		}

	public:

		boost::python::str GetTag() const {
			return m_service.GetTag().c_str();
		}

		boost::python::str GetName() const {
			return m_service.GetName().c_str();
		}

	protected:

		template<typename T>
		const T & Get() const {
			return *boost::polymorphic_downcast<const T *>(&m_service);
		}

	private:

		const Trader::Service &m_service;

	};

	//////////////////////////////////////////////////////////////////////////
	
} } }

//////////////////////////////////////////////////////////////////////////
	
namespace Trader { namespace PyApi { namespace Export { namespace Services {

	//////////////////////////////////////////////////////////////////////////

	class BarService : public Export::Service {

	public:

		class Bar {
			
		public:

			explicit Bar(
						const Trader::Services::BarService &service,
						const Trader::Services::BarService::Bar &bar)
					: m_service(&service),
					m_bar(&bar) {
				//...//
			}

		public:
			
			static void Export(const char *className) {
				namespace py = boost::python;
				py::class_<Bar>(className, py::no_init)
					.add_property("time", &Bar::GetTime)
					.add_property("size", &Bar::GetSize)
					.add_property("openPrice", &Bar::GetOpenPrice)
					.add_property("closePrice", &Bar::GetClosePrice)
					.add_property("highPrice", &Bar::GetHighPrice)
					.add_property("lowPrice", &Bar::GetLowPrice)
					.add_property("volume", &Bar::GetVolume);
			}

		public:

			time_t GetTime() const {
				return Util::ConvertToTimeT(m_bar->time);
			}

			size_t GetSize() const {
				return m_service->GetBarSize().total_seconds();
			}

			boost::intmax_t GetOpenPrice() const {
				return m_bar->openPrice;
			}
			boost::intmax_t GetClosePrice() const {
				return m_bar->closePrice;
			}
			
			boost::intmax_t GetHighPrice() const {
				return m_bar->highPrice;
			}
			boost::intmax_t GetLowPrice() const {
				return m_bar->lowPrice;
			}
			
			boost::intmax_t GetVolume() const {
				return m_bar->volume;
			}

		private:

			const Trader::Services::BarService *m_service;
			const Trader::Services::BarService::Bar *m_bar;

		};

	    class Stat {
		public:
			static void Export(const char *className) {
				namespace py = boost::python;
				py::class_<Stat, boost::noncopyable>(className, py::no_init);
			}
		};
        
		class PriceStat : public Stat {
		public:
			typedef Trader::Services::BarService::ScaledPriceStat::ValueType
				ValueType;
		public:
			explicit PriceStat(
						const boost::shared_ptr<const Trader::Services::BarService::ScaledPriceStat> &stat)
					: m_stat(stat) {
				//...//
			}
		public:
			static void Export(const char *className) {
				namespace py = boost::python;
				py::class_<PriceStat>(className, py::no_init)
					.add_property("max", &PriceStat::GetMax)
					.add_property("min", &PriceStat::GetMin);
			}
		public:
			ValueType GetMax() const {
				return m_stat->GetMax();
			}
			ValueType GetMin() const {
				return m_stat->GetMin();
			}
		private:
			boost::shared_ptr<const Trader::Services::BarService::ScaledPriceStat> m_stat;
		};

        class QtyStat : public Stat {
		public:
			typedef Trader::Services::BarService::QtyStat::ValueType
				ValueType;
		public:
			explicit QtyStat(
						const boost::shared_ptr<const Trader::Services::BarService::QtyStat> &stat)
					: m_stat(stat) {
				//...//
			}
		public:
			static void Export(const char *className) {
				namespace py = boost::python;
				py::class_<QtyStat>(className, py::no_init)
					.add_property("max", &QtyStat::GetMax)
					.add_property("min", &QtyStat::GetMin);
			}
		public:
			ValueType GetMax() const {
				return m_stat->GetMax();
			}
			ValueType GetMin() const {
				return m_stat->GetMin();
			}
		private:
			boost::shared_ptr<const Trader::Services::BarService::QtyStat> m_stat;
		};
	
	public:

		explicit BarService(const Trader::Services::BarService &barService)
				: Export::Service(barService) {
			//...//
		}

	public:

		static void Export(const char *className) {

			namespace py = boost::python;

			typedef py::class_<
					BarService,
					py::bases<Service>,
					boost::noncopyable>
				BarServiceExport;
			const boost::python::scope barServiceClass
				= BarServiceExport(className, py::no_init)
					
					.add_property("barSize", &BarService::GetBarSize)
					.add_property("size", &BarService::GetSize)
					.add_property("isEmpty", &BarService::IsEmpty)
 					
					.def("getBarByIndex", &BarService::GetBarByIndex)
 					.def("getBarByReversedIndex", &BarService::GetBarByReversedIndex)

 					.def("getOpenPriceStat", &BarService::GetOpenPriceStat)
 					.def("getClosePriceStat", &BarService::GetClosePriceStat)
 					.def("getHighPriceStat", &BarService::GetHighPriceStat)
 					.def("getHighPriceStat", &BarService::GetHighPriceStat)
 					.def("getVolumeStat", &BarService::GetVolumeStat);

			Bar::Export("Bar");

			Stat::Export("Stat");
			PriceStat::Export("PriceStat");
			QtyStat::Export("QtyStat");

		}

	public:

		boost::python::str GetName() const {
			return GetService().GetName().c_str();
		}
		
		size_t GetBarSize() const {
			return GetService().GetBarSize().total_seconds();
		}
		
		size_t GetSize() const {
			return GetService().GetSize();
		}
		
		bool IsEmpty() const {
			return GetService().IsEmpty();
		}
 		
		Bar GetBarByIndex(size_t index) const {
			return Bar(GetService(), GetService().GetBar(index));
		}
 		
		Bar GetBarByReversedIndex(size_t index) const {
			return Bar(GetService(), GetService().GetBarByReversedIndex(index));
		}

		PriceStat GetOpenPriceStat(size_t numberOfBars) const {
			return PriceStat(GetService().GetOpenPriceStat(numberOfBars));
		}
        
		PriceStat GetClosePriceStat(size_t numberOfBars) const {
			return PriceStat(GetService().GetClosePriceStat(numberOfBars));
		}
        
		PriceStat GetHighPriceStat(size_t numberOfBars) const {
			return PriceStat(GetService().GetHighPriceStat(numberOfBars));
		}
        
		PriceStat GetLowPriceStat(size_t numberOfBars) const {
			return PriceStat(GetService().GetLowPriceStat(numberOfBars));
		}
        
		QtyStat GetVolumeStat(size_t numberOfBars) const {
			return QtyStat(GetService().GetVolumeStat(numberOfBars));
		}

	protected:

		const Trader::Services::BarService & GetService() const {
			return Get<Trader::Services::BarService>();
		}

	};

	//////////////////////////////////////////////////////////////////////////

} } } }

//////////////////////////////////////////////////////////////////////////
