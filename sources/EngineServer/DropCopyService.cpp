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

DropCopyService::DropCopyService(Context &context, const IniSectionRef &)
	: Base(context, "DropCopy")
	, m_io(new Io) {
	//...//
}

DropCopyService::~DropCopyService() {
	std::unique_ptr<Io> io;
	try {
		{
			const ClientsWriteLock lock(m_clientsMutex);
			io.reset(m_io.release());
		}
		io->service.stop();
		io->threads.join_all();
	} catch (...) {
		AssertFailNoException();
		throw;
	}
	io.reset();
}

void DropCopyService::Start(const IniSectionRef &conf) {
	
	AssertEq(0, m_io->clients.size());
	AssertEq(0, m_io->threads.size());
	if (m_io->threads.size() != 0) {
		throw Exception("Already started");
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

}

void DropCopyService::StartNewClient(
		const std::string &host,
		const std::string &port) {
	
	boost::shared_ptr<DropCopyClient> newClient
		= DropCopyClient::Create(*this, host, port);
		
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

		newClient->Send(std::move(message));
		
		GetLog().Debug(
			"Dictionary: Sent %1% securities from %2% market data sources.",
			securitiesNumber,
			GetContext().GetMarketDataSourcesCount());

	}

	const ClientsWriteLock lock(m_clientsMutex);
	m_io->clients.push_back(&*newClient);

}

void DropCopyService::OnClientClose(const DropCopyClient &client) {

	{
		const ClientsWriteLock lock(m_clientsMutex);
		if (!m_io) {
			return;
		}
		auto it = std::find(
			m_io->clients.begin(),
			m_io->clients.end(),
			&client);
		if (it != m_io->clients.end()) {
			m_io->clients.erase(it);
		} else {
			// Connect error, object wasn't registered.
		}
	}

	ReconnectClient(0, client.GetHost(), client.GetPort());

}

