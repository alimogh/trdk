/**************************************************************************
 *   Created: 2016/09/06 02:54:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "History.hpp"
#include "Security.hpp"
#include "MarketDataSource.hpp"
#include "Common/HttpStreamClient.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::DdfPlus;

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

////////////////////////////////////////////////////////////////////////////////

namespace {

	std::string FormatRequestTime(const pt::ptime &source) {
		std::ostringstream result;
		result
			<< source.date().year()
			<< std::setw(2) << std::setfill('0') << int(source.date().month())
			<< std::setw(2) << std::setfill('0') << source.date().day()
			<< std::setw(2) << std::setfill('0')
				<< source.time_of_day().hours()
			<< std::setw(2) << std::setfill('0')
				<< source.time_of_day().minutes()
			<< std::setw(2) << std::setfill('0')
				<< source.time_of_day().seconds();
		return result.str();
	}

}

////////////////////////////////////////////////////////////////////////////////

class History::Client : public Lib::HttpStreamClient {

public:

	typedef Lib::HttpStreamClient Base;

public:
		
	explicit Client(History &service, RequestState &requestState)
		: Base(
			service,
			service.GetSettings().host,
			service.GetSettings().port)
		, m_requestState(requestState) {
		//...//
	}
			
	virtual ~Client() {
		//...//
	}

public:

	//! Starts new or continues request.
	void StartRequest(DdfPlus::Security &security) {

		Assert(!security.GetRequestedDataStartTime().is_not_a_date_time());

		if (
				security.GetSymbol().GetSecurityType()
					!= SECURITY_TYPE_FUTURES) {
			throw Exception("Symbol type is not supported");
		}

		const auto &startTime
			= !security.GetLastMarketDataTime().is_not_a_date_time()
				?	security.GetLastMarketDataTime() + pt::seconds(1)
				:	security.GetRequestedDataStartTime();

		RequestNext(security, startTime + GetCstTimeZoneDiffLocal());

	}

	bool ContinueRequest() {
		
		if (!m_requestState.security) {
			return false;
		}

		auto &security = *m_requestState.security;
		const auto &startTime
			= !security.GetLastMarketDataTime().is_not_a_date_time()
				?	security.GetLastMarketDataTime() + pt::seconds(1)
				:	m_requestState.time + GetCstTimeZoneDiffLocal();

		RequestNext(security, startTime);

		return true;

	}

protected:

	virtual History & GetService() {
		return *boost::polymorphic_downcast<History *>(&Base::GetService());
	}
	virtual const History & GetService() const {
		return *boost::polymorphic_downcast<const History *>(
			&Base::GetService());
	}

	virtual TimeMeasurement::Milestones StartMessageMeasurement() const {
		return GetContext().StartStrategyTimeMeasurement();
	}

	virtual pt::ptime GetCurrentTime() const {
		return GetContext().GetCurrentTime();
	}
			
	virtual void LogDebug(const std::string &message) const {
		GetLog().Debug(message.c_str());
	}

	virtual void LogInfo(const std::string &message) const {
		GetLog().Info(message.c_str());
	}

	virtual void LogWarn(const std::string &message) const {
		GetLog().Warn(message.c_str());
	}

	virtual void LogError(const std::string &message) const {
		GetLog().Error(message.c_str());
	}

	virtual void HandleNewSubMessages(
			const pt::ptime &,
			const Buffer::const_iterator &begin,
			const Buffer::const_iterator &end,
			const TimeMeasurement::Milestones &timeMeasurement) {
			
		Assert(m_requestState.security);
		Assert(begin != end);

		Buffer::const_iterator it = begin;
		if (!m_uncompletedMessagesBuffer.empty()) {
			it = std::find(begin, end, '\n');
			if (it != end) {
				++it;
			}
			const auto messageSize = std::distance(begin, it);
			const auto newBufferSize
				= m_uncompletedMessagesBuffer.size() + messageSize;
			if (newBufferSize > 100) {
				throw ProtocolError(
					"Tick message has wrong format",
					&*std::prev(end),
					'\n');
			}
			m_uncompletedMessagesBuffer.reserve(newBufferSize);
			std::copy(
				begin,
				it,
				std::back_inserter(m_uncompletedMessagesBuffer));
			if (it == end) {
				return;
			}
			const auto &messageEnd = HandleMessage(
				m_uncompletedMessagesBuffer.cbegin(),
				m_uncompletedMessagesBuffer.cend(),
				timeMeasurement);
			if (messageEnd == m_uncompletedMessagesBuffer.cend()) {
				GetLog().Warn(
					"%1%Failed to parse buffered message.",
					GetService().GetLogTag());
				return;
			}
			AssertEq('\n', *messageEnd);
			m_uncompletedMessagesBuffer.clear();
		}

		for ( ; ; ) {
				
			for ( ; it != end && (*it == '\n' || *it == '\r'); ++it);
			if (it == end) {
				break;
			}
				
			const auto &nextIt = HandleMessage(it, end, timeMeasurement);
			if (nextIt == end) {
				m_uncompletedMessagesBuffer.resize(std::distance(it, end));
				std::copy(it, end, m_uncompletedMessagesBuffer.begin());
				return;
			}
			AssertEq('\n', *nextIt);
				
			it = nextIt;
			
		}

	}

	virtual void OnRequestComplete() {

		Assert(m_requestState.security);
		Assert(!m_requestState.time.is_not_a_date_time());

		const auto &stat = GetReceivedVerbouseStat();
		GetLog().Debug(
			"%1%Completed request for %2% from %3% CST."
				" Total received volume: %4$.02f %5%.",
			GetService().GetLogTag(),
			*m_requestState.security,
			m_requestState.time,
			stat.first,
			stat.second);
		if (!m_uncompletedMessagesBuffer.empty()) {
			throw Exception("Unexpected request response end");
		}

		const auto &lastDataTime
			= m_requestState.security->GetLastMarketDataTime();
		const auto &now = GetCurrentTime();
		if (
				!lastDataTime.is_not_a_date_time()
				&& lastDataTime > m_requestState.time) {
	
			// Have data from last request.

			if (
					(lastDataTime - GetCstTimeZoneDiffLocal()).date()
						>= now.date()) {
				// Last ticks "before now".
				RequestNext(
					*m_requestState.security,
					lastDataTime + pt::seconds(1));
			} else {
				// Next day in history.
				AssertLt(lastDataTime.date(), now.date());
				RequestNext(
					*m_requestState.security,
					pt::ptime((lastDataTime + pt::hours(24)).date()));
			}

		} else if (
				(m_requestState.time - GetCstTimeZoneDiffLocal()).date()
					>= now.date()) {
			// No more time "before now". History request is completed.
			m_requestState.log.close();
			auto &security = *m_requestState.security;
			m_requestState.security = nullptr;
			m_requestState.tradingDay = 0;
			GetService().GetSource().OnHistoryRequestCompleted(security);
		} else {
			// Just weekend or holiday.
			RequestNext(
				*m_requestState.security,
				pt::ptime((m_requestState.time + pt::hours(24)).date()));
		}

	}

private:

	DdfPlus::MarketDataSource::Log & GetLog() const {
		return GetService().GetSource().GetLog();
	}

	const Context & GetContext() const {
		return GetService().GetSource().GetContext();
	}

	//! Sends request to service.
	/** @param[in]	security			Security.
		* @param[in]	startTimeRequestCst	Data start in central time time
		*									zone. Note: All times are in
		*									Eastern Time for equities and
		*									Central Time for everything else.
		*
		*/
	void RequestNext(
			DdfPlus::Security &security,
			const pt::ptime &startTimeRequestCst) {

		boost::format message(
			"/historical/queryticks.ashx"
				"?username=%1%&password=%2%&symbol=%3%&start=%4%");
		message
			% GetService().GetSettings().login
			% GetService().GetSettings().password
			% security.GenerateDdfPlusCode()
			% FormatRequestTime(startTimeRequestCst);

		GetLog().Debug(
			"%1%Sending history sub-request for %2% from %3% CST...",
			GetService().GetLogTag(),
			security,
			startTimeRequestCst);

		Base::Request(
			message.str(),
			[this, &security, &startTimeRequestCst]() {
				
				Assert(
					m_requestState.security
					|| !m_requestState.log.is_open());
				Assert(
					!m_requestState.security
					|| m_requestState.security == &security);
				AssertEq(0, m_uncompletedMessagesBuffer.size());
				
				if (!m_requestState.security) {
					OpenTicksLog(security, startTimeRequestCst);
					m_requestState.security = &security;
				}

				m_requestState.time = startTimeRequestCst;

			});

	}

	Buffer::const_iterator HandleMessage(
			const Buffer::const_iterator &begin,
			const Buffer::const_iterator &end,
			const TimeMeasurement::Milestones &timeMeasurement) {

		Assert(begin != end);
		Assert(m_requestState.security);

		auto field = begin;
		auto it = std::find(field, end, ',');
		if (it == end) {
			return end;
		}
		pt::ptime time;
		{
			const std::string timeStr(field, it);
			try {
				time = pt::time_from_string(timeStr);
			} catch (const std::exception &ex) {
				GetLog().Error(
					"%1%Failed to parse tick time field \"%2%\": \"%3%\".",
					GetService().GetLogTag(),
					timeStr,
					ex.what());
				throw Exception("Protocol error");
			}
		}

		field = std::next(it);
		it = std::find(field, end, ',');
		if (it == end) {
			return end;
		}
		uint16_t tradingDay;
		try {
			tradingDay = boost::lexical_cast<decltype(tradingDay)>(
				boost::make_iterator_range(field, it));
		} catch (const boost::bad_lexical_cast &) {
			throw ProtocolError(
				"Failed to parse tick trading day field",
				&*std::prev(it),
				0);
		}

		field = std::next(it);
		it = std::find(field, end, ',');
		if (it == end) {
			return end;
		}
		const std::string session(field, it);
		if (session.empty()) {
			GetLog().Error(
				"%1%Failed to parse tick session field.",
				GetService().GetLogTag());
			throw Exception("Protocol error");
		}

		field = std::next(it);
		it = std::find(field, end, ',');
		if (it == end) {
			return end;
		}
		double price;
		try {
			price = boost::lexical_cast<decltype(price)>(
				boost::make_iterator_range(field, it));
		} catch (const boost::bad_lexical_cast &) {
			throw ProtocolError(
				"Failed to parse tick price field",
				&*std::prev(it),
				0);
		}

		field = std::next(it);
		it = std::find(field, end, '\n');
		if (it == end) {
			return end;
		}
		Qty qty;
		try {
			qty = boost::lexical_cast<decltype(qty)>(
				boost::make_iterator_range(field, it));
		} catch (const boost::bad_lexical_cast &) {
			throw ProtocolError(
				"Failed to parse tick qty field",
				&*std::prev(it),
				0);
		}

		LogTick(time, tradingDay, session, price, qty);

#		ifdef BOOST_ENABLE_ASSERT_HANDLER
			if (!m_requestState.lastDataTime.is_not_a_date_time()) {
				AssertLe(m_requestState.lastDataTime, time);
			}
			m_requestState.lastDataTime = time;
			AssertLe(m_requestState.time, time + pt::seconds(1));
#		endif

		if (m_requestState.tradingDay != tradingDay) {
			if (m_requestState.tradingDay) {
				m_requestState.security->SwitchTradingSession();
			}
			m_requestState.tradingDay = tradingDay;
		}
	
		m_requestState.security->AddTrade(
			time,
			m_requestState.security->ScalePrice(price),
			qty,
			timeMeasurement,
			true,
			true);

		return it;

	}

	void OpenTicksLog(const Security &security, const pt::ptime &start) {
		
		if (!GetService().GetSettings().isLogEnabled) {
			Assert(!m_requestState.log);
			return;
		}

		Assert(!m_requestState.log.is_open());
		auto path = Defaults::GetTicksLogDir();
		path /= SymbolToFileName(
			(boost::format("ddfplus%1%%2%_%3%_%4%")
				% GetService().GetLogTag()
				% security
				% ConvertToFileName(start)
				% ConvertToFileName(GetContext().GetCurrentTime()))
			.str(),
			"csv");
		
		const bool isNew = !fs::exists(path);
		if (isNew) {
			fs::create_directories(path.branch_path());
		}
		
		m_requestState.log.open(
			path.string(),
			std::ios::out | std::ios::ate | std::ios::app);
		if (!m_requestState.log) {
			GetLog().Error(
				"%1%Failed to open log ticks file %2%",
				GetService().GetLogTag(),
				path);
			throw Exception("Failed to open ticks log file");
		}

		GetLog().Info("%1%Logging ticks %2%.", GetService().GetLogTag(), path);

		m_requestState.log
			<< "Date,Time,Day,Session,Price,Size" << std::endl
			<< std::setfill('0');

	}

	void LogTick(
			const pt::ptime &time,
			uint16_t tradingDay,
			const std::string &session,
			double price,
			const Qty &qty) {
		if (!m_requestState.log.is_open()) {
			return;
		}
		{
			const auto date = time.date();
			m_requestState.log
				<< date.year()
				<< '.' << std::setw(2) << date.month().as_number()
				<< '.' << std::setw(2) << date.day();
		}
		{
			const auto &timeOfDay = time.time_of_day();
			m_requestState.log
				<< ','
				<< std::setw(2) << timeOfDay.hours()
				<< ':' << std::setw(2) << timeOfDay.minutes()
				<< ':' << std::setw(2) << timeOfDay.seconds();
		}
		m_requestState.log
			<< ',' << tradingDay
			<< ',' << session
			<< ',' << price
			<< ',' << qty
			<< std::endl;
	}

