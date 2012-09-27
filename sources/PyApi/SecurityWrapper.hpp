/**************************************************************************
 *   Created: 2012/08/07 00:13:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "PositionWrapper.hpp"
#include "Core/Security.hpp"

namespace Trader { namespace PyApi { namespace Wrappers {

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

		void Init(const Trader::Algo &algo, boost::shared_ptr<Trader::Security> security) {
			Assert(!m_algo);
			Assert(!m_security);
			m_algo = &algo;
			m_security = security;
		}

		boost::shared_ptr<Trader::Security> GetSecurity() {
			return m_security;
		}

		const Trader::Algo & GetAlgo() const {
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
		size_t GetLastQty() const {
			return m_security->GetLastQty();
		}

		int GetAskPriceScaled() const {
			return int(m_security->GetAskPriceScaled());
		}
		double GetAskPrice() const {
			return m_security->GetAskPrice();
		}
		size_t GetAskQty() const {
			return m_security->GetAskQty();
		}

		int GetBidPriceScaled() const {
			return int(m_security->GetBidPriceScaled());
		}
		double GetBidPrice() const {
			return m_security->GetBidPrice();
		}
		size_t GetBidQty() const {
			return m_security->GetBidQty();
		}

	public:

		void CancelOrder(int orderId) {
			m_security->CancelOrder(orderId);
		}

		void CancelAllOrders() {
			m_security->CancelAllOrders();
		}

	private:

		const Trader::Algo *m_algo;
		boost::shared_ptr<Trader::Security> m_security;

	};

	//////////////////////////////////////////////////////////////////////////

} } }
