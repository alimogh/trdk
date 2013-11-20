/**************************************************************************
 *   Created: 2013/01/15 13:56:50
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "ModuleExport.hpp"
#include "ModuleSecurityListExport.hpp"
#include "Services/BollingerBandsService.hpp"
#include "Services/MovingAverageService.hpp"
#include "Services/BarService.hpp"
#include "PythonToCoreTransit.hpp"

//////////////////////////////////////////////////////////////////////////

namespace trdk { namespace PyApi {

	//////////////////////////////////////////////////////////////////////////

	class ServiceInfoExport : public ModuleExport {

	public:

		//! C-tor.
		/*  @param serviceRef	Reference to service, must be alive all time
		 *						while export object exists.
		 */
		explicit ServiceInfoExport(const trdk::Service &);

		explicit ServiceInfoExport(const boost::shared_ptr<PyApi::Service> &);

	public:

		static void ExportClass(const char *className);

	public:

		const trdk::Service & GetService() const;

		boost::shared_ptr<PyApi::Service> ReleaseRefHolder() throw();

	private:

		ServiceSecurityListExport GetSecurities() const;

	private:

		boost::shared_ptr<PyApi::Service> m_serviceRefHolder;

	};

	//////////////////////////////////////////////////////////////////////////

	class ServiceExport
		: public ServiceInfoExport,
		public Detail::PythonToCoreTransit<ServiceExport>,
		public boost::enable_shared_from_this<ServiceExport> {

	public:

		//! Creates new service instance.
		explicit ServiceExport(PyObject *self, uintptr_t instanceParam);
		
	public:

		static void ExportClass(const char *className);

	public:

		PyApi::Service & GetService();
		const PyApi::Service & GetService() const;

	};

	//////////////////////////////////////////////////////////////////////////

	const trdk::Service & ExtractService(const boost::python::object &);
	boost::python::object Export(const trdk::Service &);

	//////////////////////////////////////////////////////////////////////////

} }

//////////////////////////////////////////////////////////////////////////

namespace trdk { namespace PyApi {

	//////////////////////////////////////////////////////////////////////////
	
	class BarServiceExport : public ServiceInfoExport {

	public:

		typedef trdk::Services::BarService Implementation;

	public:

		class BarExport {
		public:
			explicit BarExport(
						const Implementation &,
						const Implementation::Bar &);
		public:
			static void ExportClass(const char *className);
		public:
			time_t GetTime() const;
			size_t GetSize() const;
			boost::intmax_t GetMaxAskPrice() const;
			boost::intmax_t GetOpenAskPrice() const;
			boost::intmax_t GetCloseAskPrice() const;
			boost::intmax_t GetMinBidPrice() const;
			boost::intmax_t GetOpenBidPrice() const;
			boost::intmax_t GetCloseBidPrice() const;
			boost::intmax_t GetOpenTradePrice() const;
			boost::intmax_t GetCloseTradePrice() const;
			boost::intmax_t GetHighTradePrice() const;
			boost::intmax_t GetLowTradePrice() const;
			boost::intmax_t GetTradingVolume() const;
		private:
			const Implementation *m_service;
			const Implementation::Bar *m_bar;
		};

	    class StatExport {
		public:
			static void ExportClass(const char *className);
		};
        
		class PriceStatExport : public StatExport {
		public:
			typedef Implementation::ScaledPriceStat::ValueType ValueType;
		public:
			explicit PriceStatExport(
					const boost::shared_ptr<const Implementation::ScaledPriceStat> &);
		public:
			static void ExportClass(const char *className);
		public:
			ValueType GetMax() const;
			ValueType GetMin() const;
		private:
			boost::shared_ptr<const Implementation::ScaledPriceStat> m_stat;
		};

        class QtyStatExport : public StatExport {
		public:
			typedef Implementation::QtyStat::ValueType ValueType;
		public:
			explicit QtyStatExport(
					const boost::shared_ptr<const Implementation::QtyStat> &);
		public:
			static void ExportClass(const char *className);
		public:
			ValueType GetMax() const;
			ValueType GetMin() const;
		private:
			boost::shared_ptr<const Implementation::QtyStat> m_stat;
		};
	
