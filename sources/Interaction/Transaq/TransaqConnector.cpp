/**************************************************************************
 *   Created: 2016/10/30 23:29:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TransaqConnector.hpp"
#include "TransaqConnectorContext.hpp"
#include "Core/Context.hpp"
#include "Core/Settings.hpp"
#include "Core/EventsLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Transaq;

namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

namespace {

	template<typename Source>
	pt::ptime ConvertTransacStringToPtime(
			const Source &source,
			const pt::time_duration &timeZoneDiff) {

		boost::iostreams::stream_buffer<boost::iostreams::array_source> buffer(
			&*boost::begin(source), &*boost::end(source));
		std::istream stream(&buffer);

		{
			static const std::locale locale(
				std::locale::classic(),
				new pt::time_input_facet("%d.%m.%Y %H:%M:%S%f"));
			stream.imbue(locale);
		}
		
		pt::ptime result;
		stream >> result;
		return result - timeZoneDiff;
	}

	std::string ConvertToTransaqPriceString(double val) {
		std::ostringstream os;
		os << std::fixed << std::setprecision(7) << val;
		std::string result = os.str();
		const auto dot = result.find('.');
		AssertNe(std::string::npos, dot);
		AssertLe(dot + 2, result.size());
		const auto notZero = result.find_last_not_of('0');
		Assert(notZero != std::string::npos);
		result.resize(notZero == dot ? dot + 2: notZero + 1);
		return result;
	}

}

Connector::Connector(const Context &context, ModuleEventsLog &log)
	: m_context(context)
	, m_log(log)
	, m_numberOfReconnectes(0)
	, m_timeZoneDiff(GetMskTimeZoneDiff(context.GetSettings().GetTimeZone())) {
	//...//
}

Connector::~Connector() {
	//...//
}

Connector::DataHandlers Connector::GetDataHandlers() const {
	
	DataHandlers result;
	
	const auto &insert = [this, &result](
			const char *tag,
			void(Connector::*hander)(
				const ptr::ptree &,
				const Milestones &,
				const Milestones::TimePoint &)) {
		const auto &item = std::make_pair(
			tag,
			boost::bind(hander, const_cast<Connector *>(this), _1, _2, _3));
		Verify(result.emplace(std::move(item)).second);
	};
	insert("error", &Connector::OnErrorMessage);
	insert("markets", &Connector::OnMarketsInfoMessage);
	insert("boards", &Connector::OnBoardsInfoMessage);
	insert("client", &Connector::OnClientsInfoMessage);
	insert("server_status", &Connector::OnServerStatusMessage);

	return result;

}

void Connector::Init() {
	
	Assert(!m_isConnected);
	AssertEq(0, m_dataHandlers.size());
	
	auto dataHandlers = GetDataHandlers();
	m_subscribtion = GetConnectorContext().SubscribeToNewData(
		boost::bind(&Connector::OnNewDataMessage, this, _1, _2, _3));
	dataHandlers.swap(m_dataHandlers);

}

bool Connector::IsConnected() const {
	const Lock lock(m_mutex);
	return m_isConnected && *m_isConnected;
}

void Connector::Connect(const IniSectionRef &conf) {

	Assert(!m_isConnected);
	AssertLt(0, m_dataHandlers.size());
	Assert(!m_connectCommand);

	auto command = boost::make_unique<ptr::ptree>();
	command->put("login", conf.ReadKey("login"));
	command->put("password", conf.ReadKey("password"));
	command->put("host", conf.ReadKey("host"));
	command->put("port", conf.ReadTypedKey<unsigned short>("port"));
	command->put("autopos", "false");
	command->put("micex_registers", "false");
	command->put("milliseconds", "true");
	command->put("utc_time", "false");
	command->put("rqdelay", conf.ReadTypedKey<unsigned short>("rqdelay"));
	command->put(
		"session_timeout",
		conf.ReadTypedKey<size_t>("session_timeout"));
	command->put(
		"request_timeout",
		conf.ReadTypedKey<size_t>("request_timeout"));

	Connect(*command);
	command.swap(m_connectCommand);

}

void Connector::Connect(const ptr::ptree &command) {

	m_log.Info(
		"Connecting to TRANSAQ server at %1%:%2% as %3% with password...",
		command.get<std::string>("host"),
		command.get<std::string>("port"),
		command.get<std::string>("login"));
	m_log.Debug(
		"Connection settings:"
			" rqdelay = %1%; session_timeout = %2%; request_timeout = %3%.",
		command.get<std::string>("rqdelay"),
		command.get<std::string>("session_timeout"),
		command.get<std::string>("request_timeout"));

	{
		
		Lock lock(m_mutex);
		
		Assert(!m_isConnected);

		if (m_isConnected) {
			throw Exception("Already connected");
		}
		m_isConnected = false;

		const auto &result = SendCommand(
			"connect",
			ptr::ptree(command),
			Milestones());
		if (!result.second.empty()) {
			throw ConnectException(result.second.c_str());
		}

		bool isTimeout = !m_connectCondition.timed_wait(
			lock,
			pt::seconds(command.get<long>("request_timeout") * 3));

		if (!*m_isConnected) {
			m_isConnected.reset();
			if (isTimeout) {
				throw ConnectException(
					"Failed to connect to TRANSAQ server: request timeout");
			} else {
				throw ConnectException("Failed to connect to TRANSAQ server");
			}
		}

	}

}

void Connector::Reconnect() {

	Assert(m_connectCommand);
	if (!m_connectCommand) {
		m_log.Error(
			"Failed to reconnect to TRANSAQ server"
				" as was never connected before.");
		GetConnectorContext().StopDueFatalError(
			"Failed to reconnect to TRANSAQ server");
		return;
	}


	try {
		Connect(*m_connectCommand);
	} catch (const ConnectException &ex) {
		m_log.Error(
			"Failed to reconnect to TRANSAQ server: \"%1%\"",
			ex.what());
		GetConnectorContext().StopDueFatalError(
			"Failed to reconnect to TRANSAQ server");
		return;
	}

}

std::pair<boost::property_tree::ptree, std::string> Connector::SendCommand(
		const char *commandId,
		ptr::ptree &&data,
		const Milestones &delayMeasurement) {

	data.put("<xmlattr>.id", commandId);

	ptr::ptree command;
	command.add_child("command", data);

	std::ostringstream os;
    ptr::write_xml(os, command);

	Assert(
		boost::starts_with(
			os.str(),
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"));
	AssertLe(39, os.str().size());
	const auto &resultData = GetConnectorContext().SendCommand(
		os.str().c_str() + 39,
		delayMeasurement);
	if (!resultData) {
		throw Exception("TRANSAQ connector did not return command result");
	}

	std::istringstream stream(&*resultData);
	std::pair<ptr::ptree, std::string> result;

	try {
		ptr::read_xml(stream, result.first);
		if (result.first.get<bool>("result.<xmlattr>.success")) {
			return result;
		}
		result.second = ConvertUtf8ToAscii(
			result.first.get<std::string>("result.message"));
		if (result.second.empty()) {
			m_log.Error(
				"Failed to read TRANSAQ connector answer \"%1%\""
					": there is no error description.",
				&*resultData);
			throw Exception("Failed to read TRANSAQ connector answer");
		}
		return result;
	} catch (const ptr::ptree_error &ex) {
		m_log.Error(
			"Failed to read TRANSAQ connector answer \"%1%\": \"%2%\".",
			&*resultData,
			ex.what());
		throw Exception("Failed to read TRANSAQ connector answer");
	}

}

void Connector::OnNewDataMessage(
		const ptr::ptree &message,
		const Milestones &delayMeasurement,
		const Milestones::TimePoint &timePoint) {
	for (const auto subMessage: message) {
		const auto &handler = m_dataHandlers.find(subMessage.first);
		if (handler == m_dataHandlers.cend()) {
			continue;
		}
		handler->second(subMessage.second, delayMeasurement, timePoint);
	}
}

void Connector::OnErrorMessage(
		const ptr::ptree &message,
		const Milestones &,
		const Milestones::TimePoint &) {
	try {
		m_log.Error(
			"TRANSAQ Connector error: \"%1%\".",
			ConvertUtf8ToAscii(message.get_value<std::string>()));
	} catch (const ptr::ptree_error &ex) {
		m_log.Error("Failed to read error message: \"%1%\".", ex.what());
		throw Exception("Failed to read error message");
	}
}

void Connector::OnMarketsInfoMessage(
		const ptr::ptree &message,
		const Milestones &,
		const Milestones::TimePoint &) {
	std::vector<std::string> list;
	try {
		for (const auto node: message) {
			AssertEq(std::string("market"), node.first);
			const auto &market = node.second;
			boost::format str("%1% (%2%)");
			str
				% ConvertUtf8ToAscii(market.get_value<std::string>())
				% market.get<std::string>("<xmlattr>.id");
			list.emplace_back(str.str());
		}
	} catch (const ptr::ptree_error &ex) {
		m_log.Error("Failed to read market list: \"%1%\".", ex.what());
		throw Exception("Failed to read market list");
	}
	m_log.Info("Available markets: %1%.", boost::join(list, ", "));
}

void Connector::OnBoardsInfoMessage(
		const ptr::ptree &message,
		const Milestones &,
		const Milestones::TimePoint &) {
	std::vector<std::string> list;
	try {
		for (const auto node: message) {
			AssertEq(std::string("board"), node.first);
			const auto &board = node.second;
			boost::format str("%1% (%2%, %3%)");
			str
				% ConvertUtf8ToAscii(board.get<std::string>("name"))
				% board.get<std::string>("<xmlattr>.id");
			switch (board.get<int>("type")) {
				default:
					AssertEq(0, board.get<int>("type"));
					str % "unknown";
					break;
				case 0:
					str % "FORTS";
					break;
				case 1:
					str % "T+";
					break;
				case 2:
					str % "T0";
					break;
			}
			list.emplace_back(str.str());
		}
	} catch (const ptr::ptree_error &ex) {
		m_log.Error("Failed to read board list: \"%1%\".", ex.what());
		throw Exception("Failed to read board list");
	}
	m_log.Info("Available boards: %1%.", boost::join(list, ", "));
}

void Connector::OnClientsInfoMessage(
		const ptr::ptree &message,
		const Milestones &,
		const Milestones::TimePoint &) {
	try {
		std::vector<std::string> attributes;
		{
			const auto &remove
				= message.get_optional<std::string>("<xmlattr>.remove");
			if (remove && *remove != "false") {
				AssertEq(std::string("true"), *remove);
				attributes.emplace_back("REMOVED");
			} else {
				attributes.emplace_back("ADDED");
			}
		}
		if (message.count("type")) {
			attributes.emplace_back(
				"type: " + message.get<std::string>("type"));
		}
		if (message.count("currency")) {
			attributes.emplace_back(
				"currency: " + message.get<std::string>("currency"));
		}
		if (message.count("market")) {
			attributes.emplace_back(
				"market: " + message.get<std::string>("market"));
		}
		if (message.count("union")) {
			attributes.emplace_back(
				"union: " + message.get<std::string>("union"));
		}
		if (message.count("forts_acc")) {
			attributes.emplace_back(
				"FORTS account: " + message.get<std::string>("forts_acc"));
		}
		m_log.Info(
			"Client account \"%1%\": %2%.",
			message.get<std::string>("<xmlattr>.id"),
			boost::join(attributes, "; "));
	} catch (const ptr::ptree_error &ex) {
		m_log.Error("Failed to read client info: \"%1%\".", ex.what());
		throw Exception("Failed to read client info");
	}
}

void Connector::OnServerStatusMessage(
		const ptr::ptree &message,
		const Milestones &,
		const Milestones::TimePoint &) {
	
	try {
	
		const Lock lock(m_mutex);
	
		const auto &connectStatus = message.get<std::string>(
			"<xmlattr>.connected");
		const bool isError
			= connectStatus != "true" && connectStatus != "false";
		Assert(!isError || connectStatus == "error");
		const auto &recoverStatus
			= message.get_optional<std::string>("<xmlattr>.recover");
		const bool isRecovery
			= !isError && recoverStatus && *recoverStatus == "true";
		Assert(isRecovery || !recoverStatus);
		const bool isConnected
			= !isError && !isRecovery && connectStatus == "true";

		std::string idFormat;
		{
			const auto &id = message.get_optional<std::string>("<xmlattr>.id");
			if (id) {
				idFormat += " \"";
				idFormat += *id;
				idFormat += "\"";
			}
		}
	
		if (isConnected) {

			if (m_isConnected) {
				*m_isConnected = true;
			}

			if (m_numberOfReconnectes++ || !m_isConnected) {
				m_log.Warn(
					"Connected to TRANSAQ server%1% with timezone \"%2%\".",
					idFormat,
					message.get<std::string>("<xmlattr>.server_tz"));
			} else {
				m_log.Info(
					"Connected to TRANSAQ server%1% with timezone \"%2%\".",
					idFormat,
					message.get<std::string>("<xmlattr>.server_tz"));
			}

		} else if (isError) {

			Assert(!isRecovery);
			Assert(!recoverStatus);

			m_log.Error(
				"Failed to connect to TRANSAQ server%1%: \"%2%\".",
				idFormat,
				ConvertUtf8ToAscii(message.get_value<std::string>()));
			
			if (m_connectCommand) {
				Reconnect();
			}

		} else if (isRecovery) {

			Assert(!isError);

			m_log.Warn(
				"Disconnected from TRANSAQ server%1%, reconnecting...",
				idFormat);

		} else {

			m_log.Warn("Disconnected from TRANSAQ server%1%.", idFormat);

			Reconnect();

		}

	} catch (const ptr::ptree_error &ex) {
		m_log.Error("Failed to read server status: \"%1%\".", ex.what());
		throw Exception("Failed to read server status");
	}

	m_connectCondition.notify_all();

}

////////////////////////////////////////////////////////////////////////////////

TradingConnector::TradingConnector(
		const Context &context,
		ModuleEventsLog &log,
		const IniSectionRef &conf)
	: Base(context, log) {

	m_defaultBuyOrder.put("client", conf.ReadKey("client"));
	if (conf.IsKeyExist("union")) {
		m_defaultBuyOrder.put("union", conf.ReadKey("union"));
	}
	m_defaultBuyOrder.put("unfilled", "PutInQueue");
	m_defaultSellOrder = m_defaultBuyOrder;
	m_defaultBuyOrder.put("buysell", 'B');
	m_defaultSellOrder .put("buysell", 'S');

	GetLog().Info(
		"Using trading TRANSAQ Connector"
			" with client \"%1%\" and unified account \"%2%\".",
		m_defaultBuyOrder.get<std::string>("client"),
		conf.IsKeyExist("union")
			?	m_defaultBuyOrder.get<std::string>("union")
			:	std::string());

}

TradingConnector::~TradingConnector() {
	//...//
}

TradingConnector::DataHandlers TradingConnector::GetDataHandlers() const {

	DataHandlers result = Base::GetDataHandlers();
	
	const auto &insert = [this, &result](
			const char *tag,
			void(TradingConnector::*hander)(
				const ptr::ptree &,
				const Milestones &,
				const Milestones::TimePoint &)) {
		const auto &item = std::make_pair(
			tag,
			boost::bind(
				hander,
				const_cast<TradingConnector *>(this),
				_1,
				_2,
				_3));
		Verify(result.emplace(std::move(item)).second);
	};
	insert("orders", &TradingConnector::OnOrdersMessage);
	insert("trades", &TradingConnector::OnTradeMessage);

	return result;

}

OrderId TradingConnector::SendSellOrder(
		const std::string &board,
		const std::string &symbol,
		double price,
		const Qty &qty,
		const Milestones &delayMeasurement) {
	return SendOrder(
		m_defaultSellOrder,
		board,
		symbol,
		price,
		qty,
		delayMeasurement);
}

OrderId TradingConnector::SendBuyOrder(
		const std::string &board,
		const std::string &symbol,
		double price,
		const Qty &qty,
		const Milestones &delayMeasurement) {
	return SendOrder(
		m_defaultBuyOrder,
		board,
		symbol,
		price,
		qty,
		delayMeasurement);
}

OrderId TradingConnector::SendOrder(
		const ptr::ptree &templateOrder,
		const std::string &board,
		const std::string &symbol,
		double price,
		const Qty &qty,
		const Milestones &delayMeasurement) {

	ptr::ptree order = templateOrder;
	{
		ptr::ptree security;
		security.put("board", board);
		security.put("seccode", symbol);
		order.add_child("security", security);
	}
	order.put("price", ConvertToTransaqPriceString(price));
	order.put("quantity", int32_t(qty));

	const auto &result = SendCommand(
		"neworder",
		std::move(order),
		delayMeasurement);
	if (!result.second.empty()) {
		throw Exception(result.second.c_str());
	}

	try {
		return result.first.get<OrderId>("result.<xmlattr>.transactionid");
	} catch (const ptr::ptree_error &ex) {
		GetLog().Error("Failed to read transaction reply: \"%1%\".", ex.what());
		throw Exception("Failed to read transaction reply");
	}

}

bool TradingConnector::SendCancelOrder(
		const OrderId &orderId,
		const Milestones &delayMeasurement) {
	
	ptr::ptree message;
	message.put("transactionid", orderId);

	const auto &result = SendCommand(
		"cancelorder",
		std::move(message),
		delayMeasurement);
	if (!result.second.empty()) {
		if (
				result.second.size() == 31
				&& boost::starts_with(result.second, "[151]")) {
			// [151]Субьект операции не найден".
			return false;
		}
		throw Exception(result.second.c_str());
	}

	return true;

}

namespace {

	boost::unordered_map<std::string, OrderStatus> CreateOrderStatusesMap() {
		static_assert(numberOfOrderStatuses == 9, "List changed.");
		boost::unordered_map<std::string, OrderStatus> result;
		Verify(result.emplace("none", numberOfOrderStatuses).second);
		// Активная
		Verify(result.emplace("active", ORDER_STATUS_SUBMITTED).second);
		// Снята трейдером (заявка уже попала на рынок и была отменена)
		Verify(result.emplace("cancelled", ORDER_STATUS_CANCELLED).second);
		// Отклонена Брокером
		Verify(result.emplace("denied", ORDER_STATUS_ERROR).second);
		// Прекращена трейдером (условная заявка, которую сняли до наступления
		// условия)
		Verify(result.emplace("disabled", ORDER_STATUS_CANCELLED).second);
		// Время действия истекло
		Verify(result.emplace("expired", ORDER_STATUS_CANCELLED).second);
		// Не удалось выставить на биржу
		Verify(result.emplace("failed", ORDER_STATUS_ERROR).second);
		// Выставляется на биржу
		Verify(result.emplace("forwarding", ORDER_STATUS_SENT).second);
		// Статус не известен из-за проблем со связью с биржей
		Verify(result.emplace("inactive", ORDER_STATUS_ERROR).second);
		// Исполнена
		Verify(result.emplace("matched", ORDER_STATUS_FILLED).second);
		// Отклонена контрагентом
		Verify(result.emplace("refused", ORDER_STATUS_REJECTED).second);
		// Отклонена биржей
		Verify(result.emplace("rejected", ORDER_STATUS_REJECTED).second);
		// Аннулирована биржей
		Verify(result.emplace("removed", ORDER_STATUS_ERROR).second);
		// Не наступило время активации
		Verify(result.emplace("wait", ORDER_STATUS_SENT).second);
		// Ожидает наступления условия
		Verify(result.emplace("watching", ORDER_STATUS_SENT).second);
		AssertEq(15, result.size());
		return result;
	}

	const OrderStatus & ConvertTransacStringToOrderStatus(
			const std::string &status) {
		static const auto &statuses = CreateOrderStatusesMap();
		const auto &result = statuses.find(status);
		if (result == statuses.cend()) {
			boost::format message("Failed to parse order status \"%1%\"");
			message % status;
			throw TradingConnector::Exception(message.str().c_str());
		}
		return result->second;
	}

}

void TradingConnector::OnOrdersMessage(
		const ptr::ptree &message,
		const Milestones &,
		const Milestones::TimePoint &replyTime) {

	try {

		for (const auto &subMessage: message) {
	
			if (subMessage.first != "order") {
				continue;
			}
			const auto &order = subMessage.second;

			std::string resultMessage;

			const auto &statusText = order.get<std::string>("status");
			auto status = ConvertTransacStringToOrderStatus(statusText);
			if (status == numberOfOrderStatuses) {

				continue;
			
			} else if (status == ORDER_STATUS_CANCELLED) {

				const auto &withdrawTime
					= order.get<std::string>("withdrawtime");
				if (withdrawTime.empty()) {
					continue;
				}
				AssertNe(
					ConvertTransacStringToPtime(
						withdrawTime,
						GetTimeZoneDiff()),
					pt::not_a_date_time);
			
			} else {

				resultMessage = ConvertUtf8ToAscii(
					order.get<std::string>("result"));

				if (status == ORDER_STATUS_ERROR) {
					if (!resultMessage.empty()) {
						resultMessage += " (status: " + statusText + ")";
					} else {
						resultMessage = "status: " + statusText;
					}
				}

			}

			OnOrderUpdate(
					order.get<OrderId>("<xmlattr>.transactionid"),
					order.get<TradingSystemOrderId>("orderno"),
					std::move(status),
					order.get<double>("balance"),
					std::move(resultMessage),
					replyTime);

		}

	} catch (const std::exception &ex) {
		GetLog().Error("Failed to read order messages: \"%1%\".", ex.what());
		throw Exception("Failed to read order messages");
	}

}

void TradingConnector::OnTradeMessage(
		const ptr::ptree &message,
		const Milestones &,
		const Milestones::TimePoint &) {
	try {
		for (const auto &subMessage: message) {
			AssertEq(std::string("trade"), subMessage.first);
			const auto &trade = subMessage.second;
			OnTrade(
				trade.get<std::string>("tradeno"),
				trade.get<TradingSystemOrderId>("orderno"),
				trade.get<double>("price"),
				trade.get<double>("quantity"));
		}
	} catch (const std::exception &ex) {
		GetLog().Error("Failed to read trade messages: \"%1%\".", ex.what());
		throw Exception("Failed to read trade messages");
	}
}

////////////////////////////////////////////////////////////////////////////////

MarketDataSourceConnector::MarketDataSourceConnector(
		const Context &context,
		ModuleEventsLog &log)
	: Base(context, log) {
	GetLog().Debug("Using market data source TRANSAQ Connector.");
}

MarketDataSourceConnector::~MarketDataSourceConnector() {
	//...//
}

MarketDataSourceConnector::DataHandlers
MarketDataSourceConnector::GetDataHandlers()
		const {
	
	DataHandlers result = Base::GetDataHandlers();
	
	const auto &insert = [this, &result](
			const char *tag,
			void(MarketDataSourceConnector::*hander)(
				const ptr::ptree &,
				const Milestones &,
				const Milestones::TimePoint &)) {
		const auto &item = std::make_pair(
			tag,
			boost::bind(
				hander,
				const_cast<MarketDataSourceConnector *>(this),
				_1,
				_2,
				_3));
		Verify(result.emplace(std::move(item)).second);
	};
	insert("alltrades", &MarketDataSourceConnector::OnTicksMessage);
	insert("quotations", &MarketDataSourceConnector::OnQuotationsMessage);

	return result;

}

void MarketDataSourceConnector::SendMarketDataSubscriptionRequest(
		const std::vector<std::pair<std::string, std::string>> &symbols) {

	ptr::ptree alltrades;
	ptr::ptree quotations;
	for (const auto &symbol: symbols) {
		ptr::ptree security;
		security.put("board", symbol.first);
		security.put("seccode", symbol.second);
		alltrades.add_child("security", security);
		quotations.add_child("security", security);
	}

	ptr::ptree command;
	command.add_child("alltrades", alltrades);
	command.add_child("quotations", quotations);

	const auto &result = SendCommand(
		"subscribe",
		std::move(command),
		Milestones());
	if (!result.second.empty()) {
		throw Exception(result.second.c_str());
	}

}

void MarketDataSourceConnector::OnTicksMessage(
		const ptr::ptree &message,
		const Milestones &delayMeasurement,
		const Milestones::TimePoint &) {
	try {
		for (const auto &node: message) {
			AssertEq(std::string("trade"), node.first);
			const auto &tick = node.second;
			OnNewTick(
				ConvertTransacStringToPtime(
					tick.get<std::string>("time"),
					GetTimeZoneDiff()),
				tick.get<std::string>("board"),
				tick.get<std::string>("seccode"),
				tick.get<double>("price"),
				tick.get<double>("quantity"),
				delayMeasurement);
		}
	} catch (const ptr::ptree_error &ex) {
		GetLog().Error("Failed to read tick message: \"%1%\".", ex.what());
		throw Exception("Failed to read tick message");
	}
}

void MarketDataSourceConnector::OnQuotationsMessage(
		const ptr::ptree &message,
		const Milestones &delayMeasurement,
		const Milestones::TimePoint &) {
	try {
		for (const auto &node: message) {
			AssertEq(std::string("quotation"), node.first);
			const auto &update = node.second;
			auto bidPrice = update.get_optional<double>("bid");
			auto bidQty = update.get_optional<double>("biddepth");
			auto askPrice = update.get_optional<double>("offer");
			auto askQty = update.get_optional<double>("offerdepth");
			if (!bidPrice && !bidQty && !askPrice && !askQty) {
				continue;
			}
			OnLevel1Update(
				update.get<std::string>("board"),
				update.get<std::string>("seccode"),
				std::move(bidPrice),
				std::move(bidQty),
				std::move(askPrice),
				std::move(askQty),
				delayMeasurement);
		}
	} catch (const ptr::ptree_error &ex) {
		GetLog().Error(
			"Failed to read Level 1 update message: \"%1%\".",
			ex.what());
		throw Exception("Failed to read Level 1 update message");
	}
}

////////////////////////////////////////////////////////////////////////////////
