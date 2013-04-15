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
#include "PythonToCoreTransit.hpp"
#include "Services/BarService.hpp"

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

		const trdk::Service & GetService() const {
			return *m_service;
		}

		boost::shared_ptr<PyApi::Service> ReleaseRefHolder() throw();

	private:

		boost::shared_ptr<PyApi::Service> m_serviceRefHolder;
		const trdk::Service *m_service;

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
			boost::intmax_t GetOpenPrice() const;
			boost::intmax_t GetClosePrice() const;
			boost::intmax_t GetHighPrice() const;
			boost::intmax_t GetLowPrice() const;
			boost::intmax_t GetVolume() const;
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

		boost::python::str GetName() const;
		size_t GetBarSize() const;
		size_t GetSize() const;
		bool IsEmpty() const;
		BarExport GetBarByIndex(size_t index) const;
		BarExport GetBarByReversedIndex(size_t index) const;
		PriceStatExport GetOpenPriceStat(size_t numberOfBars) const;
		PriceStatExport GetClosePriceStat(size_t numberOfBars) const;
		PriceStatExport GetHighPriceStat(size_t numberOfBars) const;
		PriceStatExport GetLowPriceStat(size_t numberOfBars) const;
		QtyStatExport GetVolumeStat(size_t numberOfBars) const;

	protected:

		const Implementation & GetService() const;

	private:

		SecurityInfoExport m_securityExport;

	};

	//////////////////////////////////////////////////////////////////////////

} }

//////////////////////////////////////////////////////////////////////////
