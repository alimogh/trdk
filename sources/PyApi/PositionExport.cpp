/**************************************************************************
 *   Created: 2013/01/10 16:55:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "PositionExport.hpp"
#include "Position.hpp"

using namespace Trader;
using namespace Trader::PyApi;
using namespace Trader::PyApi::Detail;

namespace py = boost::python;

//////////////////////////////////////////////////////////////////////////

namespace {

	typedef py::init<
			PyApi::Strategy &,
			int /*qty*/,
			double /*startPrice*/>
		PositionPyInit;

}

//////////////////////////////////////////////////////////////////////////

PositionExport::PositionExport(Position &position)
		: m_position(&position) {
	//...//
}

void PositionExport::Export(const char *className) {

	py::class_<PositionExport>(className, py::no_init)

		.add_property("type", &PositionExport::GetTypeStr)

		.add_property("isCompleted", &PositionExport::IsCompleted)

		.add_property("isOpened", &PositionExport::IsOpened)
		.add_property("isClosed", &PositionExport::IsClosed)

		.add_property("hasActiveOrders", &PositionExport::HasActiveOrders)

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

		.def("openAtMarketPrice", &PositionExport::OpenAtMarketPrice)
		.def("open", &PositionExport::Open)
		.def(
			"openAtMarketPriceWithStopPrice",
			&PositionExport::OpenAtMarketPriceWithStopPrice)
		.def("openOrCancel", &PositionExport::OpenOrCancel)

		.def("closeAtMarketPrice", &PositionExport::CloseAtMarketPrice)
		.def("close", &PositionExport::Close)
		.def(
			"closeAtMarketPriceWithStopPrice",
			&PositionExport::CloseAtMarketPriceWithStopPrice)
		.def("closeOrCancel", &PositionExport::CloseOrCancel)

		.def("cancelAtMarketPrice", &PositionExport::CancelAtMarketPrice)
		.def("cancelAllOrders", &PositionExport::CancelAllOrders);
		
}


Position & PositionExport::GetPosition() {
	return *m_position;
}

const Position & PositionExport::GetPosition() const {
	return const_cast<PositionExport *>(this)->GetPosition();
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

OrderId PositionExport::OpenAtMarketPrice() {
	return GetPosition().OpenAtMarketPrice();
}

OrderId PositionExport::Open(ScaledPrice price) {
	return GetPosition().Open(price);
}

OrderId PositionExport::OpenAtMarketPriceWithStopPrice(ScaledPrice stopPrice) {
	return GetPosition().OpenAtMarketPriceWithStopPrice(stopPrice);
}

OrderId PositionExport::OpenOrCancel(ScaledPrice price) {
	return GetPosition().OpenOrCancel(price);
}

OrderId PositionExport::CloseAtMarketPrice() {
	return GetPosition().CloseAtMarketPrice(Position::CLOSE_TYPE_NONE);
}

OrderId PositionExport::Close(ScaledPrice price) {
	return GetPosition().Close(Position::CLOSE_TYPE_NONE, price);
}

OrderId PositionExport::CloseAtMarketPriceWithStopPrice(ScaledPrice stopPrice) {
	return GetPosition().CloseAtMarketPriceWithStopPrice(
		Position::CLOSE_TYPE_NONE,
		stopPrice);
}

OrderId PositionExport::CloseOrCancel(ScaledPrice price) {
	return GetPosition().CloseOrCancel(Position::CLOSE_TYPE_NONE, price);
}

bool PositionExport::CancelAtMarketPrice() {
	return GetPosition().CancelAtMarketPrice(Position::CLOSE_TYPE_NONE);
}

bool PositionExport::CancelAllOrders() {
	return GetPosition().CancelAllOrders();
}

//////////////////////////////////////////////////////////////////////////

LongPositionExport::LongPositionExport(Trader::Position &position)
		: PositionExport(position) {
	Assert(dynamic_cast<Trader::LongPosition *>(&position));
}

namespace boost { namespace python {

	template<>
	struct has_back_reference<PyApi::LongPosition> : public mpl::true_ {
		//...//
	};

} }

void LongPositionExport::Export(const char *className) {
	typedef py::class_<
			PyApi::LongPosition,
			py::bases<PositionExport>,
			PythonToCoreTransitHolder<PyApi::LongPosition>,
			boost::noncopyable>
		LongPositionImport;
	LongPositionImport(className, PositionPyInit());
}

//////////////////////////////////////////////////////////////////////////

ShortPositionExport::ShortPositionExport(Trader::Position &position)
		: PositionExport(position) {
	Assert(dynamic_cast<Trader::ShortPosition *>(&position));
}

namespace boost { namespace python {

	template<>
	struct has_back_reference<PyApi::ShortPosition> : public mpl::true_ {
		//...//
	};

} }

void ShortPositionExport::Export(const char *className) {
	typedef py::class_<
			PyApi::ShortPosition,
			py::bases<PositionExport>,
			PythonToCoreTransitHolder<PyApi::ShortPosition>,
			boost::noncopyable>
		ShortPositionImport;
	ShortPositionImport(className, PositionPyInit());
}

//////////////////////////////////////////////////////////////////////////
