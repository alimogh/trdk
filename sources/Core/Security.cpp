/**************************************************************************
 *   Created: 2012/07/09 18:42:14
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Security.hpp"
#include "RiskControl.hpp"
#include "DropCopy.hpp"
#include "MarketDataSource.hpp"
#include "Position.hpp"
#include "PriceBook.hpp"
#include "Settings.hpp"
#include "Context.hpp"
#include "TradingLog.hpp"
#include "Common/ExpirationCalendar.hpp"

namespace fs = boost::filesystem;
namespace lt = boost::local_time;
namespace pt = boost::posix_time;
namespace sig = boost::signals2;

using namespace trdk;
using namespace trdk::Lib;

////////////////////////////////////////////////////////////////////////////////

namespace {

	typedef double Level1Value;
	
	typedef std::array<
			boost::atomic<Level1Value>,
			numberOfLevel1TickTypes>
		Level1;

	bool IsSet(const Level1Value &value) {
		return !isnan(value);
	}
	bool IsSet(const boost::atomic<Level1Value> &value) {
		return IsSet(value.load());
	}

	void Unset(boost::atomic<Level1Value> &val) noexcept {
		val = std::numeric_limits<Level1Value>::quiet_NaN();
	}

	//! Returns symbol price precision.
	uint8_t GetPrecision(const Symbol &symbol, const MarketDataSource &source) {
		if (boost::iequals(symbol.GetSymbol(), "EUR/USD")) {
			return 5;
		} else if (boost::iequals(symbol.GetSymbol(), "EUR/JPY")) {
			return 3;
		} else if (boost::iequals(symbol.GetSymbol(), "EUR/CHF")) {
			return 5;
		} else if (boost::iequals(symbol.GetSymbol(), "EUR/AUD")) {
			return 5;
		} else if (boost::iequals(symbol.GetSymbol(), "USD/JPY")) {
			return 3;
		} else if (boost::iequals(symbol.GetSymbol(), "USD/CHF")) {
			return 5;
		} else if (boost::iequals(symbol.GetSymbol(), "AUD/USD")) {
			return 5;
		} else if (boost::iequals(symbol.GetSymbol(), "AUD/JPY")) {
			return 3;
		} else if (boost::iequals(symbol.GetSymbol(), "TEST_SCALE2")) {
			return 2;
		} else {
			const uint8_t result = 2;
			source.GetLog().Warn(
				"Precision for \"%1%\" not set. Using default - %2%.",
				symbol,
				int(result));
			return result;
		}
	}

}

////////////////////////////////////////////////////////////////////////////////

 namespace { namespace MarketDataLog {

	class Record : public AsyncLogRecord {

	public:

		explicit Record(const Log::Time &time, const Log::ThreadId &threadId)
			: AsyncLogRecord(time, threadId) {
			//...//
		}

	public:

		const Record & operator >>(std::ostream &os) const {
			Dump(os, ",");
			return *this;
		}

	};

	std::ostream & operator <<(std::ostream &os, const Record &record) {
		record >> os;
		return os;
	}

	class OutStream : private boost::noncopyable {
	public:
		void Write(const Record &record) {
			m_log.Write(record);
		}
		bool IsEnabled() const {
			return m_log.IsEnabled();
		}
		void EnableStream(std::ostream &os) {
			m_log.EnableStream(os, false);
		}
		Log::Time GetTime() {
			return m_log.GetTime();
		}
		Log::ThreadId GetThreadId() const {
			return 0;
		}
	private:
		Log m_log;
	};

	typedef AsyncLog<Record, OutStream, TRDK_CONCURRENCY_PROFILE> LogBase;

	class Log : private LogBase {

	public:

		typedef LogBase Base;

	public:

		explicit Log(
				const Symbol &symbol,
				const Context &context) {

			if (context.GetSettings().IsMarketDataLogEnabled()) {
				
				auto path = context.GetSettings().GetLogsDir();
				path /= "MarketData";

				if (!context.GetSettings().IsReplayMode()) {
					boost::format fileName("%1%__%2%");
					fileName
						% symbol
						% ConvertToFileName(context.GetStartTime());
					path /= SymbolToFileName(fileName.str(), "csv");
				} else {
					boost::format fileName("%1%__%2%__%3%");
					fileName
						% symbol
						% ConvertToFileName(context.GetCurrentTime())
						% ConvertToFileName(context.GetStartTime());
					path /= SymbolToFileName(fileName.str(), "csv");
				}

				fs::create_directories(path.branch_path());
				m_file.open(
					path.string(),
					std::ios::out | std::ios::ate | std::ios::app);
				if (!m_file.is_open()) {
					context.GetLog().Error(
						"Failed to open market data log file %1%",
						path);
					throw Exception("Failed to open market data log file");
				}

				EnableStream(m_file);

				context.GetLog().Info(
					"Market data log for %1%: %2%.",
					symbol,
					path);

			}

		}

	public:

		using Base::IsEnabled;

		template<typename... Params>
		void WriteLevel1Update(
				const pt::ptime &time,
				const Params &...params) {
			FormatAndWrite(
				[&](Record &record) {
					record % record.GetTime() % time % "L1U";
					InsertFirstLevel1Update(record, params...);
				});
		}

		void WriteTrade(
				const boost::posix_time::ptime &time,
				const ScaledPrice &price,
				const Qty &qty,
				bool useAsLastTrade,
				bool useForTradedVolume) {
			FormatAndWrite(
				[&](Record &record) {
					record
						% record.GetTime()
						% time
						% "T"
						% price
						% qty
						% useAsLastTrade
						% useForTradedVolume;
				});
		}

	private:

		template<typename... OtherParams>
		void InsertFirstLevel1Update(
				Record &record,
				const Level1TickValue &tick,
				const OtherParams &...otherParams) {
			record % ConvertToPch(tick.GetType()) % tick.GetValue();
			InsertFirstLevel1Update(record, otherParams...);
		}
		void InsertFirstLevel1Update(const Record &) {
			//...//
		}

	private:

		std::ofstream m_file;

	};

} }

////////////////////////////////////////////////////////////////////////////////

Security::Request::Request()
	: m_numberOfTicks(0) {
	//...//
}

Security::Request::operator bool() const {
	return m_numberOfTicks || m_time.is_not_a_date_time();
}

void Security::Request::Swap(Request &rhs) throw() {
	std::swap(m_time, rhs.m_time);
	std::swap(m_numberOfTicks, rhs.m_numberOfTicks);
}

bool Security::Request::IsEarlier(const Request &rhs) const {

	if (!m_time.is_not_a_date_time()) {
		if (rhs.m_time.is_not_a_date_time()) {
			return true;
		} else if (m_time < rhs.m_time) {
			return true;
		}
	}

	return m_numberOfTicks > rhs.m_numberOfTicks;

}

void Security::Request::RequestNumberOfTicks(size_t numberOfTicks) {
	if (m_numberOfTicks > numberOfTicks) {
		return;
	}
	m_numberOfTicks = numberOfTicks;
}

void Security::Request::RequestTime(const pt::ptime &time) {
	Assert(!time.is_not_a_date_time());
	if (!m_time.is_not_a_date_time() && m_time < time) {
		return;
	}
	m_time = time;
}

void Security::Request::Merge(const Request &rhs) {
	if (!rhs.GetTime().is_not_a_date_time()) {
		RequestTime(rhs.GetTime());
	}
	RequestNumberOfTicks(rhs.GetNumberOfTicks());
}

size_t Security::Request::GetNumberOfTicks() const {
	return m_numberOfTicks;
}

const pt::ptime & Security::Request::GetTime() const {
	return m_time;
}

////////////////////////////////////////////////////////////////////////////////

Security::Bar::Bar(
		const pt::ptime &time,
		const pt::time_duration &size,
		Type type)
	: time(time)
	, size(size)
	, type(type) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

Security::Exception::Exception(const char *what)
	: Lib::Exception(what) {
	//...//
}

Security::MarketDataValueDoesNotExist::MarketDataValueDoesNotExist(
		const char *what)
	: Exception(what) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

class Security::Implementation : private boost::noncopyable {

public:

	template<typename SlotSignature>
	struct SignalTrait {
		typedef sig::signal<
				SlotSignature,
				sig::optional_last_value<
					typename boost::function_traits<
							SlotSignature>
						::result_type>,
				int,
				std::less<int>,
				boost::function<SlotSignature>,
				typename sig::detail::extended_signature<
						boost::function_traits<SlotSignature>::arity,
						SlotSignature>
					::function_type,
				sig::dummy_mutex>
			Signal;
	};

	Security &m_self;

	static boost::atomic<InstanceId> m_nextInstanceId;
	const InstanceId m_instanceId;

	MarketDataSource &m_source;

	static_assert(numberOfTradingModes == 3, "List changed.");
	boost::array<boost::shared_ptr<RiskControlSymbolContext>, 2>
		m_riskControlContext;

	const uint8_t m_pricePrecision;
	const uintmax_t m_priceScale;

	mutable SignalTrait<Level1UpdateSlotSignature>::Signal m_level1UpdateSignal;
	mutable SignalTrait<Level1TickSlotSignature>::Signal m_level1TickSignal;
	mutable SignalTrait<NewTradeSlotSignature>::Signal m_tradeSignal;
	mutable SignalTrait<BrokerPositionUpdateSlotSignature>::Signal
		m_brokerPositionUpdateSignal;
	mutable SignalTrait<NewBarSlotSignature>::Signal m_barSignal;
	mutable SignalTrait<BookUpdateTickSlotSignature>::Signal
		m_bookUpdateTickSignal;
	mutable SignalTrait<ServiceEventSlotSignature>::Signal m_serviceEventSignal;
	mutable SignalTrait<ContractSwitchingSlotSignature>::Signal
		m_contractSwitchedSignal;

	Level1 m_level1;
	boost::atomic<Qty::ValueType> m_brokerPosition;
	boost::atomic_int64_t m_marketDataTime;
	boost::atomic_size_t m_numberOfMarketDataUpdates;
	mutable boost::atomic_bool m_isLevel1Started;
	const SupportedLevel1Types m_supportedLevel1Types;
	bool m_isOnline;
	bool m_isOpened;

	boost::optional<ContractExpiration> m_expiration;

	Request m_request;

	MarketDataLog::Log m_marketDataLog;

public:

	Implementation(
			Security &self,
			MarketDataSource &source,
			const Symbol &symbol,
			const SupportedLevel1Types &supportedLevel1Types)
		: m_self(self)
		, m_instanceId(m_nextInstanceId++)
		, m_source(source)
		, m_pricePrecision(GetPrecision(symbol, source))
		, m_priceScale(size_t(std::pow(10, m_pricePrecision)))
		, m_brokerPosition(0)
		, m_marketDataTime(0)
		, m_numberOfMarketDataUpdates(0)
		, m_isLevel1Started(false)
		, m_supportedLevel1Types(supportedLevel1Types)
		, m_isOnline(false)
		, m_isOpened(false)
		, m_request({})
		, m_marketDataLog(symbol, source.GetContext()) {
		
		static_assert(numberOfTradingModes == 3, "List changed.");
		for (size_t i = 0; i < m_riskControlContext.size(); ++i) {
			m_riskControlContext[i] = m_source
				.GetContext()
				.GetRiskControl(TradingMode(i + 1))
				.CreateSymbolContext(symbol);
		}
	
		for (auto &item: m_level1) {
			Unset(item);
		}

	}

	void UpdateMarketDataStat(const pt::ptime &time) {
		Assert(!time.is_not_a_date_time());
#		ifdef BOOST_ENABLE_ASSERT_HANDLER
			if (!GetLastMarketDataTime().is_not_a_date_time()) {
				AssertLe(GetLastMarketDataTime(), time);
			}
#		endif
		const auto &marketDataTime = ConvertToMicroseconds(time);
		m_marketDataTime = marketDataTime;
		++m_numberOfMarketDataUpdates;
	}

	bool AddLevel1Tick(
			const pt::ptime &time,
			const Level1TickValue &tick,
			const TimeMeasurement::Milestones &timeMeasurement,
			bool flush,
			bool isPreviouslyChanged) {
		const bool isChanged = SetLevel1(
			time,
			tick,
			timeMeasurement,
			flush,
			isPreviouslyChanged);
		UpdateMarketDataStat(time);
		if (CheckLevel1Start()) {
			m_level1TickSignal(time, tick, flush);
		}
		return isChanged;
	}

	bool SetLevel1(
			const pt::ptime &time,
			const Level1TickValue &tick,
			const TimeMeasurement::Milestones &timeMeasurement,
			bool flush,
			bool isPreviouslyChanged) {
		AssertLe(0, tick.GetValue());
		const bool isChanged = !IsEqual(
			m_level1[tick.GetType()].exchange(tick.GetValue()),
			tick.GetValue());
		FlushLevel1Update(
			time,
			timeMeasurement,
			flush,
			isChanged,
			isPreviouslyChanged);
		return isChanged;
	}

	bool CompareAndSetLevel1(
			const pt::ptime &time,
			const Level1TickValue &tick,
			Level1Value prevValue,
			const TimeMeasurement::Milestones &timeMeasurement,
			bool flush,
			bool isPreviouslyChanged) {
		Assert(!IsEqual(tick.GetValue(), prevValue));
		if (IsEqual(tick.GetValue(), prevValue)) {
			return true;
		}
		auto &storage = m_level1[tick.GetType()];
		if (!storage.compare_exchange_weak(prevValue, tick.GetValue())) {
			return false;
		}
		FlushLevel1Update(
			time,
			timeMeasurement,
			flush,
			true,
			isPreviouslyChanged);
		return true;
	}

	void FlushLevel1Update(
			const pt::ptime &time,
			const TimeMeasurement::Milestones &timeMeasurement,
			bool flush,
			bool isChanged,
			bool isPreviouslyChanged) {

		timeMeasurement.Measure(TimeMeasurement::SM_DISPATCHING_DATA_STORE);
		
		if (!flush) {
			return;
		} else if (!(isChanged || isPreviouslyChanged)) {
			UpdateMarketDataStat(time);
			return;
		}

		UpdateMarketDataStat(time);

		if (CheckLevel1Start()) {
			m_level1UpdateSignal(timeMeasurement);
		}

	}

	bool CheckLevel1Start() const {
		if (m_isLevel1Started) {
			return true;
		}
		for (auto i = 0; i < m_level1.size(); ++i) {
			if (!IsSet(m_level1[i]) && m_supportedLevel1Types[i]) {
				return false;
			}
		}
		m_isLevel1Started = true;
		return true;
	}

	pt::ptime GetLastMarketDataTime() const {
		const auto marketDataTime = m_marketDataTime.load();
		return marketDataTime
			?	ConvertToPTimeFromMicroseconds(m_marketDataTime)
			:	pt::not_a_date_time;
	}

	template<Level1TickType tick>
	double GetLevel1Value(const Level1 &level1) const {
		const Level1Value &value = level1[tick];
		if (!IsSet(value)) {
			Assert(IsSet(value));
			boost::format message(
				"Market data value \"%1%\" does not exists for %2%");
			message % ConvertToPch(tick) % m_self;
			throw MarketDataValueDoesNotExist(message.str().c_str());
		}
		AssertLe(0, value);
		return value;
	}

};

boost::atomic<Security::InstanceId> Security::Implementation::m_nextInstanceId(
	0);

//////////////////////////////////////////////////////////////////////////

Security::Security(
		Context &context,
		const Symbol &symbol,
		MarketDataSource &source,
		const SupportedLevel1Types &supportedLevel1Types)
	: Base(context, symbol)
	, m_pimpl(
		new Implementation(
			*this,
			source,
			symbol, 
			supportedLevel1Types)) {
	//...//
}

Security::~Security() {
	//...//
}

const Security::InstanceId & Security::GetInstanceId() const {
	return m_pimpl->m_instanceId;
}

RiskControlSymbolContext & Security::GetRiskControlContext(
		const TradingMode &mode) {
	AssertLt(0, mode);
	AssertGe(m_pimpl->m_riskControlContext.size(), mode);
	// If context is not set - risk control is disabled and nobody should call
	// this method:
	Assert(m_pimpl->m_riskControlContext[mode - 1]);
	return *m_pimpl->m_riskControlContext[mode - 1];
}

const MarketDataSource & Security::GetSource() const {
	return m_pimpl->m_source;
}

uintmax_t Security::GetPriceScale() const {
	return m_pimpl->m_priceScale;
}

uint8_t Security::GetPricePrecision() const throw() {
	return m_pimpl->m_pricePrecision;
}

ScaledPrice Security::ScalePrice(double price) const {
	return ScaledPrice(Scale(price, GetPriceScale()));
}

double Security::DescalePrice(const ScaledPrice &price) const {
	return Descale(int64_t(price), GetPriceScale());
}
double Security::DescalePrice(double price) const {
	return Descale(price, GetPriceScale());
}

pt::ptime Security::GetLastMarketDataTime() const {
	return m_pimpl->GetLastMarketDataTime();
}

size_t Security::TakeNumberOfMarketDataUpdates() const {
	return m_pimpl->m_numberOfMarketDataUpdates.exchange(0);
}

bool Security::IsActive() const {
	return
		m_pimpl->m_isOnline
		&& IsTradingSessionOpened()
		&& m_pimpl->m_isLevel1Started;
}

bool Security::IsOnline() const {
	return m_pimpl->m_isOnline;
}

void Security::SetOnline(const pt::ptime &time, bool isOnline) {
	
	AssertNe(m_pimpl->m_isOnline, isOnline);
	if (m_pimpl->m_isOnline == isOnline) {
		return;
	}
	
	GetContext().GetLog().Info(
		"%1% now is %2% by the event %3%. Last data time: %4%.",
		*this,
		isOnline ? "online" : "offline",
		time,
		GetLastMarketDataTime());

	{
		const auto lock = GetSource().GetContext().SyncDispatching();
		m_pimpl->m_isOnline = isOnline;
		m_pimpl->m_serviceEventSignal(
			time,
			isOnline ? SERVICE_EVENT_ONLINE : SERVICE_EVENT_OFFLINE);
	}

}

bool Security::IsTradingSessionOpened() const {
	return m_pimpl->m_isOpened;
}

void Security::SetTradingSessionState(const pt::ptime &time, bool isOpened) {
	
	AssertNe(m_pimpl->m_isOpened, isOpened);
	if (m_pimpl->m_isOpened == isOpened) {
		return;
	}
	
	GetContext().GetLog().Info(
		"%1% trading session is %2% by the event at %3%. Last data time: %4%.",
		*this,
		isOpened ? "opened" : "closed",
		time,
		GetLastMarketDataTime());
	
	{
		const auto lock = GetSource().GetContext().SyncDispatching();
		m_pimpl->m_isOpened = !m_pimpl->m_isOpened;
		m_pimpl->m_serviceEventSignal(
			time,
			isOpened
				?	SERVICE_EVENT_TRADING_SESSION_OPENED
				:	SERVICE_EVENT_TRADING_SESSION_CLOSED);
	}

}

void Security::SwitchTradingSession(const pt::ptime &time) {

	GetContext().GetLog().Info(
		"%1% trading session is switched (%2%) by the event at %3%."
			" Last data time: %4%.",
		*this,
		m_pimpl->m_isOpened ? "opened" : "closed",
		time,
		GetLastMarketDataTime());

	{

		const auto lock = GetSource().GetContext().SyncDispatching();

		m_pimpl->m_isOpened = !m_pimpl->m_isOpened;
		try {
			m_pimpl->m_serviceEventSignal(
				time,
				m_pimpl->m_isOpened
					?	SERVICE_EVENT_TRADING_SESSION_CLOSED
					:	SERVICE_EVENT_TRADING_SESSION_OPENED);
		} catch (...) {
			m_pimpl->m_isOpened = !m_pimpl->m_isOpened;
			throw;
		}

		m_pimpl->m_isOpened = !m_pimpl->m_isOpened;
		m_pimpl->m_serviceEventSignal(
			time,
			m_pimpl->m_isOpened
				?	SERVICE_EVENT_TRADING_SESSION_OPENED
				:	SERVICE_EVENT_TRADING_SESSION_CLOSED);

	}

}

void Security::SetRequest(const Request &request) {
	m_pimpl->m_request.Merge(request);
}

const Security::Request & Security::GetRequest() const {
	return m_pimpl->m_request;
}

double Security::GetLastPrice() const {
	return DescalePrice(GetLastPriceScaled());
}

ScaledPrice Security::GetLastPriceScaled() const {
	return ScaledPrice(
		m_pimpl->GetLevel1Value<LEVEL1_TICK_LAST_PRICE>(m_pimpl->m_level1));
}

Qty Security::GetLastQty() const {
	return Qty(
		m_pimpl->GetLevel1Value<LEVEL1_TICK_LAST_QTY>(m_pimpl->m_level1));
}

Qty Security::GetTradedVolume() const {
	return Qty(
		m_pimpl->GetLevel1Value<LEVEL1_TICK_TRADING_VOLUME>(m_pimpl->m_level1));
}

ScaledPrice Security::GetAskPriceScaled() const {
	return ScaledPrice(
		m_pimpl->GetLevel1Value<LEVEL1_TICK_ASK_PRICE>(m_pimpl->m_level1));
}

double Security::GetAskPrice() const {
	return DescalePrice(GetAskPriceScaled());
}

Qty Security::GetAskQty() const {
	return Qty(m_pimpl->GetLevel1Value<LEVEL1_TICK_ASK_QTY>(m_pimpl->m_level1));
}

ScaledPrice Security::GetBidPriceScaled() const {
	return ScaledPrice(
		m_pimpl->GetLevel1Value<LEVEL1_TICK_BID_PRICE>(m_pimpl->m_level1));
}

double Security::GetBidPrice() const {
	return DescalePrice(GetBidPriceScaled());
}

Qty Security::GetBidQty() const {
	return Qty(m_pimpl->GetLevel1Value<LEVEL1_TICK_BID_QTY>(m_pimpl->m_level1));
}

Qty Security::GetBrokerPosition() const {
	return Qty(m_pimpl->m_brokerPosition.load());
}

Security::ContractSwitchingSlotConnection Security::SubscribeToContractSwitching(
		const ContractSwitchingSlot &slot)
		const {
	return m_pimpl->m_contractSwitchedSignal.connect(slot);
}

Security::Level1UpdateSlotConnection Security::SubscribeToLevel1Updates(
		const Level1UpdateSlot &slot)
		const {
	return m_pimpl->m_level1UpdateSignal.connect(slot);
}

Security::Level1UpdateSlotConnection Security::SubscribeToLevel1Ticks(
		const Level1TickSlot &slot)
		const {
	return m_pimpl->m_level1TickSignal.connect(slot);
}

Security::NewTradeSlotConnection Security::SubscribeToTrades(
		const NewTradeSlot &slot)
		const {
	return m_pimpl->m_tradeSignal.connect(slot);
}

Security::NewTradeSlotConnection Security::SubscribeToBrokerPositionUpdates(
		const BrokerPositionUpdateSlot &slot)
		const {
	return m_pimpl->m_brokerPositionUpdateSignal.connect(slot);
}

Security::NewBarSlotConnection Security::SubscribeToBars(
		const NewBarSlot &slot)
		const {
	return m_pimpl->m_barSignal.connect(slot);
}

Security::BookUpdateTickSlotConnection
Security::SubscribeToBookUpdateTicks(const BookUpdateTickSlot &slot) const {
	return m_pimpl->m_bookUpdateTickSignal.connect(slot);
}

Security::ServiceEventSlotConnection
Security::SubscribeToServiceEvents(const ServiceEventSlot &slot) const {
	return m_pimpl->m_serviceEventSignal.connect(slot);
}

bool Security::IsLevel1Required() const {
	return IsLevel1UpdatesRequired() || IsLevel1TicksRequired();
}

bool Security::IsLevel1UpdatesRequired() const {
	return !m_pimpl->m_level1UpdateSignal.empty();
}

bool Security::IsLevel1TicksRequired() const {
	return !m_pimpl->m_level1TickSignal.empty();
}

bool Security::IsTradesRequired() const {
	return !m_pimpl->m_tradeSignal.empty();
}

bool Security::IsBrokerPositionRequired() const {
	return !m_pimpl->m_brokerPositionUpdateSignal.empty();
}

bool Security::IsBarsRequired() const {
	return !m_pimpl->m_barSignal.empty();
}

void Security::SetLevel1(
			const pt::ptime &time,
			const Level1TickValue &tick,
			const TimeMeasurement::Milestones &timeMeasurement) {
	m_pimpl->SetLevel1(time, tick, timeMeasurement, true, false);
	m_pimpl->m_marketDataLog.WriteLevel1Update(time, tick);
}

void Security::SetLevel1(
			const pt::ptime &time,
			const Level1TickValue &tick1,
			const Level1TickValue &tick2,
			const TimeMeasurement::Milestones &timeMeasurement) {
	AssertNe(tick1.GetType(), tick2.GetType());
	m_pimpl->SetLevel1(
		time,
		tick2,
		timeMeasurement,
		true,
		m_pimpl->SetLevel1(time, tick1, timeMeasurement, false, false));
	m_pimpl->m_marketDataLog.WriteLevel1Update(time, tick1, tick2);
}

void Security::SetLevel1(
			const pt::ptime &time,
			const Level1TickValue &tick1,
			const Level1TickValue &tick2,
			const Level1TickValue &tick3,
			const TimeMeasurement::Milestones &timeMeasurement) {
	AssertNe(tick1.GetType(), tick2.GetType());
	AssertNe(tick1.GetType(), tick3.GetType());
	AssertNe(tick2.GetType(), tick3.GetType());
	m_pimpl->SetLevel1(
		time,
		tick3,
		timeMeasurement,
		true,
		m_pimpl->SetLevel1(
			time,
			tick2,
			timeMeasurement,
			false,
			m_pimpl->SetLevel1(time, tick1, timeMeasurement, false, false)));
	m_pimpl->m_marketDataLog.WriteLevel1Update(time, tick1, tick2, tick3);
}

void Security::SetLevel1(
		const pt::ptime &time,
		const Level1TickValue &tick1,
		const Level1TickValue &tick2,
		const Level1TickValue &tick3,
		const Level1TickValue &tick4,
		const TimeMeasurement::Milestones &timeMeasurement) {
	AssertNe(tick1.GetType(), tick2.GetType());
	AssertNe(tick1.GetType(), tick3.GetType());
	AssertNe(tick1.GetType(), tick4.GetType());
	AssertNe(tick2.GetType(), tick4.GetType());
	AssertNe(tick3.GetType(), tick2.GetType());
	AssertNe(tick3.GetType(), tick4.GetType());
	m_pimpl->SetLevel1(
		time,
		tick4,
		timeMeasurement,
		true,
		m_pimpl->SetLevel1(
			time,
			tick3,
			timeMeasurement,
			false,
			m_pimpl->SetLevel1(
				time,
				tick2,
				timeMeasurement,
				false,
				m_pimpl->SetLevel1(
					time,
					tick1,
					timeMeasurement,
					false,
					false))));
	m_pimpl->m_marketDataLog.WriteLevel1Update(
		time,
		tick1,
		tick2,
		tick3,
		tick4);
}

void Security::AddLevel1Tick(
			const pt::ptime &time,
			const Level1TickValue &tick,
			const TimeMeasurement::Milestones &timeMeasurement) {
	m_pimpl->AddLevel1Tick(time, tick, timeMeasurement, true, false);
}

void Security::AddLevel1Tick(
			const pt::ptime &time,
			const Level1TickValue &tick1,
			const Level1TickValue &tick2,
			const TimeMeasurement::Milestones &timeMeasurement) {
	m_pimpl->AddLevel1Tick(
		time,
		tick2,
		timeMeasurement,
		true,
		m_pimpl->AddLevel1Tick(time, tick1, timeMeasurement, false, false));
}

void Security::AddLevel1Tick(
			const pt::ptime &time,
			const Level1TickValue &tick1,
			const Level1TickValue &tick2,
			const Level1TickValue &tick3,
			const TimeMeasurement::Milestones &timeMeasurement) {
	m_pimpl->AddLevel1Tick(
		time,
		tick3,
		timeMeasurement,
		true,
		m_pimpl->AddLevel1Tick(
			time,
			tick2,
			timeMeasurement,
			false,
			m_pimpl->AddLevel1Tick(
				time,
				tick1,
				timeMeasurement,
				false,
				false)));
}

void Security::AddLevel1Tick(
			const pt::ptime &time,
			const Level1TickValue &tick1,
			const Level1TickValue &tick2,
			const Level1TickValue &tick3,
			const Level1TickValue &tick4,
			const TimeMeasurement::Milestones &timeMeasurement) {
	m_pimpl->AddLevel1Tick(
		time,
		tick4,
		timeMeasurement,
		true,
		m_pimpl->AddLevel1Tick(
			time,
			tick3,
			timeMeasurement,
			false,
			m_pimpl->AddLevel1Tick(
				time,
				tick2,
				timeMeasurement,
				false,
				m_pimpl->AddLevel1Tick(
					time,
					tick1,
					timeMeasurement,
					false,
					false))));
}

void Security::AddLevel1Tick(
		const pt::ptime &time,
		const std::vector<trdk::Level1TickValue> &ticks,
		const TimeMeasurement::Milestones &timeMeasurement) {
	size_t counter = 0;
	bool isPreviousChanged = false;
	for (const auto &tick: ticks) {
		isPreviousChanged = m_pimpl->AddLevel1Tick(
			time,
			tick,
			timeMeasurement,
			++counter >= ticks.size(),
			isPreviousChanged);
	}
}

void Security::AddTrade(
			const boost::posix_time::ptime &time,
			const ScaledPrice &price,
			const Qty &qty,
			const TimeMeasurement::Milestones &timeMeasurement,
			bool useAsLastTrade,
			bool useForTradedVolume) {

	bool isLevel1Changed = false;
	if (useAsLastTrade) {
		if (m_pimpl->SetLevel1(
				time,
				Level1TickValue::Create<LEVEL1_TICK_LAST_QTY>(qty),
				timeMeasurement,
				false,
				false)) {
			isLevel1Changed = true;
		}
		if (m_pimpl->SetLevel1(
				time,
				Level1TickValue::Create<LEVEL1_TICK_LAST_PRICE>(price),
				timeMeasurement,
				!useForTradedVolume,
				isLevel1Changed)) {
			isLevel1Changed = true;
		}
	}
	
	AssertLt(0, qty);
	if (useForTradedVolume && qty > 0) {
		for ( ; ; ) {
			const auto &prevVal
				= m_pimpl->m_level1[LEVEL1_TICK_TRADING_VOLUME].load();
			const auto newVal
				= Level1TickValue::Create<LEVEL1_TICK_TRADING_VOLUME>(
					IsSet(prevVal) ? Qty(prevVal + qty) : qty);
			if (
					m_pimpl->CompareAndSetLevel1(
						time,
						newVal,
						prevVal,
						timeMeasurement,
						true,
						isLevel1Changed)) {
				break;
			}
		}
	}

	m_pimpl->UpdateMarketDataStat(time);
	m_pimpl->m_tradeSignal(time, price, qty);

	m_pimpl->m_marketDataLog.WriteTrade(
		time,
		price,
		qty,
		useAsLastTrade,
		useForTradedVolume);

}

void Security::AddBar(const Bar &bar) {
	m_pimpl->UpdateMarketDataStat(bar.time);
	m_pimpl->m_barSignal(bar);
}

void Security::SetBrokerPosition(const Qty &qty, bool isInitial) {
	if (m_pimpl->m_brokerPosition.exchange(qty) == qty) {
		return;
	}
	m_pimpl->m_brokerPositionUpdateSignal(qty, isInitial);
}

void Security::SetBook(
		PriceBook &book,
		const TimeMeasurement::Milestones &timeMeasurement) {

	AssertNe(pt::not_a_date_time, book.GetTime());

#	if defined(BOOST_ENABLE_ASSERT_HANDLER)
	{
		for (size_t i = 0; i < book.GetBid().GetSize(); ++i) {
			Assert(!IsZero(book.GetBid().GetLevel(i).GetPrice()));
			AssertNe(0, book.GetBid().GetLevel(i).GetQty());
			AssertNe(pt::not_a_date_time, book.GetBid().GetLevel(i).GetTime());
			if (i > 0) {
				AssertGt(
					book.GetBid().GetLevel(i - 1).GetPrice(),
					book.GetBid().GetLevel(i).GetPrice());
			}
		}
		for (size_t i = 0; i < book.GetAsk().GetSize(); ++i) {
			Assert(!IsZero(book.GetAsk().GetLevel(i).GetPrice()));
			AssertNe(0, book.GetAsk().GetLevel(i).GetQty());
			AssertNe(pt::not_a_date_time, book.GetAsk().GetLevel(i).GetTime());
			if (i > 0) {
				AssertLt(
					book.GetAsk().GetLevel(i - 1).GetPrice(),
					book.GetAsk().GetLevel(i).GetPrice());
			}
		}
	}
#	endif

	GetContext().InvokeDropCopy(
		[this, &book](DropCopy &dropCopy) {
			dropCopy.CopyBook(*this, book);
		});

	// Adjusting:
	while (
			!book.GetBid().IsEmpty()
			&& !book.GetAsk().IsEmpty()
			&& book.GetBid().GetTop().GetPrice()
				> book.GetAsk().GetTop().GetPrice()) {
		bool isBidOlder
			= book.GetBid().GetTop().GetTime()
				< book.GetAsk().GetTop().GetTime();
		if (isBidOlder) {
			book.GetBid().PopTop();
		} else {
			book.GetAsk().PopTop();
		}
	}

	SetLevel1(
		book.GetTime(),
		Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>(
			book.GetBid().IsEmpty()
				?	0
				:	ScalePrice(book.GetBid().GetTop().GetPrice())),
		Level1TickValue::Create<LEVEL1_TICK_BID_QTY>(
			book.GetBid().IsEmpty()
				?	Qty(0)
				:	book.GetBid().GetTop().GetQty()),
		Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>(
			book.GetAsk().IsEmpty()
				?	0
				:	ScalePrice(book.GetAsk().GetTop().GetPrice())),
		Level1TickValue::Create<LEVEL1_TICK_ASK_QTY>(
			book.GetAsk().IsEmpty()
				?	Qty(0)
				:	book.GetAsk().GetTop().GetQty()),
		timeMeasurement);
	m_pimpl->UpdateMarketDataStat(book.GetTime());
	
	m_pimpl->m_bookUpdateTickSignal(book, timeMeasurement);

}

const ContractExpiration & Security::GetExpiration() const {
	if (!m_pimpl->m_expiration) {
		boost::format error("%1% doesn't have expiration");
		error % *this;
		throw LogicError(error.str().c_str());
	}
	return *m_pimpl->m_expiration;
}

bool Security::HasExpiration() const {
	return m_pimpl->m_expiration ? true : false;
}

bool Security::SetExpiration(
		const pt::ptime &time,
		const ContractExpiration &expiration) {

#	ifdef BOOST_ENABLE_ASSERT_HANDLER
		if (m_pimpl->m_expiration) {
			AssertLt(*m_pimpl->m_expiration, expiration);
		}
#	endif

	const bool isFirstContract = m_pimpl->m_expiration ? false : true;
	
	GetSource().GetLog().Info(
		m_pimpl->m_expiration
			?	"Switching %1% to the next contract %2%%3% (%4%)"
					" by the event at %5%..."
			:	"Starting %1% with the contract %2%%3% (%4%)"
					" by the event at %5%...",
		*this,
		GetSymbol().GetSymbol(),
		expiration.GetContract(true),
		expiration.GetDate(),
		time);
	
	{

		const auto lock = GetSource().GetContext().SyncDispatching();
		
		if (m_pimpl->m_expiration) {
			m_pimpl->m_request = {};
		}

		m_pimpl->m_isLevel1Started = false;
		m_pimpl->m_expiration = expiration;
		for (auto &item: m_pimpl->m_level1) {
			Unset(item);
		}
		m_pimpl->m_marketDataTime = 0;

		Request request;
		m_pimpl->m_contractSwitchedSignal(time, request);

		if (request.IsEarlier(m_pimpl->m_request)) {
			m_pimpl->m_request.Merge(request);
			return true;
		} else {
			return false;
		}

	}

}

////////////////////////////////////////////////////////////////////////////////
