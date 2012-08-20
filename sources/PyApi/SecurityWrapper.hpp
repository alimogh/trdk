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

		int GetPriceScale() const {
			return m_security->GetPriceScale();
		}
		int ScalePrice(double price) const {
			return int(m_security->ScalePrice(price));
		}
		double DescalePrice(int price) const {
			return m_security->DescalePrice(price);
		}

		int GetLastPriceScaled() const {
			return int(m_security->GetLastPriceScaled());
		}
		double GetLastPrice() const {
			return m_security->GetLastPrice();
		}
		size_t GetLastSize() const {
			return m_security->GetLastSize();
		}

		int GetAskPriceScaled() const {
			return int(m_security->GetAskPriceScaled());
		}
		double GetAskPrice() const {
			return m_security->GetAskPrice();
		}
		size_t GetAskSize() const {
			return m_security->GetAskSize();
		}
		double GetLevel2AskSize() const {
			return m_level2AskSizeGetter();
		}

		int GetBidPriceScaled() const {
			return int(m_security->GetBidPriceScaled());
		}
		double GetBidPrice() const {
			return m_security->GetBidPrice();
		}
		size_t GetBidSize() const {
			return m_security->GetBidSize();
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
