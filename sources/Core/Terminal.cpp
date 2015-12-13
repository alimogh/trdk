/**************************************************************************
 *   Created: 2014/10/20 01:06:04
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Terminal.hpp"
#include "TradeSystem.hpp"
#include "RiskControl.hpp"
#include "Security.hpp"
#include "Common/FileSystemChangeNotificator.hpp"

namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;

class Terminal::Implementation : private boost::noncopyable {

private:

	class RiskControlScope : public trdk::RiskControlScope {
	public:
		RiskControlScope(const TradingMode &tradingMode)
			: trdk::RiskControlScope(tradingMode),
			m_name("Terminal") {
			//...//
		}
		virtual ~RiskControlScope() {
			//...//
		}
	public:
		virtual const std::string & GetName() const {
			return m_name;
		}
		virtual size_t GetIndex() const {
			return 0;
		}
	public:
		virtual void CheckNewBuyOrder(
				const RiskControlOperationId &,
				Security &,
				const Currency &,
				const Qty &,
				const ScaledPrice &) {
			//...//
		}
		virtual void CheckNewSellOrder(
				const RiskControlOperationId &,
				Security &,
				const Currency &,
				const Qty &,
				const ScaledPrice &) {
			//...//
		}
		virtual void ConfirmBuyOrder(
				const RiskControlOperationId &,
				const OrderStatus &,
				Security &,
				const Currency &,
				const ScaledPrice &/*orderPrice*/,
				const Qty &/*remainingQty*/,
				const TradeSystem::TradeInfo *) {
			//...//
		}
		virtual void ConfirmSellOrder(
				const RiskControlOperationId &,
				const OrderStatus &,
				Security &,
				const Currency &,
				const ScaledPrice &/*orderPrice*/,
				const Qty &/*remainingQty*/,
				const TradeSystem::TradeInfo *) {
			//...//
		}
	public:
		virtual void CheckTotalPnl(double /*pnl*/) const {
			//...//
		}
		virtual void CheckTotalWinRatio(
				size_t /*totalWinRatio*/,
				size_t /*operationsCount*/)
				const {
			//...//
		}
	public:
		virtual void ResetStatistics() {
			AssertFail(
				"Statistics not available for this Risk Control Context implementation");
			throw LogicError(
				"Statistics not available for this Risk Control Context implementation");
		}
		virtual FinancialResult GetStatistics() const {
			AssertFail(
				"Statistics not available for this Risk Control Context implementation");
			throw LogicError(
				"Statistics not available for this Risk Control Context implementation");
		}
		virtual FinancialResult TakeStatistics() {
			AssertFail(
				"Statistics not available for this Risk Control Context implementation");
			throw LogicError(
				"Statistics not available for this Risk Control Context implementation");
		}
	public:
		virtual void OnSettingsUpdate(const IniSectionRef &) {
			//...//
		}
	private:
		const std::string m_name;
	};

	class Command : private boost::noncopyable {
	public:
		class Exception : public Lib::Exception {
		public:
			explicit Exception(const char *what) throw()
					: Lib::Exception(what) {
				//...//
			}
		};
	public:
		explicit Command(TradeSystem &tradeSystem)
				: m_tradeSystem(tradeSystem) {
			//...//
		}
		virtual ~Command() {
			//...//
		}
		virtual void ParseField(const std::string &) = 0;
		virtual std::string Validate() const = 0;
		virtual void Execute() = 0;
	protected:
		TradeSystem &m_tradeSystem;
	};

	class OrderCommand
		: public Command,
		public boost::enable_shared_from_this<OrderCommand> {
	protected:
		enum OrderTime {
			ORDER_TIME_NOT_SET,
			ORDER_TIME_IOC,
			ORDER_TIME_DAY
		};
	public:
		explicit OrderCommand(
				TradeSystem &tradeSystem,
				RiskControlScope &riskControlScope)
			: Command(tradeSystem),
			m_riskControlScope(riskControlScope),
			m_orderTime(ORDER_TIME_NOT_SET),
			m_security(nullptr),
			m_qty(0),
			m_price(-1.0),
			m_currency(numberOfCurrencies) {
			//...//
		}
		virtual ~OrderCommand() {
			//...//
		}
		virtual void ParseField(const std::string &field) {
			if (ParseSymbol(field)) {
				return;
			}
			const auto pos = field.find('=');
			if (pos == std::string::npos) {
				throw Exception("Failed to parse command");
			}
			std::string cmd = field.substr(0, pos);
			boost::trim(cmd);
			std::string val = field.substr(pos + 1);
			boost::trim(val);
			if (boost::iequals(cmd, "time")) {
				if (m_orderTime != ORDER_TIME_NOT_SET) {
					throw Exception("Order time already set");
				}
				if (boost::iequals(val, "IOC")) {
					m_orderTime = ORDER_TIME_IOC;
				} else if (boost::iequals(val, "DAY")) {
					m_orderTime = ORDER_TIME_DAY;
				} else {
					throw Exception("Failed to parse order time");
				}
			} else if (boost::iequals(cmd, "qty")) {
				if (m_qty) {
					throw Exception("Qty already set");
				}
				try {
					m_qty = boost::lexical_cast<Qty>(val);
				} catch (const boost::bad_lexical_cast &) {
					//...//
				}
				if (m_qty <= 0) {
					throw Exception("Failed to parse qty value");
				}
			} else if (boost::iequals(cmd, "price")) {
				if (m_price >= 0) {
					throw Exception("Price already set");
				}
				if (!boost::iequals(val, "MKT")) {
					try {
						m_price = boost::lexical_cast<double>(val);
					} catch (const boost::bad_lexical_cast &) {
						//...//
					}
					if (m_price <= 0) {
						throw Exception("Failed to parse price value");
					}
				} else {
					m_price = 0;
				}
			} else if (boost::iequals(cmd, "curr")) {
				if (m_currency != numberOfCurrencies) {
					throw Exception("Currency already set");
				}
				try {
					m_currency = ConvertCurrencyFromIso(val);
				} catch (const Lib::Exception &ex) {
					throw Exception(ex.what());
				}
			} else if (boost::iequals(cmd, "replace")) {
				if (m_replaceOrderId) {
					throw Exception("Order ID for replace already set");
				}
				try {
					m_replaceOrderId = boost::lexical_cast<OrderId>(val);
				} catch (const boost::bad_lexical_cast &) {
					throw Exception("Failed to parse order ID to replace");
				}
			} else if (boost::iequals(cmd, "min-qty")) {
				if (m_minQty) {
					throw Exception("Min qty already set");
				}
				try {
					m_minQty = boost::lexical_cast<Qty>(val);
				} catch (const boost::bad_lexical_cast &) {
					throw Exception("Failed to parse min qty value");
				}
			} else {
				throw Exception("Unknown command");
			}
		}
		virtual std::string Validate() const {
			if (!m_security) {
				return "No symbol set.";
			} else if (!m_qty) {
				return "No qty set. Ex.: \"qty=10000\".";
			} else if (m_price < 0) {
				return "No price set. Ex.: \"price=1.234\".";
			} else if (m_orderTime == ORDER_TIME_NOT_SET) {
				return "No order time set. Ex.: \"time=ioc\" or \"time=day\".";
			}
			return "";
		}
	protected:
		bool ParseSymbol(const std::string &field) {
			if (m_security) {
				return false;
			}
			const Symbol symbol = Symbol::ParseForeignExchangeContract(
				Symbol::SECURITY_TYPE_FOR_SPOT,
				field,
				"");
			try {
				m_security = &m_tradeSystem.GetContext().GetSecurity(symbol);
				return true;
			} catch (const Context::UnknownSecurity &) {
				throw Exception("Failed to find security for command");
			}
		}
		void OnReply(
					const OrderId &orderId,
					const std::string &tradeSystemOrderId,
					const OrderStatus &status,
					const Qty &remaining,
					const TradeSystem::TradeInfo *trade) {
			m_tradeSystem.GetLog().Info(
				"Terminal: Order reply received:"
					" order ID = %1% / %7%, status = %6% (%2%)"
					", filled qty = %3%, remaining qty = %4%, avgPrice = %5%.",
				orderId,
				status,
				trade ? trade->qty : Qty(0),
				remaining,
				trade ? trade->price : 0,
				TradeSystem::GetStringStatus(status),
				tradeSystemOrderId);
		}
		Currency GetCurrency() const {
			return m_currency != numberOfCurrencies
				?	m_currency
				:	m_security->GetSymbol().GetFotBaseCurrency();
		}
		OrderParams GetOrderParams() const {
			OrderParams result;
			result.isManualOrder = true;
			if (m_replaceOrderId) {
				result.orderIdToReplace = *m_replaceOrderId;
			}
			if (m_minQty) {
				result.minTradeQty = *m_minQty;
			}
			return std::move(result);
		}
	protected:
		RiskControlScope &m_riskControlScope;
		OrderTime m_orderTime;
		Security *m_security;
		Qty m_qty;
		double m_price;
	private:
		Currency m_currency;
		boost::optional<OrderId> m_replaceOrderId;
		boost::optional<Qty> m_minQty;
	};

	class SellCommand : public OrderCommand {
	public:
		explicit SellCommand(
				TradeSystem &tradeSystem,
				RiskControlScope &riskControlScope)
			: OrderCommand(tradeSystem, riskControlScope) {
			//...//
		}
		virtual ~SellCommand() {
			//...//
		}
		virtual void Execute() {
			const auto &timeMeasurement
				= m_tradeSystem.GetContext().StartStrategyTimeMeasurement();
			AssertEq(std::string(), Validate());
			const OrderParams &orderParams = GetOrderParams();
			if (Lib::IsZero(m_price)) {
				if (m_orderTime == OrderCommand::ORDER_TIME_DAY) {
					m_tradeSystem.SellAtMarketPrice(
						*m_security,
						GetCurrency(),
						m_qty,
						orderParams,
						boost::bind(
							&SellCommand::OnReply,
							shared_from_this(),
							_1,
							_2,
							_3,
							_4,
							_5),
						m_riskControlScope,
						timeMeasurement);
				} else if (m_orderTime == OrderCommand::ORDER_TIME_IOC) {
					m_tradeSystem.SellAtMarketPriceImmediatelyOrCancel(
						*m_security,
						GetCurrency(),
						m_qty,
						orderParams,
						boost::bind(
							&SellCommand::OnReply,
							shared_from_this(),
							_1,
							_2,
							_3,
							_4,
							_5),
						m_riskControlScope,
						timeMeasurement);
				}
			} else {
				if (m_orderTime == OrderCommand::ORDER_TIME_DAY) {
					m_tradeSystem.Sell(
						*m_security,
						GetCurrency(),
						m_qty,
						m_security->ScalePrice(m_price),
						orderParams,
						boost::bind(
							&SellCommand::OnReply,
							shared_from_this(),
							_1,
							_2,
							_3,
							_4,
							_5),
						m_riskControlScope,
						timeMeasurement);
				} else if (m_orderTime == OrderCommand::ORDER_TIME_IOC) {
					m_tradeSystem.SellImmediatelyOrCancel(
						*m_security,
						GetCurrency(),
						m_qty,
						m_security->ScalePrice(m_price),
						orderParams,
						boost::bind(
							&SellCommand::OnReply,
							shared_from_this(),
							_1,
							_2,
							_3,
							_4,
							_5),
						m_riskControlScope,
						timeMeasurement);
				}
			}
		}
	};

	class BuyCommand : public OrderCommand {
	public:
		explicit BuyCommand(
				TradeSystem &tradeSystem,
				RiskControlScope &riskControlScope)
			: OrderCommand(tradeSystem, riskControlScope) {
			//...//
		}
		virtual ~BuyCommand() {
			//...//
		}
		virtual void Execute() {
			const auto &timeMeasurement
				= m_tradeSystem.GetContext().StartStrategyTimeMeasurement();
			AssertEq(std::string(), Validate());
			const OrderParams &orderParams = GetOrderParams();
			if (Lib::IsZero(m_price)) {
				if (m_orderTime == OrderCommand::ORDER_TIME_DAY) {
					m_tradeSystem.BuyAtMarketPrice(
						*m_security,
						GetCurrency(),
						m_qty,
						orderParams,
						boost::bind(
							&BuyCommand::OnReply,
							shared_from_this(),
							_1,
							_2,
							_3,
							_4,
							_5),
						m_riskControlScope,
						timeMeasurement);
				} else if (m_orderTime == OrderCommand::ORDER_TIME_IOC) {
					m_tradeSystem.BuyAtMarketPriceImmediatelyOrCancel(
						*m_security,
						GetCurrency(),
						m_qty,
						orderParams,
						boost::bind(
							&BuyCommand::OnReply,
							shared_from_this(),
							_1,
							_2,
							_3,
							_4,
							_5),
						m_riskControlScope,
						timeMeasurement);
				}
			} else {
				if (m_orderTime == OrderCommand::ORDER_TIME_DAY) {
					m_tradeSystem.Buy(
						*m_security,
						GetCurrency(),
						m_qty,
						m_security->ScalePrice(m_price),
						orderParams,
						boost::bind(
							&BuyCommand::OnReply,
							shared_from_this(),
							_1,
							_2,
							_3,
							_4,
							_5),
						m_riskControlScope,
						timeMeasurement);
				} else if (m_orderTime == OrderCommand::ORDER_TIME_IOC) {
					m_tradeSystem.BuyImmediatelyOrCancel(
						*m_security,
						GetCurrency(),
						m_qty,
						m_security->ScalePrice(m_price),
						orderParams,
						boost::bind(
							&BuyCommand::OnReply,
							shared_from_this(),
							_1,
							_2,
							_3,
							_4,
							_5),
						m_riskControlScope,
						timeMeasurement);
				}
			}
		}
	};

	class CancelCommand
		: public Command,
		public boost::enable_shared_from_this<CancelCommand> {
	public:
		explicit CancelCommand(TradeSystem &tradeSystem)
				: Command(tradeSystem) {
			//...//
		}
		virtual ~CancelCommand() {
			//...//
		}
		virtual void ParseField(const std::string &field) {
			if (m_orderId) {
				throw Exception("Unknown command");
			}
			const auto pos = field.find('=');
			if (pos == std::string::npos) {
				throw Exception("Failed to parse command");
			}
			std::string cmd = field.substr(0, pos);
			boost::trim(cmd);
			if (!boost::iequals(cmd, "order")) {
				throw Exception("Unknown command");
			}
			try {
				m_orderId
					= boost::lexical_cast<OrderId>(field.substr(pos + 1));
			} catch (const boost::bad_lexical_cast &) {
				throw Exception("Failed to parse order ID");
			}
		}
		virtual std::string Validate() const {
			if (!m_orderId) {
				return "No order ID set. Ex.: \"order=123\".";
			}
			return "";
		}
		virtual void Execute() {
			AssertEq(std::string(), Validate());
			m_tradeSystem.CancelOrder(*m_orderId);
		}
	private:
		boost::optional<OrderId> m_orderId;
	};

	class TestCommand
		: public Command,
		public boost::enable_shared_from_this<TestCommand> {
	public:
		explicit TestCommand(TradeSystem &tradeSystem)
				: Command(tradeSystem) {
			//...//
		}
		virtual ~TestCommand() {
			//...//
		}
		virtual void ParseField(const std::string &) {
			throw Exception("Unknown command");
		}
		virtual std::string Validate() const {
			return "";
		}
		virtual void Execute() {
			m_tradeSystem.Test();
		}
	};

