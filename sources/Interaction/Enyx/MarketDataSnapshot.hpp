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

		typedef Security::OrderId OrderId;
		typedef Security::Qty Qty;
		typedef Security::ScaledPrice ScaledPrice;

	private:

		typedef std::map<ScaledPrice, Qty> Ask;
		typedef std::map<ScaledPrice, Qty, std::greater<Qty>> Bid;

	public:

		explicit MarketDataSnapshot(
				const std::string &symbol,
				bool handlFirstLimitUpdate);

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
				OrderId,
				const boost::posix_time::ptime &,
				Qty prevQty,
				Qty newQty,
				double price);
		
		void ChangeOrder(
				bool isBuy,
				OrderId,
				const boost::posix_time::ptime &,
				Qty prevQty,
				Qty newQty,
				double price);

		void DelOrder(
				bool isBuy,
				OrderId,
				const boost::posix_time::ptime &,
				Qty,
				double price);

	protected:

		void UpdateBid(const boost::posix_time::ptime &time);
		void UpdateAsk(const boost::posix_time::ptime &time);

	private:

		const bool m_handlFirstLimitUpdate;

		std::string m_symbol;
		mutable boost::shared_ptr<Security> m_security;

		Ask m_ask;
		Bid m_bid;

	};

} } }
