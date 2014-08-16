/**************************************************************************
 *   Created: 2013/01/31 01:04:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Fwd.hpp"
#include "Api.h"

namespace trdk {

	//////////////////////////////////////////////////////////////////////////

	class TRDK_CORE_API Context : private boost::noncopyable {

	public:

		class TRDK_CORE_API Exception : public trdk::Lib::Exception {
		public:
			explicit Exception(const char *what) throw();
		};
		class TRDK_CORE_API UnknownSecurity : public trdk::Context::Exception {
		public:
			explicit UnknownSecurity() throw();
		};

		class TRDK_CORE_API Log;

		class TRDK_CORE_API Params;

	public:

		Context();
		virtual ~Context();

	public:

		trdk::Context::Log & GetLog() const throw();

		trdk::Security & GetSecurity(const trdk::Lib::Symbol &);
		const trdk::Security & GetSecurity(const trdk::Lib::Symbol &) const;

	public:

		//! Context setting with predefined key list and predefined behavior.
		virtual const trdk::Settings & GetSettings() const = 0;

		//! User context parameters. No predefined key list. Any key can be
		//! changed.
		trdk::Context::Params & GetParams();
		//! User context parameters. No predefined key list.
		const trdk::Context::Params & GetParams() const;

		//! Applies the given predicate to the each market data source and
		//! stops if predicate returns false.
		virtual void ForEachMarketDataSource(
						const boost::function<bool (const trdk::MarketDataSource &)> &)
					const
					= 0;
		//! Applies the given predicate to the each market data source and
		//! stops if predicate returns false.
		virtual void ForEachMarketDataSource(
						const boost::function<bool (trdk::MarketDataSource &)> &)
					= 0;

		virtual trdk::TradeSystem & GetTradeSystem() = 0;
		virtual const trdk::TradeSystem & GetTradeSystem() const = 0;

	protected:

		virtual trdk::Security * FindSecurity(const trdk::Lib::Symbol &) = 0;
		virtual const trdk::Security * FindSecurity(
						const trdk::Lib::Symbol &)
					const
					= 0;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

	class trdk::Context::Log : private boost::noncopyable {
	public:
		explicit Log(const Context &);
		~Log();
	public:
		void Debug(const char *str) throw() {
			trdk::Log::Debug(str);
		}
		template<typename Params>
		void Debug(const char *str, const Params &params) throw() {
			trdk::Log::Debug(str, params);
		}
		template<typename Callback>
		void DebugEx(const Callback &callback) throw() {
			trdk::Log::DebugEx(callback);
		}
	public:
		void Info(const char *str) throw() {
			trdk::Log::Info(str);
		}
		template<typename Params>
		void Info(const char *str, const Params &params) throw() {
			trdk::Log::Info(str, params);
		}
		template<typename Callback>
		void InfoEx(const Callback &callback) throw() {
			trdk::Log::InfoEx(callback);
		}
	public:
		void Warn(const char *str) throw() {
			trdk::Log::Warn(str);
		}
		template<typename Params>
		void Warn(const char *str, const Params &params) throw() {
			trdk::Log::Warn(str, params);
		}
		template<typename Callback>
		void WarnEx(const Callback &callback) throw() {
			trdk::Log::WarnEx(callback);
		}
	public:
		void Error(const char *str) throw() {
			trdk::Log::Error(str);
		}
		template<typename Params>
		void Error(const char *str, const Params &params) throw() {
			trdk::Log::Error(str, params);
		}
		template<typename Callback>
		void ErrorEx(const Callback &callback) throw() {
			trdk::Log::ErrorEx(callback);
		}
	public:
		void Trading(const std::string &tag, const char *str) throw() {
			trdk::Log::Trading(tag, str);
		}
		template<typename Callback>
		void TradingEx(const std::string &tag, const Callback &callback) throw() {
			trdk::Log::TradingEx(tag, callback);
		}
		template<typename Params>
		void Trading(
					const std::string &tag,
					const char *str,
					const Params &params)
				throw() {
			trdk::Log::Trading(tag, str, params);
		}
	};

	//////////////////////////////////////////////////////////////////////////


	class trdk::Context::Params : private boost::noncopyable {

	public:

		class TRDK_CORE_API Exception : public trdk::Context::Exception {
		public:
			Exception(const char *what) throw();
			~Exception();
		};

		class TRDK_CORE_API KeyDoesntExistError : public Exception {
		public:
			KeyDoesntExistError(const char *what) throw();
			~KeyDoesntExistError();
		};

		typedef uintmax_t Revision;
	
	public:
	
		Params(const trdk::Context &);
		~Params();

	public:

		//! Returns key value.
		/** Throws an exception if key doesn't exist.
		  * @sa trdk::Context::Parameters::Update
		  * @throw trdk::Context::Parameters::KeyDoesntExistError
		  */
		std::string operator [](const std::string &) const;

	public:
		
		//! Returns current object revision.
		/** Any field update changes revision number. Update rule isn't defined.
		  */
		Revision GetRevision() const;

		bool IsExist(const std::string &) const;

		//! Updates key. Creates new if key doesn't exist.
		void Update(const std::string &key, const std::string &value);
	
	private:
	
		class Implementation;
		Implementation *m_pimpl;
	
	};

	////////////////////////////////////////////////////////////////////////////////

}
