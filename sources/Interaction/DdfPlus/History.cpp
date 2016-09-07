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
#include "Common/HttpClient.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::DdfPlus;

namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

namespace {

	class Client : public Lib::HttpClient {

	public:

		typedef Lib::HttpClient Base;

	public:
		
		explicit Client(History &service)
			: Base(
				service,
				service.GetCredentials().host,
				service.GetCredentials().port)
			, m_security(nullptr) {
			//...//
		}
			
		virtual ~Client() {
			//...//
		}

	public:

		void Request(DdfPlus::Security &security) {
			Base::Request(
				"/historical/queryticks.ashx?username=minagad&password=barchart&symbol=CLV16&start=20160824090000&end=20160824120000",
				[this, &security]() {
					Assert(!m_security);
					AssertEq(0, m_uncompletedMessagesBuffer.size());
					m_security = &security;
				});
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
				const pt::ptime &/*time*/,
				const Buffer::const_iterator &begin,
				const Buffer::const_iterator &end,
				const TimeMeasurement::Milestones &/*timeMeasurement*/) {
			
			Assert(m_security);
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
					m_uncompletedMessagesBuffer.cend());
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
				
				const auto &nextIt = HandleMessage(it, end);
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
			const auto &stat = GetReceivedVerbouseStat();
			GetLog().Debug(
				"%1%Completed request for %2%."
					" Total received volume: %3$.02f %4%.",
				GetService().GetLogTag(),
				*m_security,
				stat.first,
				stat.second);
			Assert(m_security);
			if (!m_uncompletedMessagesBuffer.empty()) {
				throw Exception("Unexpected request response end");
			}
			m_security = nullptr;
		}

	private:

		DdfPlus::MarketDataSource::Log & GetLog() const {
			return GetService().GetSource().GetLog();
		}

		const Context & GetContext() const {
			return GetService().GetSource().GetContext();
		}

		Buffer::const_iterator HandleMessage(
				const Buffer::const_iterator &begin,
				const Buffer::const_iterator &end) {

			Assert(begin != end);

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
			time;

			field = std::next(it);
			it = std::find(field, end, ',');
			if (it == end) {
				return end;
			}
			const std::string tradingDay(field, it);
			if (tradingDay.empty()) {
				GetLog().Error(
					"%1%Failed to parse tick trading day field \"%2%\".",
					GetService().GetLogTag(),
					tradingDay);
				throw Exception("Protocol error");
			}

			field = std::next(it);
			it = std::find(field, end, ',');
			if (it == end) {
				return end;
			}
			const std::string session(field, it);
			if (tradingDay.empty()) {
				GetLog().Error(
					"%1%Failed to parse tick session field \"%2%\".",
					GetService().GetLogTag(),
					session);
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

			return it;

		}


	private:

		std::vector<char> m_uncompletedMessagesBuffer;
		DdfPlus::Security *m_security;

	};

}


////////////////////////////////////////////////////////////////////////////////

History::History(
		const Credentials &credentials,
		DdfPlus::MarketDataSource &source)
	: NetworkClientService("History")
	, m_credentials(credentials)
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

const History::Credentials & History::GetCredentials() const {
	return m_credentials;
}

pt::ptime History::GetCurrentTime() const {
	return m_source.GetContext().GetCurrentTime();
}

std::unique_ptr<NetworkClient> History::CreateClient() {
	return std::unique_ptr<NetworkClient>(new Client(*this));
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
	m_source.SubscribeToSecurities();
}

void History::LoadHistory(DdfPlus::Security &security) {
	InvokeClient<Client>(
		boost::bind(&Client::Request, _1, boost::ref(security)));
}

////////////////////////////////////////////////////////////////////////////////

