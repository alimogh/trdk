/**************************************************************************
 *   Created: 2012/08/07 00:13:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "PositionWrapper.hpp"
#include "MarketDataSource.hpp"
#include "Core/Security.hpp"

class Algo;

namespace PyApi { namespace Wrappers {

	//////////////////////////////////////////////////////////////////////////

	class Security : private boost::noncopyable {

	public:

		Security()
				: m_algo(nullptr) {
			//...//
		}

		~Security() {
			//...//
		}

	public:

		void Init(
					const ::Algo &algo,
					boost::shared_ptr<::Security> security,
					PyApi::MarketDataSource level2DataSource) {

			Assert(!m_algo);
			Assert(!m_security);
			Assert(!m_level2AskSizeGetter);
			Assert(!m_level2BidSizeGetter);

			boost::function<::Security::Qty()> level2AskSizeGetter;
			boost::function<::Security::Qty()> level2BidSizeGetter;

			switch (level2DataSource) {
				case MARKET_DATA_SOURCE_IQFEED:
					level2AskSizeGetter = boost::bind(&::Security::GetLevel2AskSizeIqFeed, security.get());
					level2BidSizeGetter = boost::bind(&::Security::GetLevel2BidSizeIqFeed, security.get());
					break;
				case MARKET_DATA_SOURCE_INTERACTIVE_BROKERS:
					level2AskSizeGetter = boost::bind(&::Security::GetLevel2AskSizeIb, security.get());
					level2BidSizeGetter = boost::bind(&::Security::GetLevel2BidSizeIb, security.get());
					break;
				default:
					AssertFail("Unknown market data source.");
			}

			m_algo = &algo;
			m_security = security;
			m_level2AskSizeGetter = level2AskSizeGetter;
			m_level2BidSizeGetter = level2BidSizeGetter;

		}

		boost::shared_ptr<::Security> GetSecurity() {
			return m_security;
		}

		const ::Algo & GetAlgo() const {
			Assert(m_algo);
			return *m_algo;
		}

	public:

		boost::python::str GetSymbol() const {
			return m_security->GetSymbol().c_str();
		}

		boost::python::str GetFullSymbol() const {
			return m_security->GetFullSymbol().c_str();
		}

		boost::python::str GetCurrency() const {
			return m_security->GetCurrency();
		}

		int GetScale() const {
			return m_security->GetScale();
		}
		int Scale(double price) const {
			return int(m_security->Scale(price));
		}
		double Descale(int price) const {
			return m_security->Descale(price);
		}

		int GetLastScaled() const {
			return int(m_security->GetLastScaled());
		}
		double GetLast() const {
			return m_security->GetLast();
		}

		int GetAskScaled() const {
			return int(m_security->GetAskScaled());
		}
		double GetAsk() const {
			return m_security->GetAsk();
		}
		double GetLevel2AskSize() const {
			return m_level2AskSizeGetter();
		}

		int GetBidScaled() const {
			return int(m_security->GetBidScaled());
		}
		double GetBid() const {
			return m_security->GetBid();
		}
		double GetLevel2BidSize() const {
			return m_level2BidSizeGetter();
		}

	public:

		void CancelOrder(int orderId) {
			m_security->CancelOrder(orderId);
		}
	
		void CancelAllOrders() {
			m_security->CancelAllOrders();
		}

	private:

		const ::Algo *m_algo;
		boost::shared_ptr<::Security> m_security;

		boost::function<::Security::Qty()> m_level2AskSizeGetter;
		boost::function<::Security::Qty()> m_level2BidSizeGetter;

	};

	//////////////////////////////////////////////////////////////////////////

} }
