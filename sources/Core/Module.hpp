/**************************************************************************
 *   Created: 2012/09/23 14:31:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Context.hpp"
#include "Fwd.hpp"
#include "Api.h"

namespace Trader {

	//////////////////////////////////////////////////////////////////////////

	class TRADER_CORE_API Module : private boost::noncopyable {

	public:

		class TRADER_CORE_API Log;

	protected:

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;
		
	public:

		explicit Module(
					Trader::Context &,
					const std::string &typeName,
					const std::string &name,
					const std::string &tag);
		virtual ~Module();

	public:

		Trader::Context & GetContext();
		const Trader::Context & GetContext() const;

		const std::string & GetTypeName() const throw();
		const std::string & GetName() const throw();
		const std::string & GetTag() const throw();

	public:

		Trader::Module::Log & GetLog() const throw();

	public:

		virtual void OnServiceStart(const Trader::Service &);

		void UpdateSettings(const Trader::Lib::IniFileSectionRef &);

	protected:

		virtual void UpdateAlogImplSettings(
					const Trader::Lib::IniFileSectionRef &)
				= 0;

		Mutex & GetMutex() const;

		void ReportSettings(const Trader::SettingsReport::Report &) const;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

	class Trader::Module::Log : private boost::noncopyable {
	public:
		explicit Log(const Module &);
		~Log();
	public:
		void Debug(const char *str) throw() {
			m_log.DebugEx(
				[this, str]() -> boost::format {
					return GetFormat() % str;
				});
		}
		template<typename Params>
		void Debug(const char *str, const Params &params) throw() {
			m_log.DebugEx(
				[this, str, &params]() -> boost::format {
					boost::format result((GetFormat() % str).str());
					Trader::Lib::Format(params, result);
					return result;
				});
		}
		template<typename Callback>
		void DebugEx(Callback callback) throw() {
			m_log.DebugEx(
				[this, &callback]() -> boost::format {
					return GetFormat() % callback().str();
				});
		}
	public:
		void Info(const char *str) throw() {
			m_log.InfoEx(
				[this, str]() -> boost::format {
					return GetFormat() % str;
				});
		}
		template<typename Params>
		void Info(const char *str, const Params &params) throw() {
			m_log.InfoEx(
				[this, str, &params]() -> boost::format {
					boost::format result((GetFormat() % str).str());
					Trader::Lib::Format(params, result);
					return result;
				});
		}
		template<typename Callback>
		void InfoEx(Callback callback) throw() {
			m_log.InfoEx(
				[this, &callback]() -> boost::format {
					return GetFormat() % callback().str();
				});
		}
	public:
		void Warn(const char *str) throw() {
			m_log.WarnEx(
				[this, str]() -> boost::format {
					return GetFormat() % str;
				});
		}
		template<typename Params>
		void Warn(const char *str, const Params &params) throw() {
			m_log.WarnEx(
				[this, str, &params]() -> boost::format {
					boost::format result((GetFormat() % str).str());
					Trader::Lib::Format(params, result);
					return result;
				});
		}
		template<typename Callback>
		void WarnEx(Callback callback) throw() {
			m_log.WarnEx(
				[this, &callback]() -> boost::format {
					return GetFormat() % callback().str();
				});
		}
	public:
		void Error(const char *str) throw() {
			m_log.ErrorEx(
				[this, str]() -> boost::format {
					return GetFormat() % str;
				});
		}
		template<typename Params>
		void Error(const char *str, const Params &params) throw() {
			m_log.ErrorEx(
				[this, str, &params]() -> boost::format {
					boost::format result((GetFormat() % str).str());
					Trader::Lib::Format(params, result);
					return result;
				});
		}
		template<typename Callback>
		void ErrorEx(Callback callback) throw() {
			m_log.ErrorEx(
				[this, &callback]() -> boost::format {
					return GetFormat() % callback().str();
				});
		}
	private:
		boost::format GetFormat() const {
			return m_format;
		}
	private:
		Trader::Context::Log &m_log;
		const boost::format m_format;
	};

	//////////////////////////////////////////////////////////////////////////

}

//////////////////////////////////////////////////////////////////////////

namespace std {
	TRADER_CORE_API std::ostream & operator <<(
				std::ostream &,
				const Trader::Module &)
			throw();
}

//////////////////////////////////////////////////////////////////////////