void DropCopyService::ReconnectClient(
		size_t attemptIndex,
		const std::string &host,
		const std::string &port) {
	
	try {
	
		StartNewClient(host, port);
	
	} catch (const DropCopyClient::ConnectError &ex) {

		GetLog().Warn(
			"Failed to reconnect: \"%1%\". Trying again...",
			ex.what());
	
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

void DropCopyService::Send(const ServiceData &message) {
	const ClientsReadLock lock(m_clientsMutex);
	foreach (DropCopyClient *client, m_io->clients) {
		client->Send(message);
	}
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
	
	ServiceData message;
	message.set_type(ServiceData::TYPE_ORDER);
	Order &order = *message.mutable_order();

	ConvertToUuid(id, *order.mutable_id());	
	if (tradeSystemId) {
		order.set_trade_system_order_id(*tradeSystemId);
	}

	if (orderTime) {
		order.set_order_time(pt::to_iso_string(*orderTime));
	}
	if (executionTime) {
		order.set_execution_time(pt::to_iso_string(*executionTime));
	}

	static_assert(TradeSystem::numberOfOrderStatuses == 7, "List changes.");
	switch (status) {
		default:
			AssertEq(TradeSystem::ORDER_STATUS_SENT, status);
			break;
		case TradeSystem::ORDER_STATUS_SENT:
			order.set_status(Order::STATUS_SENT);
			break;
		case TradeSystem::ORDER_STATUS_REQUESTED_CANCEL:
			order.set_status(Order::STATUS_REQUESTED_CANCEL);
			break;
		case TradeSystem::ORDER_STATUS_SUBMITTED:
			order.set_status(Order::STATUS_ACTIVE);
			break;
		case TradeSystem::ORDER_STATUS_CANCELLED:
			order.set_status(Order::STATUS_CANCELED);
			break;
		case TradeSystem::ORDER_STATUS_FILLED:
			order.set_status(
				executedQty >= qty
					?	Order::STATUS_FILLED
					:	Order::STATUS_FILLED_PARTIALLY);
			break;
		case TradeSystem::ORDER_STATUS_INACTIVE:
		case TradeSystem::ORDER_STATUS_ERROR:
			order.set_status(Order::STATUS_ERROR);
			break;
	}

	ConvertToUuid(operationId, *order.mutable_operation_id());
	if (subOperationId) {
		order.set_sub_operation_id(*subOperationId);
	}

	order.set_security_id(security.GetInstanceId());

	order.set_side(
		side == ORDER_SIDE_BUY ? Order::SIDE_BUY : Order::SIDE_SELL);
	
	order.set_qty(qty);
	
	if (price) {
		order.set_type(!IsZero(*price) ? Order::TYPE_LMT : Order::TYPE_MKT);
		order.set_price(*price);
	}

	static_assert(numberOfTimeInForces == 5, "List changed.");
	if (timeInForce) {
		switch (*timeInForce) {
			default:
				AssertEq(TIME_IN_FORCE_DAY, *timeInForce);
				break;
			case TIME_IN_FORCE_DAY:
				order.set_time_in_force(Order::TIME_IN_FORCE_DAY);
				break;
			case TIME_IN_FORCE_GTC:
				order.set_time_in_force(Order::TIME_IN_FORCE_GTC);
				break;
			case TIME_IN_FORCE_OPG:
				order.set_time_in_force(Order::TIME_IN_FORCE_OPG);
				break;
			case TIME_IN_FORCE_IOC:
				order.set_time_in_force(Order::TIME_IN_FORCE_IOC);
				break;
			case TIME_IN_FORCE_FOK:
				order.set_time_in_force(Order::TIME_IN_FORCE_FOK);
				break;
		}
	}

	order.set_currency(ConvertToIso(currency));
		
	if (minQty) {
		order.set_min_qty(*minQty);
	}
		
	if (user) {
		order.set_user(*user);
	}

	order.set_executed_qty(executedQty);

	if (
			bestBidPrice
			|| bestBidQty
			|| bestAskPrice
			|| bestAskQty) {
		Assert(bestBidPrice);
		Assert(bestBidQty);
		Assert(bestAskPrice);
		Assert(bestAskQty);
		Convert(
			bestBidPrice ? *bestBidPrice : 0,
			bestBidQty ? *bestBidQty : 0,
			bestAskPrice ? *bestAskPrice : 0,
			bestAskQty ? *bestAskQty: 0,
			*order.mutable_top_of_book());
	}

	Send(message);

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

	ServiceData message;
	message.set_type(ServiceData::TYPE_TRADE);
	Trade &trade = *message.mutable_trade();

	trade.set_time(pt::to_iso_string(time));
	trade.set_trade_system_trade_id(tradeSystemTradeid);
	ConvertToUuid(orderId, *trade.mutable_order_id());
	trade.set_price(price);
	trade.set_qty(qty);
	Convert(
		bestBidPrice,
		bestBidQty,
		bestAskPrice,
		bestAskQty,
		*trade.mutable_top_of_book());

	Send(message);

}

void DropCopyService::ReportOperationStart(
		const uu::uuid &id,
		const pt::ptime &time,
		const trdk::Strategy &strategy,
		size_t updatesNumber) {
	
	ServiceData message;
	message.set_type(ServiceData::TYPE_OPERATION_START);
	OperationStart &operation = *message.mutable_operation_start();

	ConvertToUuid(id, *operation.mutable_id());
	operation.set_time(pt::to_iso_string(time));
	operation.set_trading_mode(Convert(strategy.GetTradingMode()));
	ConvertToUuid(strategy.GetId(), *operation.mutable_strategy_id());
	operation.set_updates_number(pf::uint32(updatesNumber));

	Send(message);

}

void DropCopyService::ReportOperationEnd(
		const uu::uuid &id,
		const pt::ptime &time,
		double pnl) {
	
	ServiceData message;
	message.set_type(ServiceData::TYPE_OPERATION_END);
	OperationEnd &operation = *message.mutable_operation_end();

	ConvertToUuid(id, *operation.mutable_id());
	operation.set_time(pt::to_iso_string(time));
	operation.set_pnl(pnl);

	Send(message);

}
