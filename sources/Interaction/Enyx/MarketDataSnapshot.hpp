/**************************************************************************
 *   Created: 2012/09/18 23:58:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Security.hpp"

namespace Trader {  namespace Interaction { namespace Enyx {

	class MarketDataSnapshot : private boost::noncopyable {

	public:

		typedef Security::Qty Qty;
		typedef Security::ScaledPrice ScaledPrice;

	private:

		typedef std::map<ScaledPrice, Qty> Ask;
		typedef std::map<ScaledPrice, Qty, std::greater<Qty>> Bid;

	public:

		explicit MarketDataSnapshot(
				const std::string &symbol,
				bool handleFirstLimitUpdate);

	public:

		void Subscribe(const boost::shared_ptr<Security> &) const;

	public:

		const std::string & GetSymbol() const {
			return m_symbol;
		}

	public:

		void AddOrder(
				bool isBuy,
				const boost::posix_time::ptime &,
				Qty,
				double price);
		
		void ExecOrder(
				bool isBuy,
				const boost::posix_time::ptime &,
				Qty prevQty,
				Qty newQty,
				double price);
		
		void ChangeOrder(
				bool isBuy,
				const boost::posix_time::ptime &,
				Qty prevQty,
				Qty newQty,
				double price);
		void ChangeOrder(
				bool isBuy,
				const boost::posix_time::ptime &,
				Qty prevQty,
				Qty newQty,
				double prevPrice,
				double newPrice);

		void DelOrder(
				bool isBuy,
				const boost::posix_time::ptime &,
				Qty,
				double price);

		void SignalNewTrade(
				bool isBuy,
				const boost::posix_time::ptime &time,
				Qty qty,
				double price);

	protected:

		void UpdateBid(const boost::posix_time::ptime &time);
		void UpdateAsk(const boost::posix_time::ptime &time);

		void SignalNewTrade(
				bool isBuy,
				const boost::posix_time::ptime &,
				Qty,
				ScaledPrice);

	private:

		const bool m_handleFirstLimitUpdate;

		std::string m_symbol;
		mutable boost::shared_ptr<Security> m_security;

		Ask m_ask;
		Bid m_bid;

	};

} } }
