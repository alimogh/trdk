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
#include "Security.hpp"
#include "Common/FileSystemChangeNotificator.hpp"

namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;

class Terminal::Implementation : private boost::noncopyable {

private:

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
		explicit OrderCommand(TradeSystem &tradeSystem)
				: Command(tradeSystem),
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
			const Symbol symbol = Symbol::ParseCash(
				Symbol::SECURITY_TYPE_CASH,
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
					const TradeSystem::OrderStatus &status,
					const Qty &filled,
					const Qty &remaining,
					double avgPrice) {
			m_tradeSystem.GetLog().Info(
				"Terminal: Order reply received:"
					" order ID = %1%, status = %6% (%2%), filled qty = %3%,"
					" remaining qty = %4%, avgPrice = %5%.",
				orderId,
				status,
				filled,
				remaining,
				avgPrice,
				TradeSystem::GetStringStatus(status));
		}
		Currency GetCurrency() const {
			return m_currency != numberOfCurrencies
				?	m_currency
				:	m_security->GetSymbol().GetCashCurrency();
		}
	protected:
		OrderTime m_orderTime;
		Security *m_security;
		Qty m_qty;
		double m_price;
	private:
		Lib::Currency m_currency;
	};

	class SellCommand : public OrderCommand {
	public:
		explicit SellCommand(TradeSystem &tradeSystem)
				: OrderCommand(tradeSystem) {
			//...//
		}
		virtual ~SellCommand() {
			//...//
		}
		virtual void Execute() {
			AssertEq(std::string(), Validate());
			const OrderParams orderParams;
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
							_5));
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
							_5));
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
							_5));
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
							_5));
				}
			}
		}
	};

	class BuyCommand : public OrderCommand {
	public:
		explicit BuyCommand(TradeSystem &tradeSystem)
				: OrderCommand(tradeSystem) {
			//...//
		}
		virtual ~BuyCommand() {
			//...//
		}
		virtual void Execute() {
			AssertEq(std::string(), Validate());
			const OrderParams orderParams;
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
							_5));
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
							_5));					
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
							_5));
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
							_5));
				}
			}
		}
	};

	class CancelCommand
		: public Command,
		public boost::enable_shared_from_this<CancelCommand> {
	public:
		explicit CancelCommand(TradeSystem &tradeSystem)
				: Command(tradeSystem),
				m_isOrderIdSet(false) {
			//...//
		}
		virtual ~CancelCommand() {
			//...//
		}
		virtual void ParseField(const std::string &field) {
			if (m_isOrderIdSet) {
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
				m_orderId = boost::lexical_cast<OrderId>(field.substr(pos + 1));
			} catch (const boost::bad_lexical_cast &) {
				throw Exception("Failed to parse order ID");
			}
			m_isOrderIdSet = true;
		}
		virtual std::string Validate() const {
			if (!m_isOrderIdSet) {
				return "No order ID set. Ex.: \"order=123\".";
			}
			return "";
		}
		virtual void Execute() {
			AssertEq(std::string(), Validate());
			m_tradeSystem.CancelOrder(m_orderId);
		}
	private:
		bool m_isOrderIdSet;
		OrderId m_orderId;
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
					command.reset(new BuyCommand(m_tradeSystem));
				} else if (boost::iequals(field, "sell")) {
					command.reset(new SellCommand(m_tradeSystem));
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
