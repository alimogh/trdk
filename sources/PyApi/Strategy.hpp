/**************************************************************************
 *   Created: 2012/08/06 13:08:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Import.hpp"
#include "Detail.hpp"

namespace Trader { namespace PyApi {

	class Strategy
			: public Trader::Strategy,
			public Import::Strategy,
			public boost::python::wrapper<Trader::PyApi::Strategy> {

		template<typename Module>
		friend void Trader::PyApi::Detail::UpdateAlgoSettings(
				Module &,
				const Trader::Lib::IniFileSectionRef &);

	public:

		explicit Strategy(uintmax_t);
		virtual ~Strategy();

		static boost::shared_ptr<Trader::Strategy> CreateClientInstance(
					const std::string &tag,
					boost::shared_ptr<Trader::Security>,
					const Trader::Lib::IniFileSectionRef &);

	public:

		boost::python::str CallGetNamePyMethod() const;
		void CallNotifyServiceStartPyMethod(const boost::python::object &);
		boost::python::object CallTryToOpenPositionsPyMethod();
		void CallTryToClosePositionsPyMethod(const boost::python::object &);

	public:

		using Trader::Strategy::GetTag;

		virtual const std::string & GetName() const {
			return m_name;
		}

		virtual void NotifyServiceStart(const Trader::Service &);

	public:

		virtual boost::shared_ptr<PositionBandle> TryToOpenPositions();
		virtual void TryToClosePositions(PositionBandle &);

		virtual void ReportDecision(const Trader::Position &) const;

	protected:

		virtual std::auto_ptr<Trader::PositionReporter> CreatePositionReporter()
				const;

		virtual void UpdateAlogImplSettings(
					const Trader::Lib::IniFileSectionRef &);

	private:

		void DoSettingsUpdate(const Trader::Lib::IniFileSectionRef &);
		void UpdateCallbacks();

	private:

		std::unique_ptr<Script> m_script;
		std::string m_name;
		boost::python::object m_self;

		//! @todo remove
		std::list<boost::python::object> m_pyCache;

	};

} }
