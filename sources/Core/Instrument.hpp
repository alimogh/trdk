/**************************************************************************
 *   Created: May 19, 2012 1:07:25 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "TradeSystem.hpp"

namespace Trader {

	class Instrument : private boost::noncopyable {

	public:

		explicit Instrument(
					boost::shared_ptr<TradeSystem> tradeSystem,
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange,
					boost::shared_ptr<const Trader::Settings> settings)
				: m_tradeSystem(tradeSystem),
				m_symbol(symbol),
				m_primaryExchange(primaryExchange),
				m_exchange(exchange),
				m_fullSymbol(Util::CreateSymbolFullStr(m_symbol, m_primaryExchange, m_exchange)),
				m_settings(settings) {
			//...//
		}
		explicit Instrument(
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange,
					boost::shared_ptr<const Trader::Settings> settings)
				: m_symbol(symbol),
				m_primaryExchange(primaryExchange),
				m_exchange(exchange),
				m_fullSymbol(Util::CreateSymbolFullStr(m_symbol, m_primaryExchange, m_exchange)),
				m_settings(settings) {
			//...//
		}

		virtual ~Instrument() {
			//...//
		}

	public:

		const std::string & GetFullSymbol() const throw() {
			return m_fullSymbol;
		}
		const std::string & GetSymbol() const throw() {
			return m_symbol;
		}
		const std::string & GetPrimaryExchange() const {
			return m_primaryExchange;
		}
		const std::string & GetExchange() const {
			return m_exchange;
		}
		//! @todo: remove!
		void SetExchange(const std::string &exchange) {
			m_exchange = exchange;
		}

	public:

		const TradeSystem & GetTradeSystem() const {
			if (!m_tradeSystem) {
				throw Trader::Lib::Exception("Instrument doesn't connected to trade system");
			}
			return *m_tradeSystem;
		}

		const Trader::Settings & GetSettings() const {
			return *m_settings;
		}

	protected:

		TradeSystem & GetTradeSystem() {
			return *m_tradeSystem;
		}

	private:

		const boost::shared_ptr<TradeSystem> m_tradeSystem;
		const std::string m_symbol;
		const std::string m_primaryExchange;
		std::string m_exchange;
		const std::string m_fullSymbol;
		const boost::shared_ptr<const Settings> m_settings;

	};

}
