/**************************************************************************
 *   Created: 2013/01/10 20:09:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "SecurityExport.hpp"
#include "Core/Security.hpp"

using namespace Trader;
using namespace Trader::PyApi;
namespace py = boost::python;

//////////////////////////////////////////////////////////////////////////

ConstSecurityExport::ConstSecurityExport(
			const boost::shared_ptr<const Security> &security)
		: m_security(security) {
	//...//
}

py::str ConstSecurityExport::GetSymbol() const {
	return m_security->GetSymbol().c_str();
}

py::str ConstSecurityExport::GetFullSymbol() const {
	return m_security->GetFullSymbol().c_str();
}

py::str ConstSecurityExport::GetCurrency() const {
	return m_security->GetCurrency();
}

ScaledPrice ConstSecurityExport::GetPriceScale() const {
	return m_security->GetPriceScale();
}
ScaledPrice ConstSecurityExport::ScalePrice(double price) const {
	return int(m_security->ScalePrice(price));
}
double ConstSecurityExport::DescalePrice(ScaledPrice price) const {
	return m_security->DescalePrice(price);
}

ScaledPrice ConstSecurityExport::GetLastPriceScaled() const {
	return m_security->GetLastPriceScaled();
}
Qty ConstSecurityExport::GetLastQty() const {
	return m_security->GetLastQty();
}

ScaledPrice ConstSecurityExport::GetAskPriceScaled() const {
	return m_security->GetAskPriceScaled();
}
Qty ConstSecurityExport::GetAskQty() const {
	return m_security->GetAskQty();
}

ScaledPrice ConstSecurityExport::GetBidPriceScaled() const {
	return m_security->GetBidPriceScaled();
}
Qty ConstSecurityExport::GetBidQty() const {
	return m_security->GetBidQty();
}

//////////////////////////////////////////////////////////////////////////

SecurityExport::SecurityExport(const boost::shared_ptr<Security> &security)
		: ConstSecurityExport(security),
		m_security(security) {
	//...//
}

void SecurityExport::Export(const char *className) {

	py::class_<SecurityExport, boost::noncopyable>(className,  py::no_init)

		.add_property("symbol", &SecurityExport::GetSymbol)
		.add_property("fullSymbol", &SecurityExport::GetFullSymbol)
		.add_property("currency", &SecurityExport::GetCurrency)

		.add_property("priceScale", &SecurityExport::GetPriceScale)
		.def("scalePrice", &SecurityExport::ScalePrice)
		.def("descalePrice", &SecurityExport::DescalePrice)

		.add_property("lastPrice", &SecurityExport::GetLastPriceScaled)
		.add_property("lastSize", &SecurityExport::GetLastQty)

		.add_property("askPrice", &SecurityExport::GetAskPriceScaled)
		.add_property("askSize", &SecurityExport::GetAskQty)

		.add_property("bidPrice", &SecurityExport::GetBidPriceScaled)
		.add_property("bidSize", &SecurityExport::GetBidQty)

		.def("cancelOrder", &SecurityExport::CancelOrder)
		.def("cancelAllOrders", &SecurityExport::CancelAllOrders);

}

boost::shared_ptr<Security> SecurityExport::GetSecurity() {
	return m_security;
}

void SecurityExport::CancelOrder(int orderId) {
	m_security->CancelOrder(orderId);
}

void SecurityExport::CancelAllOrders() {
	m_security->CancelAllOrders();
}

//////////////////////////////////////////////////////////////////////////