public:

	explicit Implementation(const fs::path &cmdFile, TradeSystem &tradeSystem)
			: m_cmdFile(cmdFile),
			m_tradeSystem(tradeSystem),
			m_riskControlScope(m_tradeSystem.GetMode()),
			m_notificator(m_cmdFile, [this]() {OnCmdFileChanged();}),
			m_lastSeqnumber(0) {
		m_tradeSystem.GetLog().Info(
			"Terminal: Starting with commands file %1%...",
			m_cmdFile);
		m_notificator.Start();
	}
	
private:

	void OnCmdFileChanged() {

		std::ifstream f(m_cmdFile.string().c_str());
		if (!f) {
			m_tradeSystem.GetLog().Error(
				"Terminal: Failed to open terminal commands file %1%.",
				m_cmdFile);
			return;
		}

		char buffer[255] = {};
		size_t line = 0;
		size_t seqnumber = 0;
		size_t execsCount = 0;
		while (!f.eof()) {
			f.getline(buffer, sizeof(buffer) - 1);
			++line;
			if (!ProcessCmdLine(line, buffer, seqnumber, execsCount)) {
				break;
			}
		}

		AssertGe(line, execsCount);
		if (line >= 0) {
			m_tradeSystem.GetLog().Debug(
				"Terminal: processed file %1%"
					" (lines: %2%, commands executed: %3%). ",
				m_cmdFile,
				line,
				execsCount);
		}

	}

	bool ProcessCmdLine(
				size_t lineNo,
				const char *line,
				size_t &knownSeqnumber,
				size_t &execsCount) {

		Assert(line);

		if (line[0] == ';' || (line[0] == '/' && line[1] == '/')) {
			return true;
		}

		size_t seqnumber = 0;
		boost::shared_ptr<Command> command;
		const char *commandsHelp = "\"BUY\", \"SELL\", \"CANCEL\" or \"TEST\"";
	
		typedef boost::split_iterator<const char *> It;
		const auto &end = It();
		for (
				It it = boost::make_split_iterator(
					line,
					boost::token_finder(
						boost::algorithm::is_space(),
						boost::algorithm::token_compress_on));
				it != end;
				++it) {
		
			auto field = boost::copy_range<std::string>(*it);
			boost::trim(field);
			if (field.empty()) {
				continue;
			}

			if (!seqnumber) {
		
				Assert(!command);

				try {
					seqnumber = boost::lexical_cast<size_t>(field);
				} catch (const boost::bad_lexical_cast &) {
					//...//
				}

				if (seqnumber <= 0) {
					m_tradeSystem.GetLog().Error(
						"Terminal: Failed to parse seqnumber"
							" \"%1%\" at %2%:%3%."
							" Must be a positive value greater than zero.",
						field,
						m_cmdFile,
						lineNo);
					return false;
				} else if (knownSeqnumber + 1 != seqnumber) {
					m_tradeSystem.GetLog().Error(
						"Terminal: Wrong terminal command seqnumber \"%1%\""
							" at %2%:%3%, last seqnumber was %4%.",
						seqnumber,
						m_cmdFile,
						lineNo,
						knownSeqnumber);
					return false;
				}

				if (seqnumber <= m_lastSeqnumber) {
					knownSeqnumber = seqnumber;
					return true;
				}

			} else if (!command) {
		
				if (boost::iequals(field, "buy")) {
					command.reset(
						new BuyCommand(m_tradeSystem, m_riskControlScope));
				} else if (boost::iequals(field, "sell")) {
					command.reset(
						new SellCommand(m_tradeSystem, m_riskControlScope));
				} else if (boost::iequals(field, "cancel")) {
					command.reset(new CancelCommand(m_tradeSystem));
				} else if (boost::iequals(field, "test")) {
					command.reset(new TestCommand(m_tradeSystem));
				} else {
					m_tradeSystem.GetLog().Error(
						"Terminal: Failed to parse command \"%1%\" at %2%:%3%."
							" Must be %4%.",
						field,
						m_cmdFile,
						lineNo,
						commandsHelp);
					return false;
				}
		
			} else {

				try {
					command->ParseField(field);
				} catch (const Command::Exception &ex) {
					m_tradeSystem.GetLog().Error(
						"Terminal: Failed to parse command parameter \"%1%\""
							" at \"%2%\":%3%. %4%.",
						field,
						m_cmdFile,
						lineNo,
						ex.what());
					return false;
				}

			}

		}

		if (seqnumber <= 0) {
			return true;
		}

		if (!command) {
			m_tradeSystem.GetLog().Error(
				"Terminal: No command set at %1%:%2%. Must be %3%.",
				m_cmdFile,
				lineNo,
				commandsHelp);
			return false;
		}

		const std::string &commandErrorText = command->Validate();
		if (!commandErrorText.empty()) {
			m_tradeSystem.GetLog().Error(
				"Terminal: Failed to get command parameter at %1%:%2% - %3%",
				m_cmdFile,
				lineNo,
				commandErrorText);
			return false;		
		}

		++execsCount;
		command->Execute();

		m_lastSeqnumber = seqnumber;
		knownSeqnumber = seqnumber;
		return true;

	}

private:

	const boost::filesystem::path m_cmdFile;
	trdk::TradeSystem &m_tradeSystem;
	RiskControlScope m_riskControlScope;
	trdk::Lib::FileSystemChangeNotificator m_notificator;
	size_t m_lastSeqnumber;

};

Terminal::Terminal(const fs::path &cmdFile, TradeSystem &tradeSystem)
		: m_pimpl(new Implementation(cmdFile, tradeSystem)) {
	//...//
}

Terminal::~Terminal() {
	delete m_pimpl;
}
