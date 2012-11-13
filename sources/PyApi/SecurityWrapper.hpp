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

		explicit Security(boost::shared_ptr<Trader::Security> security)
				: m_security(security) {
			//...//
		}

	public:

		Trader::Security & GetSecurity() {
			return *m_security;
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
			return int(m_security->GetAskPriceScaled(1));
		}
		double GetAskPrice() const {
			return m_security->GetAskPrice(1);
		}
		size_t GetAskQty() const {
			return m_security->GetAskQty(1);
		}

		int GetBidPriceScaled() const {
			return int(m_security->GetBidPriceScaled(1));
		}
		double GetBidPrice() const {
			return m_security->GetBidPrice(1);
		}
		size_t GetBidQty() const {
			return m_security->GetBidQty(1);
		}

	public:

		void CancelOrder(int orderId) {
			m_security->CancelOrder(orderId);
		}

		void CancelAllOrders() {
			m_security->CancelAllOrders();
		}

	private:

		boost::shared_ptr<Trader::Security> m_security;

	};

	//////////////////////////////////////////////////////////////////////////

} } }
