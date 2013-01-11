/**************************************************************************
 *   Created: 2013/01/10 20:07:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

namespace Trader { namespace PyApi {

	//////////////////////////////////////////////////////////////////////////
	
	class ConstSecurityExport : private boost::noncopyable {

	public:

		explicit ConstSecurityExport(const boost::shared_ptr<const Security> &);

	public:

		boost::python::str GetSymbol() const;
		boost::python::str GetFullSymbol() const;

		boost::python::str GetCurrency() const;

		ScaledPrice GetPriceScale() const;
		ScaledPrice ScalePrice(double price) const;
		double DescalePrice(ScaledPrice price) const;

		ScaledPrice GetLastPriceScaled() const;
		Qty GetLastQty() const;

		ScaledPrice GetAskPriceScaled() const;
		Qty GetAskQty() const;

		ScaledPrice GetBidPriceScaled() const;
		Qty GetBidQty() const;

	private:

		const boost::shared_ptr<const Security> m_security;

	};

	//////////////////////////////////////////////////////////////////////////

	class SecurityExport : public ConstSecurityExport {

	public:

		explicit SecurityExport(const boost::shared_ptr<Security> &);

	public:

		static void Export(const char *className);

	public:

		boost::shared_ptr<Security> GetSecurity();

	public:

		void CancelOrder(int orderId);
		void CancelAllOrders();

	private:

		const boost::shared_ptr<Security> m_security;

	};

	//////////////////////////////////////////////////////////////////////////

} }
