/**************************************************************************
 *   Created: 2013/01/10 20:07:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace PyApi {

	//////////////////////////////////////////////////////////////////////////
	
	class SecurityInfoExport {

	public:

		explicit SecurityInfoExport(const Security &);

	public:

		static void ExportClass(const char *className);

	public:

		const Security & GetSecurity() const;

	public:

		boost::python::str GetSymbol() const;
		boost::python::str GetExchange() const;
		boost::python::str GetPrimaryExchange() const;

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

		const Security *m_security;

	};

	//////////////////////////////////////////////////////////////////////////

	class SecurityExport : public SecurityInfoExport {

	public:

		explicit SecurityExport(Security &);

	public:

		static void ExportClass(const char *className);

	public:

		Security & GetSecurity();

	public:

		void CancelOrder(int orderId);
		void CancelAllOrders();

	};

	//////////////////////////////////////////////////////////////////////////

	boost::python::object Export(const trdk::Security &);
	boost::python::object Export(trdk::Security &);

	//////////////////////////////////////////////////////////////////////////

} }
