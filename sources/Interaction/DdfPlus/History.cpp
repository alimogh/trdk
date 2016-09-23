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
namespace gr = boost::gregorian;
namespace fs = boost::filesystem;

////////////////////////////////////////////////////////////////////////////////

namespace {

	std::string FormatRequestTime(pt::ptime source) {
		std::ostringstream result;
		source += GetCstTimeZoneDiffLocal();
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
		
	explicit Client(
			History &service,
			History::Request &request,
			RequestState &requestState)
		: Base(
			service,
			service.GetSettings().host,
			service.GetSettings().port)
		, m_request(request)
		, m_requestState(requestState) {
		//...//
	}
			
	virtual ~Client() {
		//...//
	}

public:

	void StartRequest(DdfPlus::Security &security) {

		Assert(!m_request.security);
		Assert(m_requestState.ticks.empty());
		Assert(!security.GetRequest().GetTime().is_not_a_date_time());
		Assert(!security.HasExpiration());

		if (
				security.GetSymbol().GetSecurityType()
					!= SECURITY_TYPE_FUTURES) {
			throw Exception("Symbol type is not supported");
		}

		AssertEq(security.GetLastMarketDataTime(), pt::not_a_date_time);
		Assert(security.GetRequest());
		Assert(
			security.GetRequest().GetTime().is_not_a_date_time()
			|| security.GetRequest().GetNumberOfTicks() > 0);

		History::Request request = {};
		request.security = &security;
		
		if (!security.GetRequest().GetTime().is_not_a_date_time()) {
			request.time = security.GetRequest().GetTime();
		} else {
			request.time = GetNextForwardRequestTime(
				GetContext().GetCurrentTime());
		}

		const auto &expiration = GetContext().GetExpirationCalendar().Find(
			security.GetSymbol(),
			request.time);
		if (!expiration) {
			GetLog().Error(
				"%1%Failed to find expiration info for %1% on the %2%.",
				security,
				request.time);
			throw Exception("Failed to find expiration info");
		}
		request.expiration = *expiration;

		GetLog().Info(
			"%1%Starting to load data for %2% (%3%) from %4% CST (%5% local)"
				", not less then %6% ticks...",
			GetService().GetLogTag(),
			*request.security,
			request.security->GenerateDdfPlusCode(*request.expiration),
			request.time + GetCstTimeZoneDiffLocal(),
			request.time,
			security.GetRequest().GetNumberOfTicks());

		m_requestState = {};
		RequestNext(request);

	}

	void ContinueRequest() {
		
		if (!m_request.security) {
			return;
		}

		auto request = m_request;

		if (request.reversed) {
		
			if (
					!m_requestState.ticks.empty()
					&& request.time >= m_requestState.ticks.cbegin()->first) {
				AssertEq(
					request.time,
					m_requestState.ticks.cbegin()->first);
				m_requestState.ticks.erase(m_requestState.ticks.begin());
			}
		
		} else {

			const auto &lastDatTime = GetLastDataTime();
			if (!lastDatTime.is_not_a_date_time()) {
				request.time = lastDatTime;
			}

		}

		RequestNext(request);

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
			
		Assert(m_request.security);
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
			const auto &messageEnd = HandleTickMessage(
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
				
			const auto &nextIt = HandleTickMessage(it, end, timeMeasurement);
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

		Assert(m_request.security);
		Assert(!m_request.time.is_not_a_date_time());

		const auto &stat = GetReceivedVerbouseStat();
		GetLog().Debug(
			"%1%Completed sub-request for %2% (%3%) from %4% CST (%5% local)."
				" Cached %6% ticks in %7% packs."
				" Total received volume: %8$.02f %9%.",
			GetService().GetLogTag(),
			*m_request.security,
			m_request.security->GenerateDdfPlusCode(*m_request.expiration),
			m_request.time + GetCstTimeZoneDiffLocal(),
			m_request.time,
			m_requestState.numberOfTicks,
			m_requestState.ticks.size(),
			stat.first,
			stat.second);
		if (!m_uncompletedMessagesBuffer.empty()) {
			throw Exception("Unexpected request response end");
		}

		if (
				m_request.reversed > 0
				&& m_requestState.numberOfTicks
					< m_request.security->GetRequest().GetNumberOfTicks()) {

			auto newRequest = m_request;

			if (
					m_requestState.ticks.empty()
					|| m_request.time
						< m_requestState.ticks.cbegin()->first) {
				if (m_request.reversed > 15) {
					GetLog().Error(
						"%1%Empty response for %2%.",
						GetService().GetLogTag(),
						*m_request.security);
					throw Exception("Empty response");
				}
				++m_request.reversed;
			} else {
				newRequest.reversed = 1;
			}

			newRequest.time = GetNextReversedRequestTime();

			RequestNext(newRequest);
			return;

		}

		const auto &lastDataTime = GetLastDataTime();
		const auto &now = GetCurrentTime();
		if (
				!lastDataTime.is_not_a_date_time()
				&& lastDataTime > m_request.time) {

			// Have data from last request.

			auto newRequest = m_request;

			if (lastDataTime.date() >= now.date()) {
				// Last ticks "before now".
				newRequest.time = lastDataTime + pt::seconds(1);
				RequestNext(newRequest);
				return;
			}
			
			// Next day in history.
			AssertLt(lastDataTime.date(), now.date());
			newRequest.time = GetNextForwardRequestTime();
			RequestNext(newRequest);
			
			return;

		}
		
		if (m_request.time.date() >= now.date()) {
			
			// No more time "before now". History request is completed.

			if (
					!m_request.reversed
					&& m_requestState.numberOfTicks
						< m_request.security->GetRequest().GetNumberOfTicks()) {
				// Searching for more ticks...
				auto newRequest = m_request;
				newRequest.time = GetNextReversedRequestTime();
				newRequest.reversed = 1;
				RequestNext(newRequest);
				return;
			}
			
			Flush();
			return;

		}

		// Just weekend or holiday.
		auto newRequest = m_request;
		newRequest.time = GetNextForwardRequestTime();
		RequestNext(newRequest);

	}

private:

	DdfPlus::MarketDataSource::Log & GetLog() const {
		return GetService().GetSource().GetLog();
	}

	const Context & GetContext() const {
		return GetService().GetSource().GetContext();
	}

	void RequestNext(const History::Request &request) {

		Assert(request.security);
		Assert(request.expiration);
		Assert(!request.time.is_not_a_date_time());
		Assert(!m_requestState.ticks.count(request.time));

		boost::format baseMessage(
			"/historical/queryticks.ashx"
				"?username=%1%&password=%2%&symbol=%3%&start=%4%");
		const auto &symbolCode
			= request.security->GenerateDdfPlusCode(*request.expiration);
		baseMessage
			% GetService().GetSettings().login
			% GetService().GetSettings().password
			% symbolCode
			% FormatRequestTime(request.time);

		auto message = baseMessage.str();
		if (request.reversed && !m_requestState.ticks.empty()) {

			Assert(!m_requestState.ticks.cbegin()->second.empty());

			const auto &endTime
				= m_requestState.ticks.cbegin()->second.front().time;
			message += "&end" + FormatRequestTime(endTime);

			GetLog().Debug(
				"%1%Sending reversed sub-request for %2% (%3%)"
					"  from %4% CST (%5% local) to %6% CST (%7%)...",
				GetService().GetLogTag(),
				*request.security,
				symbolCode,
				request.time + GetCstTimeZoneDiffLocal(),
				request.time,
				endTime + GetCstTimeZoneDiffLocal(),
				endTime);

		} else {

			GetLog().Debug(
				"%1%Sending %2%sub-request for %3% (%4%) from %5% CST"
					" (%6% local)...",
				GetService().GetLogTag(),
				request.reversed ? "reversed" : "",
				*request.security,
				symbolCode,
				request.time + GetCstTimeZoneDiffLocal(),
				request.time);

		}

		Request(message, [this, &request]() {m_request = request;});

	}

	Buffer::const_iterator HandleTickMessage(
			const Buffer::const_iterator &begin,
			const Buffer::const_iterator &end,
			const TimeMeasurement::Milestones &) {

		Assert(begin != end);
		Assert(m_request.security);

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

		OnNewTick({time - GetCstTimeZoneDiffLocal(), price, qty, tradingDay});

		return it;
	
	}

	void OnNewTick(const Tick &tick) {
		m_requestState.ticks[m_request.time].emplace_back(tick);
		++m_requestState.numberOfTicks;
	}

	bool SendTickToSecurity(const Tick &tick) {

		if (m_request.tradingDay != tick.tradingDay) {

			if (m_request.tradingDay) {
				Assert(m_request.security->HasExpiration());
				m_request.security->SwitchTradingSession(tick.time);
			} else {
				AssertEq(
					m_request.security->GetLastMarketDataTime(),
					pt::not_a_date_time);
			}
			m_request.tradingDay = tick.tradingDay;

			if (
					m_request.contractHistoryEndTime.is_not_a_date_time()
					|| m_request.contractHistoryEndTime <= tick.time) {
			
				const gr::date contractDate(
					tick.time.date().year(),
					tick.time.date().month(),
					tick.tradingDay);
				auto expiration = GetContext().GetExpirationCalendar().Find(
					m_request.security->GetSymbol(),
					contractDate);
				if (!expiration) {
					GetLog().Error(
						"%1%Failed to find expiration info for %1% on the %2%"
							" (%3%).",
						*m_request.security,
						contractDate,
						tick.time);
					throw Exception("Failed to find expiration info");
				}
				
				if (!m_request.security->HasExpiration()) {
					if (
							m_request.security->SetExpiration(
								tick.time,
								*expiration)) {
						// This is 1-st expiration and someone decided to
						// request more data.
						throw Exception(
							"Request addition data for the first contract"
								" is not implemented");
					}
				} else if (m_request.security->GetExpiration() != *expiration) {
					AssertLt(m_request.security->GetExpiration(), *expiration);
					m_request.security->SetExpiration(
						tick.time,
						*expiration);
					return false;
				}

			}

		}

		Assert(m_request.security->HasExpiration());
	
		m_request.security->AddTrade(
			tick.time,
			m_request.security->ScalePrice(tick.price),
			tick.qty,
			TimeMeasurement::Milestones(),
			true,
			true);

		return true;

	}

	void CompleteRequest() {
		Assert(m_request.security);
		AssertEq(0, m_requestState.ticks.size());
		auto &security = *m_request.security;
		m_request = {};
		m_requestState = {};
		GetService().GetSource().OnHistoryLoadCompleted(security);
	}

	void Flush() {

		pt::ptime contractHistoryEndTime;
		for (const auto &request: m_requestState.ticks) {
			for (const auto &tick: request.second) {
				if (!SendTickToSecurity(tick)) {
					contractHistoryEndTime = tick.time;
					break;
				}
			}
			if (!contractHistoryEndTime.is_not_a_date_time()) {
				break;
			}
		}

		m_requestState.ticks.clear();
		m_requestState.numberOfTicks = 0;
		m_request.reversed = 0;

		if (contractHistoryEndTime.is_not_a_date_time()) {
			CompleteRequest();
			return;
		}
			
		Assert(m_request.security->HasExpiration());
			
		History::Request request = {};
		request.security = m_request.security;
		request.expiration = request.security->GetExpiration();
		request.contractHistoryEndTime = contractHistoryEndTime;
		
		if (!request.security->GetRequest().GetTime().is_not_a_date_time()) {
			request.time = std::min(
				request.security->GetRequest().GetTime(),
				m_request.contractHistoryEndTime);
		} else if (request.security->GetRequest().GetNumberOfTicks()) {
			request.time = GetNextReversedRequestTime(
				request.contractHistoryEndTime);
			request.reversed = 1;
		} else {
			request.time = request.contractHistoryEndTime;
		}

		GetLog().Info(
			"%1%Starting to load data for new contract of %2% (%3%)"
				" from %4% CST (%5% local), not less then %6% ticks...",
			GetService().GetLogTag(),
			*request.security,
			request.security->GenerateDdfPlusCode(*request.expiration),
			request.time + GetCstTimeZoneDiffLocal(),
			request.time,
			request.security->GetRequest().GetNumberOfTicks());

		RequestNext(request);

	}

private:

	pt::ptime GetLastDataTime() const {
		if (
				!m_requestState.ticks.empty()
				&& !m_requestState.ticks.crbegin()->second.empty()) {
			return m_requestState.ticks.crbegin()->second.back().time;
		} else {
			return pt::not_a_date_time;
		}
	}

	static pt::ptime GetNextForwardRequestTime(pt::ptime time) {
		time += GetCstTimeZoneDiffLocal();
		time += pt::hours(24);
		time = pt::ptime(time.date());
		time -= GetCstTimeZoneDiffLocal();
		return time;
	}

	pt::ptime GetNextForwardRequestTime() const {
		return GetNextForwardRequestTime(m_request.time);
	}

	static pt::ptime GetNextReversedRequestTime(pt::ptime time) {
		time += GetCstTimeZoneDiffLocal();
		time -= pt::hours(24);
		time = pt::ptime(time.date());
		time -= GetCstTimeZoneDiffLocal();
		return time;
	}

	pt::ptime GetNextReversedRequestTime() const {
		const auto &lastRequest = m_requestState.ticks.empty()
			?	m_request.time
			:	std::min(m_requestState.ticks.cbegin()->first, m_request.time);
		return GetNextReversedRequestTime(lastRequest);
	}

private:

	History::Request &m_request;
	RequestState &m_requestState;

	std::vector<char> m_uncompletedMessagesBuffer;

};

////////////////////////////////////////////////////////////////////////////////

History::History(
		const Settings &settings,
		DdfPlus::MarketDataSource &source)
	: NetworkStreamClientService("History")
	, m_settings(settings)
	, m_request({})
	, m_requestState({})
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
		new Client(*this, m_request, m_requestState));
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
	InvokeClient<Client>(boost::bind(&Client::ContinueRequest, _1));
}

void History::LoadHistory(DdfPlus::Security &security) {
	InvokeClient<Client>(
		boost::bind(&Client::StartRequest, _1, boost::ref(security)));
}

////////////////////////////////////////////////////////////////////////////////

