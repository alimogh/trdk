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

namespace io = boost::asio;
namespace pt = boost::posix_time;
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

	void Convert(
			const trdk::Security &security,
			const OrderSide &side,
			const Qty &qty,
			const double *price,
			const TimeInForce *timeInForce,
			const Currency &currency,
			const Qty *minQty,
			const std::string *user,
			OrderParameters &result) {
		
		if (price) {
			result.set_type(
				!Lib::IsZero(*price)
					?	OrderParameters::TYPE_LMT
					:	OrderParameters::TYPE_MKT);
		}
		
		result.set_security_id(security.GetInstanceId());

		result.set_side(
			side == ORDER_SIDE_BUY
				?	OrderParameters::SIDE_BUY
				:	OrderParameters::SIDE_SELL);
		
		result.set_qty(qty);
		if (price) {
			result.set_price(*price);
		}
		
		static_assert(numberOfTimeInForces == 5, "List changed.");
		if (timeInForce) {
			switch (*timeInForce) {
				default:
					AssertEq(TIME_IN_FORCE_DAY, *timeInForce);
					break;
				case TIME_IN_FORCE_DAY:
					result.set_time_in_force(OrderParameters::TIME_IN_FORCE_DAY);
					break;
				case TIME_IN_FORCE_GTC:
					result.set_time_in_force(OrderParameters::TIME_IN_FORCE_GTC);
					break;
				case TIME_IN_FORCE_OPG:
					result.set_time_in_force(OrderParameters::TIME_IN_FORCE_OPG);
					break;
				case TIME_IN_FORCE_IOC:
					result.set_time_in_force(OrderParameters::TIME_IN_FORCE_IOC);
					break;
				case TIME_IN_FORCE_FOK:
					result.set_time_in_force(OrderParameters::TIME_IN_FORCE_FOK);
					break;
			}
		}
		
		result.set_currency(ConvertToIso(currency));
		
		if (minQty) {
			result.set_min_qty(*minQty);
		}
		
		if (user) {
			result.set_user(*user);
		}

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
		const boost::uuids::uuid &uuid,
		const pt::ptime *orderTime,
		const std::string *tradeSystemId,
		const Strategy &strategy,
		const uint64_t *operationId,
		const uint64_t *subOperationId,
		const trdk::Security &security,
		const OrderSide &side,
		const Qty &orderQty,
		const double *orderPrice,
		const TimeInForce *timeInForce,
		const Currency &currency,
		const Qty *minQty,
		const std::string *user,
		const TradeSystem::OrderStatus &status,
		const boost::posix_time::ptime *executionTime,
		const std::string *lastTradeId,
		size_t numberOfTrades,
		double avgTradePrice,
		const Qty &executedQty,
		const double *counterAmount,
		const double *bestBidPrice,
		const Qty *bestBidQty,
		const double *bestAskPrice,
		const Qty *bestAskQty) {
	
	ServiceData message;
	message.set_type(ServiceData::TYPE_ORDER);
	Order &order = *message.mutable_order();

	ConvertToUuid(uuid, *order.mutable_id());	
	if (tradeSystemId) {
		order.set_trade_system_order_id(*tradeSystemId);
	}
	ConvertToUuid(strategy.GetId(), *order.mutable_strategy_id());
	if (operationId) {
		order.set_operation_id(*operationId);
	}
	if (subOperationId) {
		order.set_sub_operation_id(*subOperationId);
	}
	Convert(strategy.GetTradingMode(), order);
	Convert(
		security,
		side,
		orderQty,
		orderPrice,
		timeInForce,
		currency,
		minQty,
		user,
		*order.mutable_params());
	static_assert(TradeSystem::numberOfOrderStatuses == 7, "List changes.");
	switch (status) {
		default:
			AssertEq(TradeSystem::ORDER_STATUS_SENT, status);
			break;
		case TradeSystem::ORDER_STATUS_SENT:
			order.set_status(ORDER_STATUS_SENT);
			break;
		case TradeSystem::ORDER_STATUS_REQUESTED_CANCEL:
			order.set_status(ORDER_STATUS_REQUESTED_CANCEL);
			break;
		case TradeSystem::ORDER_STATUS_SUBMITTED:
			order.set_status(ORDER_STATUS_ACTIVE);
			break;
		case TradeSystem::ORDER_STATUS_CANCELLED:
			order.set_status(ORDER_STATUS_CANCELED);
			break;
		case TradeSystem::ORDER_STATUS_FILLED:
			order.set_status(
				executedQty >= orderQty
					?	ORDER_STATUS_FILLED
					:	ORDER_STATUS_FILLED_PARTIALLY);
			break;
		case TradeSystem::ORDER_STATUS_INACTIVE:
		case TradeSystem::ORDER_STATUS_ERROR:
			order.set_status(ORDER_STATUS_ERROR);
			break;
	}
	if (orderTime) {
		order.set_order_time(pt::to_iso_string(*orderTime));
	}
	if (executionTime) {
		order.set_execution_time(pt::to_iso_string(*executionTime));
	}
	if (lastTradeId) {
		order.set_last_trade_id(*lastTradeId);
	}
	order.set_number_of_trades(pf::uint32(numberOfTrades));
	order.set_avg_trade_price(avgTradePrice);
	order.set_executed_qty(executedQty);
	if (counterAmount) {
		order.set_counter_amount(*counterAmount);
	}

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
		const std::string &id,
		const trdk::Strategy &strategy,
		const uint64_t *operationId,
		const uint64_t *subOperationId,
		bool isMaker,
		double tradePrice,
		const trdk::Qty &tradeQty,
		double counterAmount,
		const boost::uuids::uuid &orderUuid,
		const std::string &orderId,
		const trdk::Security &security,
		const OrderSide &side,
		const Qty &orderQty,
		double orderPrice,
		const TimeInForce &timeInForce,
		const Currency &currency,
		const Qty &minQty,
		const std::string &user,
		double bestBidPrice,
		const Qty &bestBidQty,
		double bestAskPrice,
		const Qty &bestAskQty) {

	ServiceData message;
	message.set_type(ServiceData::TYPE_TRADE);
	Trade &trade = *message.mutable_trade();

	trade.set_time(pt::to_iso_string(time));
	trade.set_id(id);
	ConvertToUuid(strategy.GetId(), *trade.mutable_strategy_id());
	Convert(strategy.GetTradingMode(), trade);
	if (operationId) {
		trade.set_operation_id(*operationId);
	}
	if (subOperationId) {
		trade.set_sub_operation_id(*subOperationId);
	}
	trade.set_is_maker(isMaker);
	trade.set_price(tradePrice);
	trade.set_qty(tradeQty);
	trade.set_counter_amount(counterAmount);
	ConvertToUuid(orderUuid, *trade.mutable_order_id());
	trade.set_trade_system_order_id(orderId);
	Convert(
		security,
		side,
		orderQty,
		&orderPrice,
		&timeInForce,
		currency,
		&minQty,
		&user,
		*trade.mutable_order_params());
	Convert(
		bestBidPrice,
		bestBidQty,
		bestAskPrice,
		bestAskQty,
		*trade.mutable_top_of_book());

	Send(message);

}

