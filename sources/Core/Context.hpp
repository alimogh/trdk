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

namespace Trader {

	//////////////////////////////////////////////////////////////////////////

	class TRADER_CORE_API Context : private boost::noncopyable {

	public:

		class TRADER_CORE_API Log;

	public:

		Context();
		virtual ~Context();

	public:

		Trader::Context::Log & GetLog() const throw();

	public:

		virtual const Trader::Settings & GetSettings() const = 0;
		
		virtual Trader::MarketDataSource & GetMarketDataSource() = 0;
		virtual const Trader::MarketDataSource & GetMarketDataSource() const = 0;

		virtual Trader::TradeSystem & GetTradeSystem() = 0;
		virtual const Trader::TradeSystem & GetTradeSystem() const = 0;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

	//////////////////////////////////////////////////////////////////////////

	class Trader::Context::Log : private boost::noncopyable {
	public:
		explicit Log(const Context &);
		~Log();
	public:
		void Debug(const char *str) throw() {
			Trader::Log::Debug(str);
		}
		template<typename Params>
		void Debug(const char *str, const Params &params) throw() {
			Trader::Log::Debug(str, params);
		}
		template<typename Callback>
		void DebugEx(Callback callback) throw() {
			Trader::Log::DebugEx(callback);
		}
	public:
		void Info(const char *str) throw() {
			Trader::Log::Info(str);
		}
		template<typename Params>
		void Info(const char *str, const Params &params) throw() {
			Trader::Log::Info(str, params);
		}
		template<typename Callback>
		void InfoEx(Callback callback) throw() {
			Trader::Log::InfoEx(callback);
		}
	public:
		void Warn(const char *str) throw() {
			Trader::Log::Warn(str);
		}
		template<typename Params>
		void Warn(const char *str, const Params &params) throw() {
			Trader::Log::Warn(str, params);
		}
		template<typename Callback>
		void WarnEx(Callback callback) throw() {
			Trader::Log::WarnEx(callback);
		}
	public:
		void Error(const char *str) throw() {
			Trader::Log::Error(str);
		}
		template<typename Params>
		void Error(const char *str, const Params &params) throw() {
			Trader::Log::Error(str, params);
		}
		template<typename Callback>
		void ErrorEx(Callback callback) throw() {
			Trader::Log::ErrorEx(callback);
		}
	public:
		void Trading(const std::string &tag, const char *) throw();
		template<typename Params>
		void Trading(
					const std::string &tag,
					const char *str,
					const Params &params)
				throw() {
			Trader::Log::Trading(tag, str, params);
		}
	};

	//////////////////////////////////////////////////////////////////////////

}
