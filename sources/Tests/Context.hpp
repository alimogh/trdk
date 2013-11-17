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

		virtual const trdk::Settings & GetSettings() const {
			throw trdk::Lib::MethodDoesNotImplementedError(
				"The method is not implemented for testing module");
		}
		
		virtual trdk::MarketDataSource & GetMarketDataSource() {
			throw trdk::Lib::MethodDoesNotImplementedError(
				"The method is not implemented for testing module");
		}

		virtual const trdk::MarketDataSource & GetMarketDataSource() const {
			throw trdk::Lib::MethodDoesNotImplementedError(
				"The method is not implemented for testing module");
		}

		virtual trdk::TradeSystem & GetTradeSystem() {
			throw trdk::Lib::MethodDoesNotImplementedError(
				"The method is not implemented for testing module");
		}

		virtual const trdk::TradeSystem & GetTradeSystem() const {
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