void DropCopyService::GenerateDebugEvents() {

	{
		ServiceData message;
		message.set_type(ServiceData::TYPE_ORDER);
		Order &order = *message.mutable_order();
		ConvertToUuid(
			boost::uuids::uuid(
				boost::uuids::string_generator()(
					"1C9F1A17-AA80-41E5-972C-CA770BEDA873")),
			*order.mutable_id());
		order.set_trade_system_order_id("XXXZZZYYY");
		ConvertToUuid(
			boost::uuids::uuid(
				boost::uuids::string_generator()(
					"DDA37FBD-1EB2-4552-8A95-19A4AD53D261")),
			*order.mutable_strategy_id());
		Convert(TRADING_MODE_LIVE, order);
		{
			auto &params = *order.mutable_params();
			params.set_type(OrderParameters::TYPE_MKT);
			params.set_security_id(1);
			params.set_side(OrderParameters::SIDE_BUY);
			params.set_qty(100000000);
			params.set_price(987.123);
			params.set_time_in_force(OrderParameters::TIME_IN_FORCE_IOC);
			params.set_currency(ConvertToIso(CURRENCY_EUR));
			params.set_min_qty(10000);
			params.set_user("MRUSER");
		}
		order.set_status(ORDER_STATUS_ACTIVE);
		order.set_order_time(
			pt::to_iso_string(boost::posix_time::microsec_clock::local_time()));
		order.set_execution_time(
			pt::to_iso_string(boost::posix_time::microsec_clock::local_time()));
		order.set_last_trade_id("ASADASD12");
		order.set_number_of_trades(10);
		order.set_avg_trade_price(123.456);
		order.set_executed_qty(9000000);
		order.set_counter_amount(234.567);
		Convert(
			11.22,
			2211,
			33.44,
			4455,
			*order.mutable_top_of_book());
		Send(message);
	}

	{
		ServiceData message;
		message.set_type(ServiceData::TYPE_TRADE);
		Trade &trade = *message.mutable_trade();
		trade.set_time(
			pt::to_iso_string(boost::posix_time::microsec_clock::local_time()));
		trade.set_id("ASDAD123123");
		ConvertToUuid(
			boost::uuids::uuid(
			boost::uuids::string_generator()(
				"DDA37FBD-1EB2-4552-8A95-19A4AD53D261")),
			*trade.mutable_strategy_id());
		Convert(TRADING_MODE_LIVE, trade);
		trade.set_is_maker(false);
		trade.set_price(123.45);
		trade.set_qty(10000000);
		trade.set_counter_amount(23.45);
		ConvertToUuid(
			boost::uuids::uuid(
			boost::uuids::string_generator()(
				"1C9F1A17-AA80-41E5-972C-CA770BEDA873")),
			*trade.mutable_order_id());
		trade.set_trade_system_order_id("SSSDDDFFF");
		{
			auto &params = *trade.mutable_order_params();
			params.set_type(OrderParameters::TYPE_MKT);
			params.set_security_id(1);
			params.set_side(OrderParameters::SIDE_BUY);
			params.set_qty(100000000);
			params.set_price(987.123);
			params.set_time_in_force(OrderParameters::TIME_IN_FORCE_IOC);
			params.set_currency(ConvertToIso(CURRENCY_EUR));
			params.set_user("MRUSER");
		}
		Convert(
			11.22,
			2211,
			33.44,
			4455,
			*trade.mutable_top_of_book());
		Send(message);
	}

}

