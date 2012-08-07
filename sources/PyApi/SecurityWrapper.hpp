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
			Assert(!m_askSizeGetter);
			Assert(!m_bidSizeGetter);

			boost::function<::Security::Qty()> askSizeGetter;
			boost::function<::Security::Qty()> bidSizeGetter;

			switch (level2DataSource) {
				case MARKET_DATA_SOURCE_IQFEED:
					askSizeGetter = boost::bind(&::Security::GetAskSizeIqFeed, security.get());
					bidSizeGetter = boost::bind(&::Security::GetBidSizeIqFeed, security.get());
					break;
				case MARKET_DATA_SOURCE_INTERACTIVE_BROKERS:
					askSizeGetter = boost::bind(&::Security::GetAskSizeIb, security.get());
					bidSizeGetter = boost::bind(&::Security::GetBidSizeIb, security.get());
					break;
				default:
					AssertFail("Unknown market data source.");
			}

			m_algo = &algo;
			m_security = security;
			m_askSizeGetter = askSizeGetter;
			m_bidSizeGetter = bidSizeGetter;

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
		double GetAskSize() const {
			return m_askSizeGetter();
		}

		int GetBidScaled() const {
			return int(m_security->GetBidScaled());
		}
		double GetBid() const {
			return m_security->GetBid();
		}
		double GetBidSize() const {
			return m_bidSizeGetter();
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

		boost::function<::Security::Qty()> m_askSizeGetter;
		boost::function<::Security::Qty()> m_bidSizeGetter;

	};

	//////////////////////////////////////////////////////////////////////////

} }
