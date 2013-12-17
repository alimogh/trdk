/**************************************************************************
 *   Created: 2012/11/05 14:02:49
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Service.hpp"
#include "Detail.hpp"

namespace trdk { namespace PyApi {

	class Service : public trdk::Service {

		template<typename Module>
		friend void trdk::PyApi::Detail::UpdateAlgoSettings(
				Module &,
				const trdk::Lib::IniSectionRef &);

	public:

		typedef trdk::Service Base;

	public:

		explicit Service(uintptr_t, ServiceExport &serviceExport);
		virtual ~Service();

	public:

		static boost::shared_ptr<trdk::Service> CreateClientInstance(
				Context &context,
				const std::string &tag,
				const Lib::IniSectionRef &configuration);

		ServiceExport & GetExport();
		const ServiceExport & GetExport() const;

	public:

		using trdk::Service::GetTag;

		virtual std::string GetRequiredSuppliers() const;

		virtual boost::posix_time::ptime OnSecurityStart(
					const trdk::Security &);
		virtual void OnServiceStart(const trdk::Service &);

		virtual bool OnLevel1Update(const trdk::Security &);
		virtual bool OnNewTrade(
					const trdk::Security &,
					const boost::posix_time::ptime &,
					trdk::ScaledPrice,
					trdk::Qty,
					trdk::OrderSide);
		virtual bool OnServiceDataUpdate(const trdk::Service &);
		virtual bool OnBrokerPositionUpdate(
					trdk::Security &,
					trdk::Qty,
					bool isInitial);

	protected:

		virtual void UpdateAlogImplSettings(
					const trdk::Lib::IniSectionRef &);

	private:

		void DoSettingsUpdate(const trdk::Lib::IniSectionRef &);
		
		bool CallVirtualMethod(
					const char *name,
					const boost::function<void (const boost::python::override &)> &)
				const;
		boost::shared_ptr<PyApi::Service> TakeExportObjectOwnership();

	private:
		
		ServiceExport &m_serviceExport;
		boost::shared_ptr<ServiceExport> m_serviceExportRefHolder;

	};

} }
