/**************************************************************************
 *   Created: 2013/01/10 16:55:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "PositionExport.hpp"
#include "Detail.hpp"

using namespace trdk;
using namespace trdk::PyApi;
using namespace trdk::PyApi::Detail;

namespace py = boost::python;

//////////////////////////////////////////////////////////////////////////

namespace {

	template<typename PyApiImpl>
	py::object GetPyPosition(trdk::Position &source) {
		auto *const pyApiImpl = dynamic_cast<PyApiImpl *>(&source);
		if (!pyApiImpl) {
			AssertFail("Use cache for Python objects.");
			throw Error("Failed to export position");
		} else {
			return pyApiImpl->GetExport().GetSelf();
		}
	}

}

py::object PyApi::Export(trdk::Position &position) {
 	static_assert(Position::numberOfTypes == 2, "Position type list changed.");
 	switch (position.GetType()) {
  		case Position::TYPE_LONG:
  			return GetPyPosition<PyApi::LongPosition>(position);
  		case Position::TYPE_SHORT:
  			return GetPyPosition<PyApi::ShortPosition>(position);
 		default:
 			AssertNe(int(Position::numberOfTypes), int(position.GetType()));
 			throw Lib::Exception("Unknown position type");
 	}
}

Position & PyApi::ExtractPosition(const py::object &position) {
	try {
		Assert(position);
		PositionExport &positionExport
			= py::extract<PositionExport &>(position);
		return positionExport.GetPosition();
	} catch (const py::error_already_set &) {
		throw GetPythonClientException("Failed to extract position");
	}
}

//////////////////////////////////////////////////////////////////////////

PositionExport::PositionExport(
			const boost::shared_ptr<Position> &position)
		: m_positionRefHolder(position),
		m_position(&*m_positionRefHolder) {
	//...//
}

void PositionExport::ExportClass(const char *className) {

	py::class_<PositionExport>(className, py::no_init)

		.add_property("type", &PositionExport::GetTypeStr)

		.add_property("isCompleted", &PositionExport::IsCompleted)

		.add_property("isOpened", &PositionExport::IsOpened)
		.add_property("isClosed", &PositionExport::IsClosed)

		.add_property("isError", &PositionExport::IsError)
		.add_property("isCanceled", &PositionExport::IsCanceled)

		.add_property("hasActiveOrders", &PositionExport::HasActiveOrders)
		.add_property(
			"hasActiveOpenOrders",
			&PositionExport::HasActiveOpenOrders)
		.add_property(
			"hasActiveCloseOrders",
			&PositionExport::HasActiveCloseOrders)

		.add_property("planedQty", &PositionExport::GetPlanedQty)

		.add_property("openStartPrice", &PositionExport::GetOpenStartPrice)
		.add_property("openOrderId", &PositionExport::GetOpenOrderId)
		.add_property("openedQty", &PositionExport::GetOpenedQty)
		.add_property("openPrice", &PositionExport::GetOpenPrice)

		.add_property("notOpenedQty", &PositionExport::GetNotOpenedQty)
		.add_property("activeQty", &PositionExport::GetActiveQty)

		.add_property("closeOrderId", &PositionExport::GetCloseOrderId)
		.add_property("closeStartPrice", &PositionExport::GetCloseStartPrice)
		.add_property("closePrice", &PositionExport::GetClosePrice)
		.add_property("closedQty", &PositionExport::GetClosedQty)

		.add_property("commission", &PositionExport::GetCommission)

		.add_property("security", &PositionExport::GetSecurity)

		.def("openAtMarketPrice", &PositionExport::OpenAtMarketPrice)
		.def("openAtMarketPrice", &PositionExport::OpenAtMarketPriceByIceberg)
		.def("open", &PositionExport::Open)
		.def("open", &PositionExport::OpenByIceberg)
		.def(
			"openAtMarketPriceWithStopPrice",
			&PositionExport::OpenAtMarketPriceWithStopPrice)
		.def(
			"openAtMarketPriceWithStopPrice",
			&PositionExport::OpenAtMarketPriceWithStopPriceByIceberg)
		.def("openOrCancel", &PositionExport::OpenOrCancel)

		.def("closeAtMarketPrice", &PositionExport::CloseAtMarketPrice)
		.def("closeAtMarketPrice", &PositionExport::CloseAtMarketPriceByIceberg)
		.def("close", &PositionExport::Close)
		.def("close", &PositionExport::CloseByIceberg)
		.def(
			"closeAtMarketPriceWithStopPrice",
			&PositionExport::CloseAtMarketPriceWithStopPrice)
		.def(
			"closeAtMarketPriceWithStopPrice",
			&PositionExport::CloseAtMarketPriceWithStopPriceByIceberg)
		.def("closeOrCancel", &PositionExport::CloseOrCancel)

		.def("cancelAtMarketPrice", &PositionExport::CancelAtMarketPrice)
		.def(
			"cancelAtMarketPrice",
			&PositionExport::CancelAtMarketPriceByIceberg)
		.def("cancelAllOrders", &PositionExport::CancelAllOrders);
		
}

Position & PositionExport::GetPosition() {
	return *m_position;
}

const Position & PositionExport::GetPosition() const {
	return const_cast<PositionExport *>(this)->GetPosition();
}

void PositionExport::ResetRefHolder() throw() {
	Assert(m_positionRefHolder);
	m_positionRefHolder.reset();
}

bool PositionExport::IsCompleted() const {
	return GetPosition().IsCompleted();
}

bool PositionExport::IsOpened() const {
	return GetPosition().IsOpened();
}

bool PositionExport::IsClosed() const {
	return GetPosition().IsClosed();
}

bool PositionExport::IsError() const {
	return GetPosition().IsError();
}

bool PositionExport::IsCanceled() const {
	return GetPosition().IsCanceled();
}

bool PositionExport::HasActiveOrders() const {
	return GetPosition().HasActiveOrders();
}

bool PositionExport::HasActiveOpenOrders() const {
	return GetPosition().HasActiveOpenOrders();
}

bool PositionExport::HasActiveCloseOrders() const {
	return GetPosition().HasActiveCloseOrders();
}

const char * PositionExport::GetTypeStr() const {
	return GetPosition().GetTypeStr().c_str();
}

Qty PositionExport::GetPlanedQty() const {
	return GetPosition().GetPlanedQty();
}

ScaledPrice PositionExport::GetOpenStartPrice() const {
	return GetPosition().GetOpenStartPrice();
}

OrderId PositionExport::GetOpenOrderId() const {
	return GetPosition().GetOpenOrderId();
}

Qty PositionExport::GetOpenedQty() const {
	return GetPosition().GetOpenedQty();
}

ScaledPrice PositionExport::GetOpenPrice() const {
	return GetPosition().GetOpenPrice();
}

Qty PositionExport::GetNotOpenedQty() const {
	return GetPosition().GetNotOpenedQty();
}

Qty PositionExport::GetActiveQty() const {
	return GetPosition().GetActiveQty();
}

OrderId PositionExport::GetCloseOrderId() const {
	return GetPosition().GetCloseOrderId();
}

ScaledPrice PositionExport::GetCloseStartPrice() const {
	return GetPosition().GetCloseStartPrice();
}

ScaledPrice PositionExport::GetClosePrice() const {
	return GetPosition().GetClosePrice();
}

Qty PositionExport::GetClosedQty() const {
	return GetPosition().GetClosedQty();
}

ScaledPrice PositionExport::GetCommission() const {
	return GetPosition().GetCommission();
}

py::object PositionExport::GetSecurity() {
	return PyApi::Export(GetPosition().GetSecurity());
}

OrderId PositionExport::OpenAtMarketPrice() {
	return GetPosition().OpenAtMarketPrice();
}

OrderId PositionExport::OpenAtMarketPriceByIceberg(Qty displaySize) {
	return GetPosition().OpenAtMarketPrice(displaySize);
}

OrderId PositionExport::Open(ScaledPrice price) {
	return GetPosition().Open(price);
}

OrderId PositionExport::OpenByIceberg(ScaledPrice price, Qty displaySize) {
	return GetPosition().Open(price, displaySize);
}

OrderId PositionExport::OpenAtMarketPriceWithStopPrice(ScaledPrice stopPrice) {
	return GetPosition().OpenAtMarketPriceWithStopPrice(stopPrice);
}

OrderId PositionExport::OpenAtMarketPriceWithStopPriceByIceberg(
			ScaledPrice stopPrice,
			Qty displaySize) {
	return GetPosition().OpenAtMarketPriceWithStopPrice(stopPrice, displaySize);
}

OrderId PositionExport::OpenOrCancel(ScaledPrice price) {
	return GetPosition().OpenOrCancel(price);
}

OrderId PositionExport::CloseAtMarketPrice() {
	return GetPosition().CloseAtMarketPrice(Position::CLOSE_TYPE_NONE);
}

OrderId PositionExport::CloseAtMarketPriceByIceberg(Qty displaySize) {
	return GetPosition().CloseAtMarketPrice(
		Position::CLOSE_TYPE_NONE,
		displaySize);
}

OrderId PositionExport::Close(ScaledPrice price) {
	return GetPosition().Close(Position::CLOSE_TYPE_NONE, price);
}

OrderId PositionExport::CloseByIceberg(ScaledPrice price, Qty displaySize) {
	return GetPosition().Close(Position::CLOSE_TYPE_NONE, price, displaySize);
}

OrderId PositionExport::CloseAtMarketPriceWithStopPrice(ScaledPrice stopPrice) {
	return GetPosition().CloseAtMarketPriceWithStopPrice(
		Position::CLOSE_TYPE_NONE,
		stopPrice);
}

OrderId PositionExport::CloseAtMarketPriceWithStopPriceByIceberg(
			ScaledPrice stopPrice,
			Qty displaySize) {
	return GetPosition().CloseAtMarketPriceWithStopPrice(
		Position::CLOSE_TYPE_NONE,
		stopPrice,
		displaySize);
}

OrderId PositionExport::CloseOrCancel(ScaledPrice price) {
	return GetPosition().CloseOrCancel(Position::CLOSE_TYPE_NONE, price);
}

bool PositionExport::CancelAtMarketPrice() {
	return GetPosition().CancelAtMarketPrice(Position::CLOSE_TYPE_NONE);
}

bool PositionExport::CancelAtMarketPriceByIceberg(Qty displaySize) {
	return GetPosition().CancelAtMarketPrice(
		Position::CLOSE_TYPE_NONE,
		displaySize);
}

bool PositionExport::CancelAllOrders() {
	return GetPosition().CancelAllOrders();
}

//////////////////////////////////////////////////////////////////////////
