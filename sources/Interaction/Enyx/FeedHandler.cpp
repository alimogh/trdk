/**************************************************************************
 *   Created: 2012/09/13 00:13:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "FeedHandler.hpp"
#include "Security.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;
using namespace Trader::Interaction::Enyx;

//////////////////////////////////////////////////////////////////////////

class FeedHandler::RawLog : private boost::noncopyable {

public:

	RawLog() {
		fs::path filePath = Defaults::GetRawDataLogDir();
		filePath /= "Enyx.log";
		const bool isNew = !fs::exists(filePath);
		if (isNew) {
			fs::create_directories(filePath.branch_path());
		}
		m_file.open(filePath.c_str(), std::ios::out | std::ios::trunc);
		if (!m_file) {
			Log::Error(
				TRADER_ENYX_LOG_PREFFIX "failed to open raw log file %1%.",
				filePath);
			throw Exception("Failed to open Enyx raw log file");
		}
		Log::Info(
			TRADER_ENYX_LOG_PREFFIX "opened log file for raw messages %1%.",
			filePath);
	}

	template<typename Message>
	void Append(const pt::ptime &time, Message &message) {
		m_file << (time + Util::GetEdtDiff()).time_of_day() << "\t" << message << std::endl;
	}

private:

	std::ofstream m_file;

};

////////////////////////////////////////////////////////////////////////////////

FeedHandler::FeedHandler(bool handlFirstLimitUpdate)
		: m_handlFirstLimitUpdate(handlFirstLimitUpdate),
		m_rawLog(nullptr) {
	//...//
}

FeedHandler::~FeedHandler() {
	delete m_rawLog;
}

namespace {

	bool IsBuy(const NXFeed_side_t &side) {
		switch (side) {
			case NXFEED_SIDE_BUY:
				return true;
			default:
				AssertFail("Unknown NXFeed_side_t.");
			case NXFEED_SIDE_SELL:
				return false;
		}
	}

}

void FeedHandler::HandleMessage(const nasdaqustvitch41::NXFeedOrderAdd &enyxOrder) {

	const auto &index = m_marketDataSnapshots.get<BySecurtiy>();
	std::string symbol = enyxOrder.getInstrumentName().c_str();
	boost::trim(symbol);
	const auto snapshot = index.find(symbol);
	if (snapshot == index.end()) {
		Log::Warn(
			TRADER_ENYX_LOG_PREFFIX "no Marked Data Snapshot for \"%1%\".",
			symbol);
		throw Exception("Marked Data Snapshot not found");
	}

	const auto id = enyxOrder.getOrderId();
	const Order order(
		IsBuy(enyxOrder.getBuyNSell()),
		enyxOrder.getQuantity(),
		enyxOrder.getPrice(),
		snapshot->ptr);
	Assert(m_orders.find(id) == m_orders.end());
	m_orders[id] = order;

	order.GetMarketDataSnapshot().AddOrder(
		order.IsBuy(),
		GetMessageTime(enyxOrder),
		order.GetQty(),
		order.GetPrice());

}

void FeedHandler::HandleMessage(const NXFeedOrderExecute &enyxOrder) {
	const auto id = enyxOrder.getOrderId();
	const auto execQty = enyxOrder.getExecutedQuantity();
	auto orderPos = FindOrderPos(id);
	const auto prevQty = orderPos->second.GetQty();
	if (orderPos->second.ReduceQty(execQty) <= 0) {
		const Order order = orderPos->second;
		m_orders.erase(orderPos);
		order.GetMarketDataSnapshot().ExecOrder(
			order.IsBuy(),
			GetMessageTime(enyxOrder),
			prevQty,
			order.GetQty(),
			order.GetPrice());
	} else {
		const Order &order = orderPos->second;
		order.GetMarketDataSnapshot().ExecOrder(
			order.IsBuy(),
			GetMessageTime(enyxOrder),
			prevQty,
			order.GetQty(),
			order.GetPrice());
	}
}

void FeedHandler::HandleMessage(
			const nasdaqustvitch41::NXFeedOrderExeWithPrice &enyxOrder) {
	const auto id = enyxOrder.getOrderId();
	const auto execQty = enyxOrder.getExecutedQuantity();
	auto orderPos = FindOrderPos(id);
	const auto prevQty = orderPos->second.GetQty();
	if (orderPos->second.ReduceQty(execQty) <= 0) {
		const Order order = orderPos->second;
		m_orders.erase(orderPos);
		order.GetMarketDataSnapshot().ExecOrder(
			order.IsBuy(),
			GetMessageTime(enyxOrder),
			prevQty,
			order.GetQty(),
			order.GetPrice());
	} else {
		const Order &order = orderPos->second;
		order.GetMarketDataSnapshot().ExecOrder(
			order.IsBuy(),
			GetMessageTime(enyxOrder),
			prevQty,
			order.GetQty(),
			order.GetPrice());
	}
}

void FeedHandler::HandleMessage(const NXFeedOrderReduce &enyxOrder) {
	const auto id = enyxOrder.getOrderId();
	const auto reducedQty = enyxOrder.getReducedQuantity();
	auto orderPos = FindOrderPos(id);
	const auto prevQty = orderPos->second.GetQty();
	if (orderPos->second.ReduceQty(reducedQty) <= 0) {
		const Order order = orderPos->second;
		m_orders.erase(orderPos);
		order.GetMarketDataSnapshot().ChangeOrder(
			order.IsBuy(),
			GetMessageTime(enyxOrder),
			prevQty,
			order.GetQty(),
			order.GetPrice());
	} else {
		const Order &order = orderPos->second;
		order.GetMarketDataSnapshot().ChangeOrder(
			order.IsBuy(),
			GetMessageTime(enyxOrder),
			prevQty,
			order.GetQty(),
			order.GetPrice());
	}
}

void FeedHandler::HandleMessage(const NXFeedOrderReplace &enyxOrder) {
	Assert(enyxOrder.getNewOrderId() != enyxOrder.getOrderId());
	const auto orderPos = FindOrderPos(enyxOrder.getOrderId());
	const Order oldOrder = orderPos->second;
	Order newOrder = oldOrder;
	newOrder.SetQty(enyxOrder.getNewQuantity());
	newOrder.SetPrice(enyxOrder.getNewPrice());
	m_orders.erase(orderPos);
	Assert(m_orders.find(enyxOrder.getNewOrderId()) == m_orders.end());
	m_orders[enyxOrder.getNewOrderId()] = newOrder;
	newOrder.GetMarketDataSnapshot().ChangeOrder(
		newOrder.IsBuy(),
		GetMessageTime(enyxOrder),
		oldOrder.GetQty(),
		newOrder.GetQty(),
		oldOrder.GetPrice(),
		newOrder.GetPrice());
}

void FeedHandler::HandleMessage(const NXFeedOrderDelete &enyxOrder) {
	const auto id = enyxOrder.getOrderId();
	const auto orderIt = FindOrderPos(id);
	const Order order = orderIt->second;
	m_orders.erase(orderIt);
	order.GetMarketDataSnapshot().DelOrder(
		order.IsBuy(),
		GetMessageTime(enyxOrder),
		order.GetQty(),
		order.GetPrice());
}

void FeedHandler::HandleMessage(const NXFeedMiscTime &time) {
	const auto secondsSinceMidnight = time.getSecondMidnight();
	const auto timeSinceMidnight = pt::seconds(secondsSinceMidnight);
	const auto currentTime = boost::get_system_time();
	auto serverTime = currentTime + Util::GetEdtDiff();
	serverTime -= serverTime.time_of_day();
	serverTime += timeSinceMidnight;
	serverTime -= Util::GetEdtDiff();
	Assert(m_serverTime == pt::not_a_date_time || m_serverTime <= serverTime);
	static const auto maxDiffForLog = pt::minutes(1);
	const bool isFirstSet
		= m_serverTime == pt::not_a_date_time
			|| serverTime < m_serverTime
			|| serverTime - m_serverTime >= maxDiffForLog;
	if (isFirstSet) {
		Log::Debug(
			TRADER_ENYX_LOG_PREFFIX "server time: %1% -> %2% (%3% = %4%).",
			m_serverTime.time_of_day(),
			serverTime.time_of_day(),
			secondsSinceMidnight,
			timeSinceMidnight);
	}
	m_serverTime = serverTime;
}

pt::ptime FeedHandler::GetMessageTime(const NXFeedMessage &time) const {
	if (m_serverTime.is_not_a_date_time()) {
		throw ServerTimeNotSetError();
	}
	pt::ptime result = m_serverTime;
	result += pt::microseconds(const_cast<NXFeedMessage &>(time).getMarketDataTimestamp() / 1000);
	return result;
}

FeedHandler::Order & FeedHandler::FindOrder(EnyxOrderId id) {
	return FindOrderPos(id)->second;
}

FeedHandler::Orders::iterator FeedHandler::FindOrderPos(EnyxOrderId id) {
	const auto result = m_orders.find(id);
	if (result == m_orders.end()) {
		throw OrderNotFoundError();
	}
	return result;
}

template<typename Message>
void FeedHandler::LogAndHandleMessage(const Message &message) {
	if (m_rawLog) {
		m_rawLog->Append(GetMessageTime(message), message);
	}
	HandleMessage(message);
}

void FeedHandler::onOrderMessage(NXFeedOrder *message) {
	try {
		switch(message->getSubType()) {
			case  NXFEED_SUBTYPE_ORDER_ADD:
				LogAndHandleMessage(
					*reinterpret_cast<nasdaqustvitch41::NXFeedOrderAdd *>(
						message));
				break;
			case  NXFEED_SUBTYPE_ORDER_EXEC:
				LogAndHandleMessage(
					*reinterpret_cast<NXFeedOrderExecute *>(message));
				break;
			case  NXFEED_SUBTYPE_ORDER_EXEC_PRICE:
				LogAndHandleMessage(
					*reinterpret_cast<nasdaqustvitch41::NXFeedOrderExeWithPrice *>(
						message));
				break;
			case  NXFEED_SUBTYPE_ORDER_REDUCE:
				LogAndHandleMessage(
					*reinterpret_cast<NXFeedOrderReduce *>(message));
				return;
			case  NXFEED_SUBTYPE_ORDER_DEL:
				LogAndHandleMessage(
					*reinterpret_cast<NXFeedOrderDelete *>(message));
				break;
			case  NXFEED_SUBTYPE_ORDER_REPLACE:
				LogAndHandleMessage(
					*reinterpret_cast<NXFeedOrderReplace *>(message));
				break;
			default:
				Log::Error(
					TRADER_ENYX_LOG_PREFFIX "unknown message %1%.",
					message->getSubType());
				AssertFail("Unknown message.");
				break;
		}
	} catch (const OrderNotFoundError &) {
		//...//
	} catch (const ServerTimeNotSetError &) {
		//...//
	} catch (...) {
		AssertFailNoException();
	}
}


void FeedHandler::onMiscMessage(NXFeedMisc *message) {
	try {
		switch(message->getSubType()) {
			case NXFEED_SUBTYPE_MISC_TIME:
				HandleMessage(*reinterpret_cast<NXFeedMiscTime *>(message));
				break;
		}
	} catch (...) {
		AssertFailNoException();
	}
}

void FeedHandler::Subscribe(
			const boost::shared_ptr<Security> &security)
		const
		throw() {
	try {
		auto &index = m_marketDataSnapshots.get<BySecurtiy>();
		const auto &pos = index.find(security->GetSymbol());
		if (pos == index.end()) {
			boost::shared_ptr<MarketDataSnapshot> newSnapshot(
				new MarketDataSnapshot(security->GetSymbol(), m_handlFirstLimitUpdate));
			newSnapshot->Subscribe(security);
			m_marketDataSnapshots.insert(MarketDataSnapshotHolder(newSnapshot));
		} else {
			pos->ptr->Subscribe(security);
		}
	} catch (...) {
		AssertFailNoException();
	}
}

void FeedHandler::EnableRawLog() {
	if (m_rawLog) {
		return;
	}
	m_rawLog = new RawLog;
}
