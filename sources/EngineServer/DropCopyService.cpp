/**************************************************************************
 *   Created: 2015/07/12 19:37:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "DropCopyService.hpp"
#include "DropCopyClient.hpp"
#include "EngineService/Utils.hpp"
#include "Core/Strategy.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Security.hpp"
#include "Core/Context.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::EngineServer;
using namespace trdk::EngineService;
using namespace trdk::EngineService::DropCopy;
using namespace trdk::EngineService::MarketData;
using namespace trdk::EngineService::Reports;

namespace io = boost::asio;
namespace pt = boost::posix_time;
namespace uu = boost::uuids;
namespace pf = google::protobuf;

//////////////////////////////////////////////////////////////////////////

namespace {

	void Convert(
			const trdk::Security &source,
			MarketData::Security &result) {
		
		result.set_symbol(source.GetSymbol().GetSymbol());

		static_assert(Symbol::numberOfSecurityTypes == 3, "List changed.");
		switch (source.GetSymbol().GetSecurityType()) {
			case Symbol::SECURITY_TYPE_FOR_SPOT:
				result.set_product(MarketData::Security::PRODUCT_SPOT);
				result.set_type(MarketData::Security::TYPE_FOR);
				break;
			default:
				AssertEq(
					Symbol::SECURITY_TYPE_FOR_SPOT,
					source.GetSymbol().GetSecurityType());
				throw Exception("Unknown security type");
		}

		result.set_source(source.GetSource().GetTag());
	
	}

	void Convert(double price, const trdk::Qty &qty, PriceLevel &result) {
		result.set_price(price);
		result.set_qty(qty);
	}

	void Convert(
			double bestBidPrice,
			const trdk::Qty &bestBidQty,
			double bestAskPrice,
			const trdk::Qty &bestAskQty,
			BidAsk &result) {
		Convert(bestBidPrice, bestBidQty, *result.mutable_bid());
		Convert(bestAskPrice, bestAskQty, *result.mutable_ask());
	}
	
}

//////////////////////////////////////////////////////////////////////////

DropCopyService::SendList::SendList(DropCopyService &service)
	: m_service(service)
	, m_flushFlag(false)
	, m_currentQueue(&m_queues.first)
	, m_isStopped(true) {
	//...//
}

DropCopyService::SendList::~SendList() {
	try {
		Stop();
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

size_t DropCopyService::SendList::GetQueueSize() const {
	return m_queues.first.size + m_queues.second.size;
}

void DropCopyService::SendList::Start() {
	const Lock lock(m_mutex);
	Assert(m_isStopped); // Assert could be removed without doubt.
	if (!m_isStopped) {
		return;
	}
	m_thread = boost::thread(
		[this]() {
			m_service.GetLog().Debug("Send-thread started...");
			m_isStopped = false;
			try {
				SendTask();
			} catch (...) {
				AssertFailNoException();
				m_service.GetLog().Error("Fatal error in the send-thread.");
				throw;
			}
			m_service.GetLog().Debug("Send-thread completed.");
		});
}

void DropCopyService::SendList::Stop() {
	{
		const Lock lock(m_mutex);
		if (!m_isStopped) {
			m_isStopped = true;
			m_dataCondition.notify_all();
		}
	}
	m_thread.join();
}

void DropCopyService::SendList::Flush() {
	Lock lock(m_mutex);
	if (m_flushFlag) {
		return;
	}
	m_flushFlag = true;
	m_dataCondition.notify_all();
	while (m_flushFlag && !m_isStopped) {
		if (GetQueueSize() > 0) {
			m_service.GetLog().Info("Flushing %1% messages...", GetQueueSize());
		}
		m_dataCondition.wait(lock);
	}
}

void DropCopyService::SendList::LogQueue() {

	if (GetQueueSize() == 0) {
		AssertEq(0, m_queues.first.data.size());
		AssertEq(0, m_queues.second.data.size());
		m_service.GetLog().Debug("Messages queue is empty.");
		return;
	}

	m_service.GetLog().Warn("Messages queue has %1% messages!", GetQueueSize());
	size_t counter = 0;
	const auto &logRecords = [&](const Queue &queue) {
		foreach (const auto &message, queue.data) {
			m_service.GetLog().Debug(
				"Unsent message #%1%: \"%2%\".",
				++counter,
				CreateMessage(message).DebugString());
		}
	};
	{
		const Lock lock(m_mutex);
		logRecords(
			m_currentQueue == &m_queues.first
				?	m_queues.second
				:	m_queues.first);
		logRecords(*m_currentQueue);
	}

}

void DropCopyService::SendList::Enqueue(OperationStart &&message) {
	Enqueue(MESSAGE_TYPE_OPERATION_START, message);
}

void DropCopyService::SendList::Enqueue(OperationEnd &&message) {
	Enqueue(MESSAGE_TYPE_OPERATION_END, message);
}

void DropCopyService::SendList::Enqueue(Order &&message) {
	Enqueue(MESSAGE_TYPE_ORDER, message);
}

void DropCopyService::SendList::Enqueue(Trade &&message) {
	Enqueue(MESSAGE_TYPE_TRADE, message);
}

void DropCopyService::SendList::Enqueue(
		MessageType &&messageType,
		boost::any &&message) {
	{
		const Lock lock(m_mutex);
		m_currentQueue->data.emplace_back(messageType, message);
		++m_currentQueue->size;
	}
	m_dataCondition.notify_one();
}

ServiceData DropCopyService::SendList::CreateMessage(
		const Queue::Message &message)
		const {
	static_assert(numberOfMessageTypes == 4, "List changed.");
	switch (message.first) {
		case MESSAGE_TYPE_OPERATION_START:
			return CreateMessage(
				boost::any_cast<const OperationStart &>(message.second));
		case MESSAGE_TYPE_OPERATION_END:
			return CreateMessage(
				boost::any_cast<const OperationEnd &>(message.second));
		case MESSAGE_TYPE_ORDER:
			return CreateMessage(
				boost::any_cast<const Order &>(message.second));
		case MESSAGE_TYPE_TRADE:
			return CreateMessage(
				boost::any_cast<const Trade &>(message.second));
		default:
			throw LogicError("Unknown message type for Drop Copy queue");
	}
}

ServiceData DropCopyService::SendList::CreateMessage(
		const OperationStart &message)
		const {

	ServiceData result;
	result.set_type(ServiceData::TYPE_OPERATION_START);
	Reports::OperationStart &operation = *result.mutable_operation_start();

	ConvertToUuid(message.id, *operation.mutable_id());
	operation.set_time(pt::to_iso_string(message.time));
	operation.set_trading_mode(Convert(message.strategy->GetTradingMode()));
	ConvertToUuid(message.strategy->GetId(), *operation.mutable_strategy_id());
	operation.set_updates_number(pf::uint32(message.updatesNumber));

	return result;

}

ServiceData DropCopyService::SendList::CreateMessage(
		const OperationEnd &message)
		const {
	
	ServiceData result;
	result.set_type(ServiceData::TYPE_OPERATION_END);
	Reports::OperationEnd &operation = *result.mutable_operation_end();

	ConvertToUuid(message.id, *operation.mutable_id());
	operation.set_time(pt::to_iso_string(message.time));
	operation.set_pnl(message.pnl);

	foreach (const auto &position, *message.financialResult) {
		Reports::OperationEnd::FinancialResult &positionMessage
			= *operation.add_financial_result();
		positionMessage.set_currency(ConvertToIso(position.first));
		positionMessage.set_value(position.second);
	}
	
	return result;

}

ServiceData DropCopyService::SendList::CreateMessage(
		const Order &message)
		const {

	ServiceData result;
	result.set_type(ServiceData::TYPE_ORDER);
	MarketData::Order &order = *result.mutable_order();

	ConvertToUuid(message.id, *order.mutable_id());	
	if (message.tradeSystemId) {
		order.set_trade_system_order_id(*message.tradeSystemId);
	}

	if (message.orderTime) {
		order.set_order_time(pt::to_iso_string(*message.orderTime));
	}
	if (message.executionTime) {
		order.set_execution_time(pt::to_iso_string(*message.executionTime));
	}

	static_assert(TradeSystem::numberOfOrderStatuses == 8, "List changes.");
	switch (message.status) {
		default:
			AssertEq(TradeSystem::ORDER_STATUS_SENT, message.status);
			break;
		case TradeSystem::ORDER_STATUS_SENT:
			order.set_status(MarketData::Order::STATUS_SENT);
			break;
		case TradeSystem::ORDER_STATUS_REQUESTED_CANCEL:
			order.set_status(MarketData::Order::STATUS_REQUESTED_CANCEL);
			break;
		case TradeSystem::ORDER_STATUS_SUBMITTED:
			order.set_status(MarketData::Order::STATUS_ACTIVE);
			break;
		case TradeSystem::ORDER_STATUS_CANCELLED:
			order.set_status(MarketData::Order::STATUS_CANCELED);
			break;
		case TradeSystem::ORDER_STATUS_FILLED:
			order.set_status(
				message.executedQty >= message.qty
					?	MarketData::Order::STATUS_FILLED
					:	MarketData::Order::STATUS_FILLED_PARTIALLY);
			break;
		case TradeSystem::ORDER_STATUS_REJECTED:
			order.set_status(MarketData::Order::STATUS_REJECTED);
			break;
		case TradeSystem::ORDER_STATUS_INACTIVE:
		case TradeSystem::ORDER_STATUS_ERROR:
			order.set_status(MarketData::Order::STATUS_ERROR);
			break;
	}

	ConvertToUuid(message.operationId, *order.mutable_operation_id());
	if (message.subOperationId) {
		order.set_sub_operation_id(*message.subOperationId);
	}

	order.set_security_id(message.security->GetInstanceId());

	order.set_side(
		message.side == ORDER_SIDE_BUY
			?	MarketData::Order::SIDE_BUY
			:	MarketData::Order::SIDE_SELL);
	
	order.set_qty(message.qty);
	
	if (message.price) {
		order.set_type(
			!IsZero(*message.price)
				?	MarketData::Order::TYPE_LMT
				:	MarketData::Order::TYPE_MKT);
		order.set_price(*message.price);
	}

	static_assert(numberOfTimeInForces == 5, "List changed.");
	if (message.timeInForce) {
		switch (*message.timeInForce) {
			default:
				AssertEq(TIME_IN_FORCE_DAY, *message.timeInForce);
				break;
			case TIME_IN_FORCE_DAY:
				order.set_time_in_force(MarketData::Order::TIME_IN_FORCE_DAY);
				break;
			case TIME_IN_FORCE_GTC:
				order.set_time_in_force(MarketData::Order::TIME_IN_FORCE_GTC);
				break;
			case TIME_IN_FORCE_OPG:
				order.set_time_in_force(MarketData::Order::TIME_IN_FORCE_OPG);
				break;
			case TIME_IN_FORCE_IOC:
				order.set_time_in_force(MarketData::Order::TIME_IN_FORCE_IOC);
				break;
			case TIME_IN_FORCE_FOK:
				order.set_time_in_force(MarketData::Order::TIME_IN_FORCE_FOK);
				break;
		}
	}

	order.set_currency(ConvertToIso(message.currency));
		
	if (message.minQty) {
		order.set_min_qty(*message.minQty);
	}
		
	if (message.user) {
		order.set_user(*message.user);
	}

	order.set_executed_qty(message.executedQty);

	if (
			message.bestBidPrice
			|| message.bestBidQty
			|| message.bestAskPrice
			|| message.bestAskQty) {
		Assert(message.bestBidPrice);
		Assert(message.bestBidQty);
		Assert(message.bestAskPrice);
		Assert(message.bestAskQty);
		Convert(
			message.bestBidPrice ? *message.bestBidPrice : 0,
			message.bestBidQty ? *message.bestBidQty : 0,
			message.bestAskPrice ? *message.bestAskPrice : 0,
			message.bestAskQty ? *message.bestAskQty: 0,
			*order.mutable_top_of_book());
	}

	return result;

}

ServiceData DropCopyService::SendList::CreateMessage(
		const Trade &message)
		const {

	ServiceData result;
	result.set_type(ServiceData::TYPE_TRADE);
	MarketData::Trade &trade = *result.mutable_trade();

	trade.set_time(pt::to_iso_string(message.time));
	trade.set_trade_system_trade_id(message.tradeSystemTradeid);
	ConvertToUuid(message.orderId, *trade.mutable_order_id());
	trade.set_price(message.price);
	trade.set_qty(message.qty);
	Convert(
		message.bestBidPrice,
		message.bestBidQty,
		message.bestAskPrice,
		message.bestAskQty,
		*trade.mutable_top_of_book());

	return result;
}

void DropCopyService::SendList::SendTask() {

	Lock lock(m_mutex);

	while (!m_isStopped) {

		if (m_currentQueue->data.empty()) {
			m_dataCondition.wait(lock);
			AssertEq(m_currentQueue->data.size(), m_currentQueue->size);
		}
		
		if (m_isStopped) {
			break;
		}

		for (
				size_t count = 1
				; !m_currentQueue->data.empty() && !m_isStopped
				; ++count) {

			Queue *const listToRead = m_currentQueue;
			AssertEq(listToRead->data.size(), listToRead->size);
			m_currentQueue = m_currentQueue == &m_queues.first
				?	&m_queues.second
				:	&m_queues.first;
		
			lock.unlock();

			if (!(count % 3)) {
				m_service.GetLog().Warn(
					"Large queue size to send: %1% messages."
						" After %2% iterations queue still not empty.",
					GetQueueSize(),
					count);
			}
	
			Assert(!listToRead->data.empty());
			AssertEq(listToRead->data.size(), listToRead->size);
			foreach (auto &message, listToRead->data) {
				if (!m_service.SendSync(CreateMessage(message))) {
					break;
				}
				--listToRead->size;
				AssertGt(listToRead->data.size(), listToRead->size);
			}
			listToRead->Reset();
		
			lock.lock();

		}

		if (m_flushFlag) {
			m_flushFlag = false;
			m_dataCondition.notify_all();
		}

	}

}

//////////////////////////////////////////////////////////////////////////

DropCopyService::DropCopyService(Context &context, const IniSectionRef &)
	: Base(context, "DropCopy")
	, m_sendList(*this)
	, m_io(new Io) {
	//...//
}

DropCopyService::~DropCopyService() {
	
	std::unique_ptr<Io> io;
	
	try {

		m_sendList.Flush();

		{
			const ClientLock lock(m_clientMutex);
			io.reset(m_io.release());
			m_clientCondition.notify_all();
		}
		io->service.stop();
		m_sendList.Stop();
		io->threads.join_all();

		m_sendList.LogQueue();

	} catch (...) {
		AssertFailNoException();
		throw;
	}
	
	io.reset();

}

void DropCopyService::Start(const IniSectionRef &conf) {
	
	Assert(!m_io->client);
	AssertEq(0, m_io->threads.size());
	if (m_io->threads.size() != 0) {
		throw Exception("Already started");
	}

	{
		ServiceData message;
		message.set_type(ServiceData::TYPE_DICTIONARY);	
		Dictionary &dictionary = *message.mutable_dictionary();
		size_t securitiesNumber = 0;
		GetContext().ForEachMarketDataSource(
			[&](const MarketDataSource &source) -> bool {
				source.ForEachSecurity(
					[&](const Security &security) -> bool {
						Dictionary::SecurityRecord &record
							= *dictionary.add_securities();
						record.set_id(security.GetInstanceId());
						Convert(security, *record.mutable_security());
						++securitiesNumber;
						return true;
					});
				return true;
			});
		m_dictonary = message;
		GetLog().Debug(
			"Dictionary: Cached %1% securities from %2% market data sources.",
			securitiesNumber,
			GetContext().GetMarketDataSourcesCount());
	}

	try {
		StartNewClient(
			conf.ReadKey("host"),
			boost::lexical_cast<std::string>(conf.ReadTypedKey<size_t>("port")));
	} catch (const trdk::Lib::Exception &ex) {
		GetLog().Error(
			"Failed to connect to Drop Copy Storage: \"%1%\".",
			ex.what());
		throw Exception("Failed to connect to Drop Copy Storage");
	}

	while (m_io->threads.size() < 2) {
		m_io->threads.create_thread(
			[&]() {
				GetLog().Debug("Started IO-service thread...");
				try {
					m_io->service.run();
				} catch (const Exception &ex) {
					GetLog().Error("Unexpected error: \"%1%\".", ex);
				} catch (...) {
					AssertFailNoException();
					throw;
				}
				GetLog().Debug("IO-service thread completed.");
			});
	}

	m_sendList.Start();

}

void DropCopyService::StartNewClient(
		const std::string &host,
		const std::string &port) {
	
	boost::shared_ptr<DropCopyClient> newClient
		= DropCopyClient::Create(*this, host, port);
		
	newClient->Send(m_dictonary);
	GetLog().Debug("Dictionary: Sent.");

	const ClientLock lock(m_clientMutex);
	Assert(!m_io->client);
	m_io->client = newClient;
	m_clientCondition.notify_all();

}

void DropCopyService::OnClientClose() {

	boost::shared_ptr<DropCopyClient> client;
	{
		const ClientLock lock(m_clientMutex);
		if (!m_io) {
			return;
		} 
		client = m_io->client;
		m_io->client.reset();
	}
	if (!client) {
		// Fleeing from a recursion after failed reconnection...
		return;
	}

	ReconnectClient(0, client->GetHost(), client->GetPort());

}

void DropCopyService::ReconnectClient(
		size_t attemptIndex,
		const std::string &host,
		const std::string &port) {
	
	try {
	
		StartNewClient(host, port);
	
	} catch (const DropCopyClient::ConnectError &ex) {

		GetLog().Warn(
			"Failed to reconnect: \"%1%\". Trying again, %2% times...",
			ex.what(),
			attemptIndex + 1);
	
		const auto &sleepTime
			= pt::seconds(long(1 * std::min<size_t>(attemptIndex, 30)));
		boost::shared_ptr<boost::asio::deadline_timer> timer(
			new io::deadline_timer(m_io->service, sleepTime));
		timer->async_wait(
			[this, timer, host, port, attemptIndex] (
					const boost::system::error_code &error) {
				if (error) {
					GetLog().Debug(
						"Reconnect canceled: \"%1%\".",
						SysError(error.value()));
					return;
				}
				ReconnectClient(attemptIndex + 1, host, port);
			});

	}

}

bool DropCopyService::SendSync(const ServiceData &message) {

	ClientLock lock(m_clientMutex);

	for (bool byTimeOut = false; !m_io || !m_io->client; ) {

		if (!m_io) {
			GetLog().Warn(
				"Failed to send message: service stopped."
					" Current queue: %1% messages.",
				m_sendList.GetQueueSize());
			return false;
		}
		
		if (!byTimeOut) {
			GetLog().Warn(
				"Message sending suspended, waiting for reconnect."
					" Current queue: %1% messages.",
				m_sendList.GetQueueSize());
		}
		
		byTimeOut = !m_clientCondition.timed_wait(lock, pt::seconds(30));
		if (byTimeOut) {
			GetLog().Warn(
				"Message sending still not active, waiting for reconnect."
					" Current queue: %1% messages.",
				m_sendList.GetQueueSize());
		}

	}

	m_io->client->Send(message);

	return true;

}

void DropCopyService::CopyOrder(
		const uu::uuid &id,
		const std::string *tradeSystemId,
		const pt::ptime *orderTime,
		const pt::ptime *executionTime,
		const TradeSystem::OrderStatus &status,
		const uu::uuid &operationId,
		const int64_t *subOperationId,
		const trdk::Security &security,
		const OrderSide &side,
		const Qty &qty,
		const double *price,
		const TimeInForce *timeInForce,
		const Currency &currency,
		const Qty *minQty,
		const std::string *user,
		const Qty &executedQty,
		const double *bestBidPrice,
		const Qty *bestBidQty,
		const double *bestAskPrice,
		const Qty *bestAskQty) {

	Order message = {
		id,
		boost::none,
		boost::none,
		boost::none,
		status,
		operationId,
		boost::none,
		&security,
		side,
		qty,
		boost::none,
		boost::none,
		currency,
		boost::none,
		boost::none,
		executedQty
	};

	if (tradeSystemId) {
		message.tradeSystemId = *tradeSystemId;
	}
	if (orderTime) {
		message.orderTime = *orderTime;
	}
	if (executionTime) {
		message.executionTime = *executionTime;
	}
	if (subOperationId) {
		message.subOperationId = *subOperationId;
	}
	if (price) {
		message.price = *price;
	}
	if (timeInForce) {
		message.timeInForce = *timeInForce;
	}
	if (minQty) {
		message.minQty = *minQty;
	}
	if (user) {
		message.user = *user;
	}
	if (bestBidPrice) {
		message.bestBidPrice = *bestBidPrice;
	}
	if (bestBidQty) {
		message.bestBidQty = *bestBidQty;
	}
	if (bestAskPrice) {
		message.bestAskPrice = *bestAskPrice;
	}
	if (bestAskQty) {
		message.bestAskQty = *bestAskQty;
	}

	m_sendList.Enqueue(std::move(message));

}

void DropCopyService::CopyTrade(
		const pt::ptime &time,
		const std::string &tradeSystemTradeid,
		const uu::uuid &orderId,
		double price,
		const Qty &qty,
		double bestBidPrice,
		const Qty &bestBidQty,
		double bestAskPrice,
		const Qty &bestAskQty) {
	Trade message = {
		time,
		tradeSystemTradeid,
		orderId,
		price,
		qty,
		bestBidPrice,
		bestBidQty,
		bestAskPrice,
		bestAskQty
	};
	m_sendList.Enqueue(std::move(message));
}

void DropCopyService::ReportOperationStart(
		const uu::uuid &id,
		const pt::ptime &time,
		const trdk::Strategy &strategy,
		size_t updatesNumber) {
	OperationStart message = {id, time, &strategy, updatesNumber};
	m_sendList.Enqueue(std::move(message));
}

void DropCopyService::ReportOperationEnd(
		const uu::uuid &id,
		const pt::ptime &time,
		double pnl,
		const boost::shared_ptr<const FinancialResult> &financialResult) {
	OperationEnd message = {
		id,
		time,
		pnl,
		financialResult
	};
	m_sendList.Enqueue(std::move(message));
}

