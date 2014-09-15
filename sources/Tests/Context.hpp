/**************************************************************************
 *   Created: 2013/11/12 07:55:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Context.hpp"

namespace trdk { namespace Testing {

	class Context : public trdk::Context {

	public:

		virtual ~Context() {
			//...//
		}

	public:

		virtual const trdk::Settings & GetSettings() const {
			throw trdk::Lib::MethodDoesNotImplementedError(
				"The method is not implemented for testing module");
		}

		virtual size_t GetMarketDataSourcesCount() const {
			throw trdk::Lib::MethodDoesNotImplementedError(
				"The method is not implemented for testing module");
		}
		
		virtual trdk::MarketDataSource & GetMarketDataSource(size_t) {
			throw trdk::Lib::MethodDoesNotImplementedError(
				"The method is not implemented for testing module");
		}

		virtual const trdk::MarketDataSource & GetMarketDataSource(size_t) const {
			throw trdk::Lib::MethodDoesNotImplementedError(
				"The method is not implemented for testing module");
		}

		virtual void ForEachMarketDataSource(
						const boost::function<bool (const trdk::MarketDataSource &)> &)
					const {
			throw trdk::Lib::MethodDoesNotImplementedError(
				"The method is not implemented for testing module");
		}

		virtual void ForEachMarketDataSource(
						const boost::function<bool (trdk::MarketDataSource &)> &) {
			throw trdk::Lib::MethodDoesNotImplementedError(
				"The method is not implemented for testing module");
		}

		virtual size_t GetTradeSystemsCount() const {
			throw trdk::Lib::MethodDoesNotImplementedError(
				"The method is not implemented for testing module");
		}

		virtual trdk::TradeSystem & GetTradeSystem(size_t) {
			throw trdk::Lib::MethodDoesNotImplementedError(
				"The method is not implemented for testing module");
		}

		virtual const trdk::TradeSystem & GetTradeSystem(size_t) const {
			throw trdk::Lib::MethodDoesNotImplementedError(
				"The method is not implemented for testing module");
		}

	protected:

		virtual trdk::Security * FindSecurity(const trdk::Lib::Symbol &) {
			throw trdk::Lib::MethodDoesNotImplementedError(
				"The method is not implemented for testing module");
		}
		
		virtual const trdk::Security * FindSecurity(
						const trdk::Lib::Symbol &)
					const {
			throw trdk::Lib::MethodDoesNotImplementedError(
				"The method is not implemented for testing module");
		}

	};

} }
