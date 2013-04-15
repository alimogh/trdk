/**************************************************************************
 *   Created: 2013/01/10 20:09:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "SecurityExport.hpp"
#include "Core/Security.hpp"
#include "Detail.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::PyApi;
using namespace trdk::PyApi::Detail;
namespace py = boost::python;

//////////////////////////////////////////////////////////////////////////

SecurityInfoExport::SecurityInfoExport(const Security &security)
		: m_security(&security) {
	//...//
}

void SecurityInfoExport::ExportClass(const char *className) {

	py::class_<SecurityInfoExport>(className,  py::no_init)

		.add_property("symbol", &SecurityInfoExport::GetSymbol)
		.add_property("fullSymbol", &SecurityInfoExport::GetFullSymbol)
		.add_property("currency", &SecurityInfoExport::GetCurrency)

		.add_property("priceScale", &SecurityInfoExport::GetPriceScale)
		.def("scalePrice", &SecurityInfoExport::ScalePrice)
		.def("descalePrice", &SecurityInfoExport::DescalePrice)

		.add_property("lastPrice", &SecurityInfoExport::GetLastPriceScaled)
		.add_property("lastSize", &SecurityInfoExport::GetLastQty)

		.add_property("askPrice", &SecurityInfoExport::GetAskPriceScaled)
		.add_property("askSize", &SecurityInfoExport::GetAskQty)

		.add_property("bidPrice", &SecurityInfoExport::GetBidPriceScaled)
		.add_property("bidSize", &SecurityInfoExport::GetBidQty);

}

const Security & SecurityInfoExport::GetSecurity() const {
	return *m_security;
}

py::str SecurityInfoExport::GetSymbol() const {
	return m_security->GetSymbol().c_str();
}

py::str SecurityInfoExport::GetFullSymbol() const {
	return m_security->GetFullSymbol().c_str();
}

py::str SecurityInfoExport::GetCurrency() const {
	return m_security->GetCurrency();
}

ScaledPrice SecurityInfoExport::GetPriceScale() const {
	return m_security->GetPriceScale();
}
ScaledPrice SecurityInfoExport::ScalePrice(double price) const {
	return int(m_security->ScalePrice(price));
}
double SecurityInfoExport::DescalePrice(ScaledPrice price) const {
	return m_security->DescalePrice(price);
}

ScaledPrice SecurityInfoExport::GetLastPriceScaled() const {
	return m_security->GetLastPriceScaled();
}
Qty SecurityInfoExport::GetLastQty() const {
	return m_security->GetLastQty();
}

ScaledPrice SecurityInfoExport::GetAskPriceScaled() const {
	return m_security->GetAskPriceScaled();
}
Qty SecurityInfoExport::GetAskQty() const {
	return m_security->GetAskQty();
}

ScaledPrice SecurityInfoExport::GetBidPriceScaled() const {
	return m_security->GetBidPriceScaled();
}
Qty SecurityInfoExport::GetBidQty() const {
	return m_security->GetBidQty();
}

//////////////////////////////////////////////////////////////////////////

SecurityExport::SecurityExport(Security &security)
		: SecurityInfoExport(security) {
	//...//
}

void SecurityExport::ExportClass(const char *className) {
	typedef py::class_<
			SecurityExport,
			py::bases<SecurityInfoExport>,
			boost::noncopyable>
		Export;
	Export(className, py::no_init)
		.def("cancelOrder", &SecurityExport::CancelOrder)
		.def("cancelAllOrders", &SecurityExport::CancelAllOrders);
}

Security & SecurityExport::GetSecurity() {
	return const_cast<Security &>(SecurityInfoExport::GetSecurity());
}

void SecurityExport::CancelOrder(int orderId) {
	GetSecurity().CancelOrder(orderId);
}

void SecurityExport::CancelAllOrders() {
	GetSecurity().CancelAllOrders();
}

//////////////////////////////////////////////////////////////////////////

py::object PyApi::Export(const Security &security) {
	try {
		return py::object(SecurityInfoExport(security));
	} catch (const py::error_already_set &) {
		LogPythonClientException();
		throw Error("Failed to export security info");
	}
}

//////////////////////////////////////////////////////////////////////////
