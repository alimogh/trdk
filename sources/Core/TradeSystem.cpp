/**************************************************************************
 *   Created: 2012/07/24 10:07:02
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TradeSystem.hpp"
#include "TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;

//////////////////////////////////////////////////////////////////////////

TradeSystem::Error::Error(const char *what) throw()
		: Base::Error(what) {
	//...//
}

TradeSystem::OrderParamsError::OrderParamsError(
			const char *what,
			Qty,
			const trdk::OrderParams &)
		throw()
		: Error(what) {
	//...//
}

TradeSystem::SendingError::SendingError() throw()
		: Error("Failed to send data to trade system") {
	//...//
}

TradeSystem::ConnectionDoesntExistError::ConnectionDoesntExistError(
			const char *what)
		throw()
		: Error(what) {
	//...//
}

TradeSystem::UnknownAccountError::UnknownAccountError(
			const char *what)
		throw()
		: Error(what) {
	//...//
}

TradeSystem::PositionError::PositionError(
			const char *what)
		throw()
		: Error(what) {
	//...//
}

//////////////////////////////////////////////////////////////////////////

namespace {
	
	std::string FormatStringId(const std::string &tag) {
		std::string	result("TradeSystem");
		if (!tag.empty()) {
			result += '.';
			result += tag;
		}
		return std::move(result);
	}

}

class TradeSystem::Implementation : private boost::noncopyable {

public:

	const size_t m_index;

	Context &m_context;

	const std::string m_tag;
	const std::string m_stringId;

	TradeSystem::Log m_log;
	TradeSystem::TradingLog m_tradingLog;

	explicit Implementation(
			size_t index,
			Context &context,
			const std::string &tag)
		: m_index(index),
		m_context(context),
		m_tag(tag),
		m_stringId(FormatStringId(m_tag)),
		m_log(m_stringId, m_context.GetLog()),
		m_tradingLog(m_tag, m_context.GetTradingLog()) {
		//...//
	}

};

TradeSystem::TradeSystem(size_t index, Context &context, const std::string &tag)
	: m_pimpl(new Implementation(index, context, tag)) {
	//...//
}

TradeSystem::~TradeSystem() {
	delete m_pimpl;
}

size_t TradeSystem::GetIndex() const {
	return m_pimpl->m_index;
}

Context & TradeSystem::GetContext() {
	return m_pimpl->m_context;
}

const Context & TradeSystem::GetContext() const {
	return const_cast<TradeSystem *>(this)->GetContext();
}

TradeSystem::Log & TradeSystem::GetLog() const throw() {
	return m_pimpl->m_log;
}

TradeSystem::TradingLog & TradeSystem::GetTradingLog() const throw() {
	return m_pimpl->m_tradingLog;
}

const char * TradeSystem::GetStringStatus(OrderStatus code) {

	static_assert(
		numberOfOrderStatuses == 6,
		"Changed trade system order status list.");

	switch (code) {
		case TradeSystem::ORDER_STATUS_PENDIGN:
			return "pending";
		case TradeSystem::ORDER_STATUS_SUBMITTED:
			return "submitted";
		case TradeSystem::ORDER_STATUS_CANCELLED:
			return "canceled";
		case TradeSystem::ORDER_STATUS_FILLED:
			return "filled";
		case TradeSystem::ORDER_STATUS_INACTIVE:
			return "inactive";
		case TradeSystem::ORDER_STATUS_ERROR:
			return "send error";
		default:
			AssertFail("Unknown order status code");
			return "unknown";
	}

}

const std::string & TradeSystem::GetTag() const {
	return m_pimpl->m_tag;
}

const std::string & TradeSystem::GetStringId() const throw() {
	return m_pimpl->m_stringId;
}

void TradeSystem::Validate(
			Qty qty,
			const OrderParams &params,
			bool isIoc)
		const {

	if (qty == 0) {
		throw OrderParamsError("Order size can't be zero", qty, params);
	}

	if (params.displaySize && *params.displaySize > qty) {
		throw OrderParamsError(
			"Order display size can't be greater then order size",
			qty,
			params);
	}

	if (isIoc && (params.goodInSeconds || params.goodTillTime)) {
		throw OrderParamsError(
			"Good Next Seconds and Good Till Time can't be used for"
				" Immediate Or Cancel (IOC) order",
			qty,
			params);
	} else if (params.goodInSeconds && params.goodTillTime) {
		throw OrderParamsError(
			"Good Next Seconds and Good Till Time can't be used at"
				" the same time",
			qty,
			params);
	}

}

const TradeSystem::Account & TradeSystem::GetAccount() const {
	throw MethodDoesNotImplementedError(
		"Account Cash Balance not implemented");
}

TradeSystem::Position TradeSystem::GetBrokerPostion(
			const std::string &,
			const trdk::Lib::Symbol &)
		const {
	throw MethodDoesNotImplementedError("Broker Position Info not implemented");
}

void TradeSystem::ForEachBrokerPostion(
			const std::string &,
			const boost::function<bool (const Position &)> &)
		const {
	throw MethodDoesNotImplementedError("Broker Position Info not implemented");
}

void TradeSystem::Connect(const IniSectionRef &conf) {
	if (IsConnected()) {
		return;
	}
	CreateConnection(conf);
}

void TradeSystem::Test() {
	throw MethodDoesNotImplementedError(
		"Trading system does not support testing");
}

//////////////////////////////////////////////////////////////////////////

std::ostream & std::operator <<(
			std::ostream &oss,
			const TradeSystem &tradeSystem) {
	oss << tradeSystem.GetStringId();
	return oss;
}

////////////////////////////////////////////////////////////////////////////////