private:

	RequestState &m_requestState;

	std::vector<char> m_uncompletedMessagesBuffer;

};

////////////////////////////////////////////////////////////////////////////////

History::History(
		const Settings &settings,
		DdfPlus::MarketDataSource &source)
	: NetworkStreamClientService("History")
	, m_settings(settings)
	, m_source(source) {
	//...//
}

History::~History() {
	try {
		Stop();
	} catch (...) {
		AssertFailNoException();
		terminate();
	}
}

DdfPlus::MarketDataSource & History::GetSource() {
	return m_source;
}
const DdfPlus::MarketDataSource & History::GetSource() const {
	return m_source;
}

const History::Settings & History::GetSettings() const {
	return m_settings;
}

pt::ptime History::GetCurrentTime() const {
	return m_source.GetContext().GetCurrentTime();
}

std::unique_ptr<NetworkStreamClient> History::CreateClient() {
	return std::unique_ptr<NetworkStreamClient>(
		new Client(*this, m_requestState));
}

void History::LogDebug(const char *message) const {
	m_source.GetLog().Debug(message);
}

void History::LogInfo(const std::string &message) const {
	m_source.GetLog().Info(message.c_str());
}

void History::LogError(const std::string &message) const {
	m_source.GetLog().Error(message.c_str());
}

void History::OnConnectionRestored() {
	bool hasRequest = false;
	InvokeClient<Client>(
		[&hasRequest](Client &client) {
			hasRequest = client.ContinueRequest();
		});
	if (!hasRequest) {
		throw Exception("Don't know what to do");
	}
}

void History::LoadHistory(DdfPlus::Security &security) {
	InvokeClient<Client>(
		boost::bind(&Client::StartRequest, _1, boost::ref(security)));
}

////////////////////////////////////////////////////////////////////////////////

