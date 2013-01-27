/**************************************************************************
 *   Created: 2012/11/05 14:02:49
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/Service.hpp"
#include "Detail.hpp"

namespace Trader { namespace PyApi {

	class Service : public Trader::Service {

		template<typename Module>
		friend void Trader::PyApi::Detail::UpdateAlgoSettings(
				Module &,
				const Trader::Lib::IniFileSectionRef &);

	public:

		typedef Trader::Service Base;

	public:

		explicit Service(uintptr_t, ServiceExport &serviceExport);
		virtual ~Service();

	public:

		static boost::shared_ptr<Trader::Service> CreateClientInstance(
				const std::string &tag,
				boost::shared_ptr<Trader::Security>,
				const Trader::Lib::IniFileSectionRef &,
				const boost::shared_ptr<const Trader::Settings> &);

		ServiceExport & GetExport();
		const ServiceExport & GetExport() const;

	public:

		using Trader::Service::GetTag;

		virtual void OnServiceStart(const Trader::Service &);
		virtual bool OnLevel1Update();
		virtual bool OnNewTrade(
					const boost::posix_time::ptime &,
					Trader::ScaledPrice,
					Trader::Qty,
					Trader::OrderSide);
		virtual bool OnServiceDataUpdate(const Trader::Service &);

	protected:

		virtual void UpdateAlogImplSettings(
					const Trader::Lib::IniFileSectionRef &);

	private:

		void DoSettingsUpdate(const Trader::Lib::IniFileSectionRef &);
		
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
