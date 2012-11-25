/**************************************************************************
 *   Created: 2012/11/05 14:02:49
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Import.hpp"
#include "Detail.hpp"

namespace Trader { namespace PyApi {

	class Service
			: public Trader::Service,
			public Import::Service,
			public boost::python::wrapper<Trader::PyApi::Service> {

		template<typename Module>
		friend void Trader::PyApi::Detail::UpdateAlgoSettings(
				Module &,
				const Trader::Lib::IniFileSectionRef &);

	public:

		explicit Service(uintmax_t);
		virtual ~Service();

		static boost::shared_ptr<Trader::Service> CreateClientInstance(
				const std::string &tag,
				boost::shared_ptr<Trader::Security>,
				const Trader::Lib::IniFileSectionRef &);

	public:

		virtual boost::python::str Service::PyGetName() const;
		virtual void PyNotifyServiceStart(boost::python::object);

	public:

		using Trader::Service::GetTag;

		virtual const std::string & GetName() const {
			return m_name;
		}

		virtual void NotifyServiceStart(const Trader::Service &);

		operator boost::python::object &() const {
			Assert(m_self);
			return m_self;
		}

	protected:

		virtual void UpdateAlogImplSettings(
					const Trader::Lib::IniFileSectionRef &);

	private:

		void DoSettingsUpdate(const Trader::Lib::IniFileSectionRef &);
		void UpdateCallbacks();

	private:

		std::unique_ptr<Script> m_script;
		std::string m_name;
		mutable boost::python::object m_self;

	};

} }