	public:

		//! C-tor.
		/*  @param serviceRef	Reference to service, must be alive all time
		 *						while export object exists.
		 */
		explicit BarServiceExport(const Implementation &serviceRef);

	public:

		static void ExportClass(const char *className);

	public:

		size_t GetBarSize() const;
		size_t GetSize() const;
		bool IsEmpty() const;
		BarExport GetBar(size_t index) const;
		BarExport GetBarByReversedIndex(size_t index) const;
		BarExport GetLastBar() const;
		PriceStatExport GetOpenPriceStat(size_t numberOfBars) const;
		PriceStatExport GetClosePriceStat(size_t numberOfBars) const;
		PriceStatExport GetHighPriceStat(size_t numberOfBars) const;
		PriceStatExport GetLowPriceStat(size_t numberOfBars) const;
		QtyStatExport GetTradingVolumeStat(size_t numberOfBars) const;

	protected:

		const Implementation & GetService() const;

	};

	//////////////////////////////////////////////////////////////////////////

	class MovingAverageServiceExport : public ServiceInfoExport {

	public:

		typedef trdk::Services::MovingAverageService Implementation;

	public:

		class PointExport {
		public:
			explicit PointExport(
						const Implementation &,
						const Implementation::Point &);
		public:
			static void ExportClass(const char *className);
		public:
			boost::intmax_t GetSource() const {
				return m_point.source;
			}
			double GetValue() const {
				return m_point.value;
			}
		private:
			const Implementation *m_service;
			Implementation::Point m_point;
		};

	public:

		//! C-tor.
		/*  @param serviceRef	Reference to service, must be alive all time
		 *						while export object exists.
		 */
		explicit MovingAverageServiceExport(const Implementation &serviceRef);

	public:

		static void ExportClass(const char *className);

	public:

		bool IsEmpty() const;

		PointExport GetLastPoint() const;

		size_t GetHistorySize() const;

		PointExport GetHistoryPoint(size_t index) const;
		PointExport GetHistoryPointByReversedIndex(size_t index) const;

	protected:

		const Implementation & GetService() const;

	};

	//////////////////////////////////////////////////////////////////////////
	
	class BollingerBandsServiceExport : public ServiceInfoExport {

	public:

		typedef trdk::Services::BollingerBandsService Implementation;

	public:

		class PointExport {
		public:
			explicit PointExport(
						const Implementation &service,
						const Implementation::Point &point)
				: m_service(&service),
				m_point(point) {
			//...//
		}
		public:
			static void ExportClass(const char *className);
		public:
			boost::intmax_t GetSource() const {
				return m_point.source;
			}
			double  GetLow() const {
				return m_point.low;
			}
			double  GetHigh() const {
				return m_point.high;
			}
		private:
			const Implementation *m_service;
			Implementation::Point m_point;
		};

	public:

		//! C-tor.
		/*  @param serviceRef	Reference to service, must be alive all time
		 *						while export object exists.
		 */
		explicit BollingerBandsServiceExport(const Implementation &serviceRef)
				: ServiceInfoExport(serviceRef) {
			//...//
		}

	public:

		static void ExportClass(const char *className);

	public:

		bool IsEmpty() const {
			return GetService().IsEmpty();
		}

		PointExport GetLastPoint() const {
			return PointExport(GetService(), GetService().GetLastPoint());
		}

		size_t GetHistorySize() const {
			return GetService().GetHistorySize();
		}

		PointExport GetHistoryPoint(size_t index) const {
			return PointExport(
				GetService(),
				GetService().GetHistoryPoint(index));
		}
		PointExport GetHistoryPointByReversedIndex(size_t index) const {
			return PointExport(
				GetService(),
				GetService().GetHistoryPointByReversedIndex(index));
		}

	protected:

		const Implementation & GetService() const {
			return *boost::polymorphic_downcast<const Implementation *>(
				&ServiceInfoExport::GetService());
		}

	};

	//////////////////////////////////////////////////////////////////////////

} }

//////////////////////////////////////////////////////////////////////////
