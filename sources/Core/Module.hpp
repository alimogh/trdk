/**************************************************************************
 *   Created: 2012/09/23 14:31:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

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
					const std::string &typeName,
					const std::string &name,
					const std::string &tag,
					boost::shared_ptr<const Trader::Settings>);
		virtual ~Module();

	public:

		const std::string & GetTypeName() const throw();
		const std::string & GetName() const throw();
		const std::string & GetTag() const throw();

	public:

		Trader::Module::Log & GetLog() const throw();

		//! @todo: Move ref to engine.
		const Trader::Settings & GetSettings() const;

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
			Trader::Log::DebugEx(
				[this, str]() -> boost::format {
					return GetFormat() % str;
				});
		}
		template<typename Params>
		void Debug(const char *str, const Params &params) throw() {
			Trader::Log::DebugEx(
				[this, str, &params]() -> boost::format {
					boost::format result((GetFormat() % str).str());
					Trader::Util::Format(params, result);
					return result;
				});
		}
		template<typename Callback>
		void DebugEx(Callback callback) throw() {
			Trader::Log::DebugEx(
				[this, &callback]() -> boost::format {
					return GetFormat() % callback().str();
				});
		}
	public:
		void Info(const char *str) throw() {
			Trader::Log::InfoEx(
				[this, str]() -> boost::format {
					return GetFormat() % str;
				});
		}
		template<typename Params>
		void Info(const char *str, const Params &params) throw() {
			Trader::Log::InfoEx(
				[this, str, &params]() -> boost::format {
					boost::format result((GetFormat() % str).str());
					Trader::Util::Format(params, result);
					return result;
				});
		}
		template<typename Callback>
		void InfoEx(Callback callback) throw() {
			Trader::Log::InfoEx(
				[this, &callback]() -> boost::format {
					return GetFormat() % callback().str();
				});
		}
	public:
		void Warn(const char *str) throw() {
			Trader::Log::WarnEx(
				[this, str]() -> boost::format {
					return GetFormat() % str;
				});
		}
		template<typename Params>
		void Warn(const char *str, const Params &params) throw() {
			Trader::Log::WarnEx(
				[this, str, &params]() -> boost::format {
					boost::format result((GetFormat() % str).str());
					Trader::Util::Format(params, result);
					return result;
				});
		}
		template<typename Callback>
		void WarnEx(Callback callback) throw() {
			Trader::Log::WarnEx(
				[this, &callback]() -> boost::format {
					return GetFormat() % callback().str();
				});
		}
	public:
		void Error(const char *str) throw() {
			Trader::Log::ErrorEx(
				[this, str]() -> boost::format {
					return GetFormat() % str;
				});
		}
		template<typename Params>
		void Error(const char *str, const Params &params) throw() {
			Trader::Log::ErrorEx(
				[this, str, &params]() -> boost::format {
					boost::format result((GetFormat() % str).str());
					Trader::Util::Format(params, result);
					return result;
				});
		}
		template<typename Callback>
		void ErrorEx(Callback callback) throw() {
			Trader::Log::ErrorEx(
				[this, &callback]() -> boost::format {
					return GetFormat() % callback().str();
				});
		}
	public:
		void Trading(const char *str) throw() {
			Trader::Log::TradingEx(
				m_tag.c_str(),
				[this, str]() -> boost::format {
					return GetFormat() % str;
				});
		}
		template<typename Params>
		void Trading(const char *str, const Params &params) throw() {
			Trader::Log::TradingEx(
				m_tag.c_str(),
				[this, str, &params]() -> boost::format {
					boost::format result((GetFormat() % str).str());
					Trader::Util::Format(params, result);
					return result;
				});
		}
		template<typename Callback>
		void TradingEx(Callback callback) throw() {
			Trader::Log::TradingEx(
				m_tag.c_str(),
				[this, &callback]() -> boost::format {
					return GetFormat() % callback().str();
				});
		}
	private:
		boost::format GetFormat() const {
			return m_format;
		}
	private:
		const boost::format m_format;
		const std::string &m_tag;
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
