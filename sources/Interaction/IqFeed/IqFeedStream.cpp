/**************************************************************************
 *   Created: 2016/11/20 20:41:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "IqFeedStream.hpp"
#include "IqFeedMarketDataSource.hpp"
#include "IqFeedSecurity.hpp"
#include "Core/Settings.hpp"
#include "Common/NetworkStreamClient.hpp"
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Interaction;
using namespace trdk::Interaction::IqFeed;

namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

namespace {

	class Client : public NetworkStreamClient {

	public:

		typedef NetworkStreamClient Base;

	public:

		explicit Client(ClientService &service, const size_t port)
			: Base(service, "localhost", port)
			, m_timeZoneDiff(
				GetEstTimeZoneDiff(
					service
						.GetSource()
						.GetContext()
						.GetSettings()
						.GetTimeZone())) {
			//...//
		}

		virtual ~Client() {
			//...//
		}

	public:

		const Context & GetContext() const {
			return GetService().GetSource().GetContext();
		}

		IqFeed::MarketDataSource::Log & GetLog() const {
			return GetService().GetSource().GetLog();
		}

		virtual void SubscribeToMarketData(const IqFeed::Security &) = 0;

	protected:

		virtual void OnStart() override {
			SendSynchronously(
				"S,SET CLIENT NAME," TRDK_BUILD_IDENTITY "\r\n",
				"SET CLIENT NAME");
		}

		virtual ClientService & GetService() override {
			return *boost::polymorphic_downcast<ClientService *>(
				&Base::GetService());
		}
		virtual const ClientService & GetService() const override {
			return *boost::polymorphic_downcast<const ClientService *>(
				&Base::GetService());
		}

		virtual Milestones StartMessageMeasurement() const override {
			return GetContext().StartStrategyTimeMeasurement();
		}

		virtual pt::ptime GetCurrentTime() const override {
			return GetContext().GetCurrentTime();
		}

		virtual void LogDebug(const std::string &message) const override {
			GetLog().Debug(message.c_str());
		}

		virtual void LogInfo(const std::string &message) const override {
			GetLog().Info(message.c_str());
		}
		
		virtual void LogWarn(const std::string &message) const override {
			GetLog().Warn(message.c_str());
		}
		
		virtual void LogError(const std::string &message) const override {
			GetLog().Error(message.c_str());
		}

	protected:

		template<typename Source>
		pt::ptime ConvertIqFeedStringToPtime(const Source &source) const {

			boost::iostreams::stream_buffer<boost::iostreams::array_source> buffer(
				&*boost::begin(source), &*boost::end(source));
			std::istream stream(&buffer);

			{
				static const std::locale locale(
					std::locale::classic(),
					new pt::time_input_facet("%Y-%m-%d %H:%M:%S%f"));
				stream.imbue(locale);
			}
		
			pt::ptime result;
			stream >> result;

			if (result == pt::not_a_date_time) {
				boost::format message("Failed to parse time value \"%1%\"");
				message % boost::copy_range<std::string>(source);
				throw NetworkStreamClient::Exception(message.str().c_str());
			}

			return result - m_timeZoneDiff;

		}

		template<typename Source>
		pt::ptime ConvertIqFeedStringToPtime(
				const Source &source,
				const pt::ptime &now) {

			boost::iostreams::stream_buffer<boost::iostreams::array_source> buffer(
				&*boost::begin(source), &*boost::end(source));
			std::istream stream(&buffer);

			{
				static const std::locale locale(
					std::locale::classic(),
					new pt::time_input_facet("%H:%M:%S%f"));
				stream.imbue(locale);
			}
		
			pt::time_duration result;
			stream >> result;

			if (result == pt::not_a_date_time) {
				boost::format message(
					"Failed to parse time of day value \"%1%\"");
				message % boost::copy_range<std::string>(source);
				throw NetworkStreamClient::Exception(message.str().c_str());
			}

			switch (result.hours()) {
				case 0:
					{
						const auto nowTz = now + m_timeZoneDiff;
						switch (nowTz.time_of_day().hours()) {
							case 23:
								return
									pt::ptime(
										(nowTz + pt::hours(1)).date(),
										result)
									- m_timeZoneDiff;
						case 0:
							break;
						default:
							{
								boost::format message(
									"Time of day %1%"
										" differs from the current time %2%");
								message % result % nowTz;
								throw NetworkStreamClient::Exception(
									message.str().c_str());
							}
						}
					}
					break;
				case 23:
					{
						const auto nowTz = now + m_timeZoneDiff;
						switch (nowTz.time_of_day().hours()) {
							case 0:
								return
									pt::ptime(
										(nowTz - pt::hours(1)).date(),
										result)
									- m_timeZoneDiff;
							case 22:
							case 23:
								break;
							default:
								{
									boost::format message(
										"Time of day %1% differs"
											" from the current time %2%");
									message % result % nowTz;
									throw NetworkStreamClient::Exception(
										message.str().c_str());
								}
						}
						break;
					}
			}

			return pt::ptime(now.date(), result) - m_timeZoneDiff;

		}

	private:

		const pt::time_duration m_timeZoneDiff;

	};

}

////////////////////////////////////////////////////////////////////////////////

ClientService::ClientService(
		const std::string &name,
		IqFeed::MarketDataSource &source)
	: NetworkStreamClientService(name)
	, m_source(source) {
	//...//
}

ClientService::~ClientService() noexcept {
	try {
		Stop();
	} catch (...) {
		AssertFailNoException();
		terminate();
	}
}

IqFeed::MarketDataSource & ClientService::GetSource() {
	return m_source;
}
const IqFeed::MarketDataSource & ClientService::GetSource() const {
	return const_cast<ClientService *>(this)->GetSource();
}

pt::ptime ClientService::GetCurrentTime() const {
	return m_source.GetContext().GetCurrentTime();
}

void ClientService::LogDebug(const char *message) const {
	m_source.GetLog().Debug(message);
}

void ClientService::LogInfo(const std::string &message) const {
	m_source.GetLog().Info(message.c_str());
}

void ClientService::LogError(const std::string &message) const {
	m_source.GetLog().Error(message.c_str());
}

void ClientService::OnConnectionRestored() {
	m_source.ResubscribeToSecurities();
}

void ClientService::SubscribeToMarketData(const IqFeed::Security &security) {
	InvokeClient<Client>(
		boost::bind(&Client::SubscribeToMarketData, _1, boost::cref(security)));
}

////////////////////////////////////////////////////////////////////////////////

namespace {

	class StreamClient : public Client {

	public:

		typedef Client Base;

	public:

		explicit StreamClient(ClientService &service)
			: Base(service, 5009) {
			//...//
		}

		virtual ~StreamClient() {
			//...//
		}

	public:

		virtual void SubscribeToMarketData(
				const IqFeed::Security &security)
				override {

			GetLog().Info(
				"%1%Sending market data request for %2% (%3%)...",
				GetService().GetLogTag(),
				security,
				security.GetSymbol().GetSymbol());

			if (security.GetSymbol().IsExplicit()) {
				throw Exception("Source works only with not explicit symbols");
			}

			Assert(security.IsLevel1Required());
			if (!security.IsLevel1Required()) {
				GetLog().Warn(
					"%1%Market data is not required by %2%.",
					GetService().GetLogTag(),
					security);
				return;
			}

			// http://iqfeed.net/symbolguide/index.cfm?symbolguide=guide&displayaction=support&section=guide&web=iqfeed&guide=commod&web=IQFeed&symbolguide=guide&displayaction=support&section=guide&type=cme&type2=cme-mini&type3=cme_gbx
			boost::format command("wQ%1%%2%\r\n");
			command
				% security.GetSymbol().GetSymbol()
				% security.GetExpiration().GetContract(false);
			Send(command.str());

		}

	protected:

		virtual void OnStart() override {
			Base::OnStart();
			SendSynchronously("S,SET PROTOCOL,5.2\r\n", "SET PROTOCOL");
			SendSynchronously(
				"S"
					",SELECT UPDATE FIELDS"
					",Ask,Ask Size,Ask Time" // 1, 2, 3
					",Bid,Bid Size,Bid Time" // 4, 5, 6
					",Most Recent Trade,Most Recent Trade Size" // 7, 8
					",Most Recent Trade Time,Most Recent Trade Date" // 9, 10
					",Total Volume" // 11
					",Market Open" // 12
					"\r\n",
				"SELECT UPDATE FIELDS");
		}

		virtual Buffer::const_iterator FindLastMessageLastByte(
				const Buffer::const_iterator &,
				const Buffer::const_iterator &begin,
				const Buffer::const_iterator &end)
				const
				override {
			Assert(begin != end);
			const Buffer::const_reverse_iterator rbegin(end);
			const Buffer::const_reverse_iterator rend(begin);
			const auto messageEnd = std::find(rbegin, rend, '\n');
			if (messageEnd == rend) {
				return end;
			}
			auto result = messageEnd.base();
			if (result == begin) {
				return end;
			}
			return --result;
		}

		virtual void HandleNewMessages(
				const boost::posix_time::ptime &time,
				const Buffer::const_iterator &bufferBegin,
				const Buffer::const_iterator &end,
				const Milestones &measurement)
				override {

			Assert(bufferBegin != end);
			AssertEq('\n', *end);
			AssertLt(0, std::distance(bufferBegin, end));

			for (auto it = bufferBegin; it < end; ++it) {

				const auto &messageType = *it++;
				if (*it != ',') {
					boost::format message(
						"Failed to find delimiter after type '%1%'");
					message % messageType;
					throw ProtocolError(message.str().c_str(), &*it, ',');
				}
				if (*++it == '\n') {
					throw ProtocolError("Second field is too short", &*it, 0);
				}

				switch (messageType) {
					case 'S':
						it = OnServiceMessage(it, end);
						AssertEq('\n', *it);
						break;
					case 'T':
						it = OnTimeMessage(time, it, end);
						AssertEq('\n', *it);
						break;
					case 'Q':
						it = OnSymbolUpdateMessage(time, it, end, measurement);
						AssertEq('\n', *it);
						break;
					case 'F':
						it = OnFundamentalMessage(it, end);
						AssertEq('\n', *it);
						break;
					case 'P':
						it = OnSummaryMessage(it, end);
						AssertEq('\n', *it);
						break;
					case 'E':
						it = OnErrorMessage(it, end);
						AssertEq('\n', *it);
						break;
					default:
						{
							boost::format message("Unknown message type '%1%'");
							message % messageType;
							throw ProtocolError(
								message.str().c_str(),
								&*std::prev(std::prev(it)),
								0);
						}
						break;
				}

			}

		}

	private:

		Buffer::const_iterator OnErrorMessage(
				const Buffer::const_iterator &messageBegin,
				const Buffer::const_iterator &bufferEnd) {
			GetLog().Error(
				"%1%IQFeed server error: \"%2%\".",
				GetService().GetLogTag(),
				std::string(
					messageBegin,
					std::find(messageBegin, bufferEnd, '\n')));
			throw Exception("IQFeed server error");
		}

		Buffer::const_iterator OnServiceMessage(
				const Buffer::const_iterator &messageBegin,
				const Buffer::const_iterator &bufferEnd) {
		
			AssertEq('\n', *bufferEnd);
			AssertLt(0, std::distance(messageBegin, bufferEnd));

			typedef boost::split_iterator<Buffer::const_iterator> Iterator;
			const auto &it
				= boost::make_iterator_range(messageBegin, bufferEnd);
			auto field = boost::make_split_iterator(
				it,
				boost::token_finder(boost::is_any_of(",\n")));

			if (*field == std::string("KEY")) {
				if (++field == Iterator() || *boost::end(*field) != '\n') {
					throw ProtocolError(
						"Service message \"KEY\" has invalid format",
						&*boost::end(*field),
						'\n');
				}
				return boost::end(*field);
			} else if (*field == std::string("SERVER CONNECTED")) {
				GetLog().Info("%1%Server connected.", GetService().GetLogTag());
				if (*boost::end(*field) != '\n') {
					throw ProtocolError(
						"Service message \"Server state\" has invalid format",
						&*boost::end(*field),
						'\n');
				}
				return boost::end(*field);
			} else if (*field == std::string("SERVER DISCONNECTED")) {
				GetLog().Warn(
					"%1%Server disconnected.",
					GetService().GetLogTag());
				if (*boost::end(*field) != '\n') {
					throw ProtocolError(
						"Service message \"Server state\" has invalid format",
						&*boost::end(*field),
						'\n');
				}
				return boost::end(*field);
			} else if (*field == std::string("IP")) {
				while (++field != Iterator() && *boost::end(*field) != '\n');
				if (*boost::end(*field) != '\n') {
					throw ProtocolError(
						"Service message \"Server state\" has invalid format",
						&*boost::end(*field),
						'\n');
				}
				if (*boost::end(*field) != '\n') {
					throw ProtocolError(
						"Service message \"IP\" has invalid format",
						&*boost::end(*field),
						'\n');
				}
				return boost::end(*field);
			} else if (*field == std::string("CUST")) {
				boost::format message(
					"%1%IQFeed customer info"
						": Service Type=\"%2%\"; IP=\"%3%\"; Port=\"%4%\""
						"; Token=\"%5%\"; Version=\"%6%\""
						"; Deprecated(8)=\"%7%\"; Verbose Exchanges=\"%8%\""
						"; Deprecated(10)=\"%9%\"; Max Symbols=\"%10%\""
						"; Flags=\"%11%\"; Deprecated(13)=\"%12%\""
						"; Deprecated(14)=\"%13%\".");
				message % GetService().GetLogTag();
				for (size_t i = 0; i < 12; ++i) {
					if (++field == Iterator()) {
						throw ProtocolError(
							"Service message \"CUST\" has invalid format",
							&*boost::end(*field),
							0);
					}
					message % boost::copy_range<std::string>(*field);
				}
				GetLog().Info(message.str().c_str());
				if (*boost::end(*field) != '\n') {
					throw ProtocolError(
						"Service message \"CUST\" has invalid format",
						&*boost::end(*field),
						'\n');
				}
				return boost::end(*field);
			} else if (*field == std::string("KEYOK")) {
				if (*boost::end(*field) != '\n') {
					throw ProtocolError(
						"Service message \"KEYOK\" has invalid format",
						&*boost::end(*field),
						'\n');
				}
				return boost::end(*field);
			} else if (*field == std::string("CURRENT UPDATE FIELDNAMES")) {
				if (++field == Iterator() || *boost::end(*field) != ',') {
					throw ProtocolError(
						"Service message \"SELECT UPDATE FIELDS\""
							" has invalid format",
						&*boost::end(*field),
						',');
				}
				const auto &message = boost::make_iterator_range(
					boost::begin(*field),
					std::find(boost::end(*field), bufferEnd, '\n'));
				if (m_level1SymbolBuffer.capacity()) {
					GetLog().Error(
						"%1%Wrong field set: \"%2%\".",
						GetService().GetLogTag(),
						boost::copy_range<std::string>(message));
					throw Exception("Wrong field set");
				}
				const std::string controlContent(
					"Symbol"
					",Ask,Ask Size,Ask Time"
					",Bid,Bid Size,Bid Time"
					",Most Recent Trade,Most Recent Trade Size"
					",Most Recent Trade Time,Most Recent Trade Date"
					",Total Volume"
					",Market Open");
				if (message != controlContent) {
					GetLog().Debug(
						"%1%Initial field set: \"%2%\".",
						GetService().GetLogTag(),
						boost::copy_range<std::string>(message));
				} else {
					GetLog().Debug(
						"%1%Custom field set: \"%2%\".",
						GetService().GetLogTag(),
						boost::copy_range<std::string>(message));
					m_level1SymbolBuffer.reserve(7);
				}
				return boost::end(message);
			} else if (*field == std::string("CURRENT PROTOCOL")) {
				if (++field == Iterator() || *boost::end(*field) != ',') {
					throw ProtocolError(
						"Service message \"CURRENT PROTOCOL\""
							" has invalid format",
						&*boost::end(*field),
						',');
				}
				const auto &message = boost::make_iterator_range(
					boost::begin(*field),
					std::find(boost::end(*field), bufferEnd, '\n'));
				if (message != std::string("5.2,")) {
					GetLog().Error(
						"%1%Unknown protocol: \"%2%\".",
						GetService().GetLogTag(),
						boost::copy_range<std::string>(message));
					throw Exception("Wrong field set");
				}
				return boost::end(message);
			} else {
				throw ProtocolError(
					"Unknown service message type",
					&*messageBegin,
					0);
			}

		}

		Buffer::const_iterator OnTimeMessage(
				const pt::ptime &time,
				const Buffer::const_iterator &messageBegin,
				const Buffer::const_iterator &bufferEnd) {
			const auto &end = std::find(messageBegin, bufferEnd, '\n');
			if (
					m_lastTimeReportTime == pt::not_a_date_time
					|| m_lastTimeReportTime + pt::minutes(10) <= time) {
				GetLog().Debug(
					"%1%Server time: %2%.",
					GetService().GetLogTag(),
					std::string(messageBegin, end));
				m_lastTimeReportTime = time;
			}
			return end;
		}

		Buffer::const_iterator OnFundamentalMessage(
				const Buffer::const_iterator &messageBegin,
				const Buffer::const_iterator &bufferEnd) {
			return std::find(messageBegin, bufferEnd, '\n');
		}

		Buffer::const_iterator OnSummaryMessage(
				const Buffer::const_iterator &messageBegin,
				const Buffer::const_iterator &bufferEnd) {
			return std::find(messageBegin, bufferEnd, '\n');
		}

		Buffer::const_iterator OnSymbolUpdateMessage(
			const pt::ptime &now,
				const Buffer::const_iterator &messageBegin,
				const Buffer::const_iterator &bufferEnd,
				const Milestones &measurement) {
			
			AssertEq('\n', *bufferEnd);
			AssertLt(0, std::distance(messageBegin, bufferEnd));
			
			if (m_level1SymbolBuffer.capacity() != 7) {
				throw Exception("Field set is not set");
			}
			Assert(m_level1SymbolBuffer.empty());

			typedef boost::split_iterator<Buffer::const_iterator>
				Iterator;
			const auto &it
				= boost::make_iterator_range(messageBegin, bufferEnd);
			auto field = boost::make_split_iterator(
				it,
				boost::token_finder(boost::is_any_of(",\n")));

			IqFeed::Security *security = nullptr;
			pt::ptime time;
			pt::ptime tradeTime;
			bool isMarketOpened;
			for (size_t i = 0; field != Iterator(); ++field, ++i) {

				try {

					switch (i) {

						case 0:
							if (
									std::distance(field->begin(), field->end())
										< 5) {
								throw Exception("Symbol format is wrong");
							}
							security
								= GetService()
								.GetSource()
								.FindSecurityBySymbolString(
									std::string(
										field->begin() + 1,
										field->end() - 3));
							if (!security) {
								throw Exception(
									"Symbol update has unknown symbol");
							}
							break;

						case 1: // Ask
							m_level1SymbolBuffer.emplace_back(
								LEVEL1_TICK_ASK_PRICE,
								security->ScalePrice(
									boost::lexical_cast<double>(*field)));
							break;

						case 2: // Ask Size
							m_level1SymbolBuffer.emplace_back(
								LEVEL1_TICK_ASK_QTY,
								boost::lexical_cast<double>(*field));
							break;

						case 4: // Bid
							m_level1SymbolBuffer.emplace_back(
								LEVEL1_TICK_BID_PRICE,
								security->ScalePrice(
									boost::lexical_cast<double>(*field)));
							break;

						case 5: // Bid Size
							m_level1SymbolBuffer.emplace_back(
								LEVEL1_TICK_BID_QTY,
								boost::lexical_cast<double>(*field));
							break;

						case 7: // Last trade price
							m_level1SymbolBuffer.emplace_back(
								LEVEL1_TICK_LAST_PRICE,
								security->ScalePrice(
									boost::lexical_cast<double>(*field)));
							break;
						case 8: // Last trade size
							m_level1SymbolBuffer.emplace_back(
								LEVEL1_TICK_LAST_QTY,
								boost::lexical_cast<double>(*field));
							break;

						case 10: // Last trade date
							break;

						case 11: // Total Volume
							m_level1SymbolBuffer.emplace_back(
								LEVEL1_TICK_TRADING_VOLUME,
								boost::lexical_cast<double>(*field));
							break;

						case 12: // Market Open
							// 1 = market open, 0 = market closed NOTE: This
							// field is valid for Futures and Future Options
							// only.
							isMarketOpened = boost::lexical_cast<int>(*field)
								? true
								: false;
							break;

						case 3: // Ask Time
						case 6: // Bid Time
						case 9: // Last Time
							{
								const auto &newTime
									= ConvertIqFeedStringToPtime(*field, now);
								if (i == 9) {
									tradeTime = newTime;
								}
								Assert(newTime != pt::not_a_date_time);
								if (
										time == pt::not_a_date_time
										|| time < newTime) {
									time = std::move(newTime);
								}
							}
							break;

						case 13:
							if (!field->empty()) {
								throw ProtocolError(
									"Symbol update message has not"
										"last empty field",
									&*field->end(),
									'\r');
							}
							break;

						default:

							throw ProtocolError(
								"Symbol update has too many fields",
								&*boost::end(*field),
								0);

					}

					if (*boost::end(*field) == '\n') {
						
						if (i != 13) {
							throw ProtocolError(
								"Symbol update has too few fields",
								&*boost::end(*field),
								0);
						}
						Assert(security);
						AssertEq(7, m_level1SymbolBuffer.size());
						AssertEq(
							LEVEL1_TICK_LAST_PRICE,
							m_level1SymbolBuffer[4].GetType());
						AssertEq(
							LEVEL1_TICK_LAST_QTY,
							m_level1SymbolBuffer[5].GetType());
						Assert(time != pt::not_a_date_time);
						Assert(tradeTime != pt::not_a_date_time);

						if (time >= tradeTime) {
							if (
									tradeTime
										>= security->GetLastMarketDataTime()) {
								security->AddTrade(
									tradeTime,
									ScaledPrice(
										m_level1SymbolBuffer[4].GetValue()),
									m_level1SymbolBuffer[5].GetValue(),
									measurement,
									false,
									false);
								security->SetLevel1(
									time,
									m_level1SymbolBuffer,
									measurement);
							}
						} else if (time >= security->GetLastMarketDataTime()) {
							security->SetLevel1(
								time,
								m_level1SymbolBuffer,
								measurement);
							security->AddTrade(
								tradeTime,
								ScaledPrice(m_level1SymbolBuffer[4].GetValue()),
								m_level1SymbolBuffer[5].GetValue(),
								measurement,
								false,
								false);
						}
						m_level1SymbolBuffer.clear();

						return boost::end(*field);
					
					}

				} catch (const boost::bad_lexical_cast &) {

					auto errorField = boost::make_split_iterator(
						it,
						boost::token_finder(boost::is_any_of(",\n")));
					for (
							size_t j = 1;
							errorField != Iterator();
							++errorField, ++j) {
						if (j == 3) {
							if (*errorField == std::string("Not Found")) {
								throw Exception("Server did not found symbol");
							}
							break;
						}
					}

					throw ProtocolError(
						"Symbol update message field has invalid format",
						&*boost::end(*field),
						0);

				}

			}

			throw ProtocolError(
				"Symbol update message has invalid format",
				&*bufferEnd,
				'\n');

		}

	private:

		std::vector<trdk::Level1TickValue> m_level1SymbolBuffer;
		pt::ptime m_lastTimeReportTime;

	};

}

Stream::Stream(IqFeed::MarketDataSource &source)
	: ClientService("Stream", source) {
	//...//
}

Stream::~Stream() noexcept {
	//...//
}

std::unique_ptr<NetworkStreamClient> Stream::CreateClient() {
	return boost::make_unique<StreamClient>(*this);
}

////////////////////////////////////////////////////////////////////////////////

namespace {

	class HistoryClient : public Client {

	public:

		typedef Client Base;

	public:

		explicit HistoryClient(ClientService &service)
			: Base(service, 9100) {
			//...//
		}

		virtual ~HistoryClient() {
			//...//
		}

	public:

		virtual void SubscribeToMarketData(
				const IqFeed::Security &security)
				override{

			GetLog().Info(
				"%1%Sending %2% (%3% per bar) market data history points"
					" request for %4% (%5%)...",
				GetService().GetLogTag(),
				GetService().GetSource().GetSettings().historyDepth,
				GetService().GetSource().GetSettings().historyBarSize,
				security,
				security.GetSymbol().GetSymbol());

			if (security.GetSymbol().IsExplicit()) {
				throw Exception("Source works only with not explicit symbols");
			}

			Assert(security.IsLevel1Required());
			if (!security.IsLevel1Required()) {
				GetLog().Warn(
					"%1%Market data is not required by %2%.",
					GetService().GetLogTag(),
					security);
				return;
			}

			// http://www.iqfeed.net/dev/api/docs/HistoricalviaTCPIP.cfm
			// HIX,[Symbol],[Interval],[MaxDatapoints],[DataDirection]
			// ,[RequestID],[DatapointsPerSend],[IntervalType]<CR><LF>
			boost::format command("HIX,Q%1%#C,%2%,%3%,1,%1%,,t\r\n");
			command
				% security.GetSymbol().GetSymbol()
				% GetService().GetSource().GetSettings().historyBarSize
				% GetService().GetSource().GetSettings().historyDepth;
			Send(command.str());

		}

	protected:

		virtual void OnStart() override {
			Base::OnStart();
		}

		virtual Buffer::const_iterator FindLastMessageLastByte(
				const Buffer::const_iterator &,
				const Buffer::const_iterator &begin,
				const Buffer::const_iterator &end)
				const
				override {
			Assert(begin < end);
			const char *const marker = "\r\n";
			const auto result = std::find_end(begin, end, marker, marker + 2);
			if (result == end) {
				return end;
			}
			return std::next(result);
		}

		virtual void HandleNewMessages(
				const boost::posix_time::ptime &/*time*/,
				const Buffer::const_iterator &bufferBegin,
				const Buffer::const_iterator &bufferEnd,
				const Milestones &/*measurement*/)
				override {
			Assert(bufferBegin != bufferEnd);
			AssertEq('\n', *bufferEnd);
			AssertEq('\r', *std::prev(bufferEnd));
			AssertLt(0, std::distance(bufferBegin, std::next(bufferEnd)));
			typedef boost::split_iterator<Buffer::const_iterator> Iterator;
			const auto messages = boost::make_iterator_range(
				bufferBegin,
				std::next(bufferEnd));
			for (
					auto message = boost::make_split_iterator(
						messages,
						boost::first_finder("\r\n", boost::is_iequal()));
					message != Iterator();
					++message) {
				if (message->empty()) {
					continue;
				}
				HandleMessage(*message);
			}
		}

	private:

		template<typename Message>
		void HandleMessage(const Message &message) {

			typedef boost::split_iterator<Message::iterator> Iterator;
			auto field = boost::make_split_iterator(
				message,
				boost::first_finder(",", boost::is_iequal()));
			if (field == Iterator()) {
				throw ProtocolError(
					"Received empty history message, no one field",
					&*boost::end(*field),
					',');
			}

			const auto &getNext
				= [&field]() -> boost::iterator_range<Message::iterator> {
					Assert(field != Iterator());
					if (++field == Iterator()) {
						throw ProtocolError(
							"Failed to extract history message field",
							&*boost::end(*field),
							',');
					}
#					ifdef DEV_VER
						const std::string str = boost::copy_range<std::string>(
							*field);
#					endif
					return *field;
				};

			try {

				IqFeed::Security *const security
					= GetService()
					.GetSource()
					.FindSecurityBySymbolString(
						boost::copy_range<std::string>(*field));
				if (!security) {
					GetLog().Error(
						"%1%History message \"%2%\""
							" has unknown symbol \"%3%\".",
						GetService().GetLogTag(),
						boost::copy_range<std::string>(message),
						boost::copy_range<std::string>(*field));
					throw Exception("History message has unknown symbol");
				} else if (security->IsOnline()) {
					throw Exception(
						"Failed to write history data to security"
							" as it online");
				}

				const auto &secondField = getNext();
				if (boost::equals(secondField, "!ENDMSG!")) {
					const auto &last = getNext();
					if (!last.empty()) {
						throw ProtocolError(
							"History message has wrong format",
							&*boost::end(last),
							'\r');
					}
					GetService().GetSource().OnHistoryLoadCompleted(*security);
					return;
				}

				IqFeed::Security::Bar bar(
					ConvertIqFeedStringToPtime(secondField),
					IqFeed::Security::Bar::TRADES);
				bar.highPrice = security->ScalePrice(
					boost::lexical_cast<double>(getNext()));
				bar.lowPrice = security->ScalePrice(
					boost::lexical_cast<double>(getNext()));
				bar.openPrice = security->ScalePrice(
					boost::lexical_cast<double>(getNext()));
				bar.closePrice = security->ScalePrice(
					boost::lexical_cast<double>(getNext()));
				// Total Volume.
				getNext(); 
				bar.volume = boost::lexical_cast<double>(getNext());
				bar.numberOfPoints
					= GetService().GetSource().GetSettings().historyBarSize;

				{
					const auto &last = getNext();
					if (!last.empty()) {
						throw ProtocolError(
							"History message has wrong format",
							&*boost::end(last),
							'\r');
					}
				}

				security->AddBar(std::move(bar));

			} catch (const boost::bad_lexical_cast &) {
				throw ProtocolError(
					"History message field has invalid format",
					&*boost::end(*field),
					0);
			}

		}

	};

}

History::History(IqFeed::MarketDataSource &source)
	: ClientService("History", source) {
	//...//
}

History::~History() noexcept {
	//...//
}

std::unique_ptr<NetworkStreamClient> History::CreateClient() {
	return boost::make_unique<HistoryClient>(*this);
}

////////////////////////////////////////////////////////////////////////////////
