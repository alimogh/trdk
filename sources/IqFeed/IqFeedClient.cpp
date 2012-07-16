/**************************************************************************
 *   Created: 2012/6/6/ 20:02
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#pragma once

#include "Prec.hpp"
#include "IqFeedClient.hpp"
#include "IqMessageParser.hpp"
#include "Core/Security.hpp"

namespace pt = boost::posix_time;
namespace lt = boost::local_time;
namespace io = boost::asio;
namespace fs = boost::filesystem;

//////////////////////////////////////////////////////////////////////////

namespace {

	typedef boost::mutex LogMutex;
	typedef LogMutex::scoped_lock LogLock;

	typedef io::ip::tcp Proto;
	typedef Proto::resolver Resolver;
	typedef Proto::resolver::iterator ResolverIterator;
	typedef Proto::endpoint Endpoint;
	typedef Proto::socket Socket;

	typedef boost::circular_buffer<char> MessagesBuffer;
	typedef IqMessageParser<MessagesBuffer> MessageParser;

	typedef std::map<std::string, boost::shared_ptr<DynamicSecurity>> UpdatesSubscribers;
	typedef std::map<std::string, boost::shared_ptr<DynamicSecurity>> HistorySubscribers;

}

//////////////////////////////////////////////////////////////////////////

#define IQFEED_CLIENT_CONNECTION_NAME "IQFeed"

namespace {

	const pt::time_duration pingRequestPeriod = pt::seconds(120);
	const pt::time_duration timeout = pt::seconds(15);

}

namespace {

	void WriteLogRecordHead(std::ostream &os, unsigned short port) {
		os << pt::microsec_clock::local_time() << " [" << GetCurrentThreadId() << ":" << port << "]: ";
	}

}

//////////////////////////////////////////////////////////////////////////

namespace {

	//////////////////////////////////////////////////////////////////////////

	template<typename Service>
	class Connection : private boost::noncopyable {

	public:

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;

	protected:

		typedef std::vector<char> SocketBuffer;
		
	private:

		typedef boost::condition_variable Condition;

	public:

		Connection(Service &service, unsigned short port)
				: m_host("127.0.0.1"),
				m_port(port),
				m_isActive(false),
				m_thread(nullptr),
				m_service(service),
				m_socketBuffer(1024 * 4),
				m_messagesBuffer(1024 * 8) {
			//...//
		}

		virtual ~Connection() {
			try {
				Close();
			} catch (...) {
				AssertFail("Unhandled exception caught");
				throw;
			}
		}

	public:

		void Connect() {

			Log::Info(
				IQFEED_CLIENT_CONNECTION_NAME ": connecting to IQLink at \"%1%:%2%...",
				m_host,
				m_port);

			Lock lock(m_mutex);

			Assert(!m_isActive);
			Assert(!m_thread);
			if (m_thread) {
				return;
			}
			m_isActive = false;

			m_ioService.reset(new io::io_service);
			m_socket.reset(new Socket(*m_ioService));
		
			Proto::resolver::query query(
				Proto::v4(),
				m_host,
				boost::lexical_cast<std::string>(m_port));
			Resolver resolver(*m_ioService);
			resolver.async_resolve(
				query,
				[this](
							const boost::system::error_code &error,
							const ResolverIterator &endpointIterator) {
					HandleResolve(error, endpointIterator);
				});

			std::unique_ptr<boost::thread> thread(
				new boost::thread(
					[this]() {
						Task();
					}));
			if (!m_condition.timed_wait(lock, boost::get_system_time() + timeout)) {
				Log::Error(IQFEED_CLIENT_CONNECTION_NAME " connection error: failed to start task thread.");
				m_isActive = false;
				m_ioService->stop();
				m_condition.notify_all();
				thread->join();
				m_socket.reset();
				m_ioService.reset();
				throw IqFeedClient::ConnectError();
			} else if (!m_isActive) {
				Log::Error(IQFEED_CLIENT_CONNECTION_NAME " connection: failed to connect to IQLink.");
				m_ioService->stop();
				m_condition.notify_all();
				thread->join();
				m_socket.reset();
				m_ioService.reset();
				throw IqFeedClient::ConnectError();
			}

			Log::Info(IQFEED_CLIENT_CONNECTION_NAME ": connection stated.");
			m_thread.reset(thread.release());

		}

		void Close() {
			{
				const Lock lock(m_mutex);
				if (!m_thread) {
					return;
				}
				Assert(m_ioService);
				Assert(m_socket);
				m_isActive = false;
				m_ioService->stop();
				m_condition.notify_all();
			}
			m_thread->join();
			m_thread.reset();
		}

		void CheckState() {
			if (!m_isActive) {
				Log::Error(IQFEED_CLIENT_CONNECTION_NAME " connection doesn't exist");
				throw IqFeedClient::Error(IQFEED_CLIENT_CONNECTION_NAME " connection doesn't exist");
			}
		}

		void Send(const std::string &message) {
			m_service.DumpSend(message, m_port);
			io::write(*m_socket, io::buffer(message));
		}

		size_t GetSocketBufferSize() const {
			return m_socketBuffer.size();
		}

	protected:

		void DumpReceived(const MessageParser &message) {
			m_service.DumpReceived(message, m_port);
		}

	private:

		void Task() {
			Log::Info("Started " IQFEED_CLIENT_CONNECTION_NAME " connection read task.");
			try {
				m_ioService->run();
			} catch (const std::exception &ex) {
				Log::Error(IQFEED_CLIENT_CONNECTION_NAME " connection unhandled exception: \"%1%\".", ex.what());
				AssertFail("Unhandled exception caught");
				throw;
			} catch (...) {
				AssertFail("Unhandled exception caught");
				throw;
			}
			Log::Info("Stopped " IQFEED_CLIENT_CONNECTION_NAME " connection read task.");
		}

	protected:

		virtual void HandleMessage(MessageParser &) = 0;


	private:

		void HandleRead(const boost::system::error_code &error, size_t size) {
			
			const Lock lock(m_mutex);
			
			if (size == 0) {
				Log::Info(IQFEED_CLIENT_CONNECTION_NAME " connection CLOSED.");
				m_isActive = false;
				return;
			} else if (error) {
				Log::Error(
					IQFEED_CLIENT_CONNECTION_NAME " connection error: \"%1%\" (%2%).",
					error.message(),
					error.value());
				Log::Error(IQFEED_CLIENT_CONNECTION_NAME " connection CLOSED.");
				m_isActive = false;
				return;
			} else if (!m_isActive) {
				return;
			}

			const size_t oldSize = m_messagesBuffer.size();
			if (m_messagesBuffer.max_size() < oldSize + size) {
				Log::Warn(
					IQFEED_CLIENT_CONNECTION_NAME ": exceeded the internal buffer"
						" for %1% (current capacity: %2%, new data size: %2%)."
						" Buffer will be reallocated.",
					m_port,
					m_messagesBuffer.max_size(),
					size);
				MessagesBuffer newBuffer(oldSize + size);
				newBuffer.resize(oldSize);
				memcpy(&newBuffer[0], &m_messagesBuffer[0], oldSize);
				newBuffer.swap(m_messagesBuffer);
			}
			m_messagesBuffer.resize(oldSize + size);
			memcpy(&m_messagesBuffer[oldSize], &m_socketBuffer[0], size);
			
			for ( ; ; ) {
				MessagesBuffer::iterator end = std::find(
					m_messagesBuffer.begin(),
					m_messagesBuffer.end(),
					'\n');
				if (end == m_messagesBuffer.end()) {
					break;
				}
				MessageParser message(m_messagesBuffer.begin(), end);
				if (*message.GetBegin() == 'E') {
					Log::Error(IQFEED_CLIENT_CONNECTION_NAME ": ERROR message received.");
					DumpReceived(message);
				} else {
					// DumpReceived(message);
					HandleMessage(message);
				}
				m_messagesBuffer.erase(m_messagesBuffer.begin(), ++end);
			}

			StartRead();

		}

		void StartRead() {
			if (!m_isActive) {
				return;
			}
			m_socket->async_read_some(
				io::buffer(&m_socketBuffer[0], m_socketBuffer.size()),
				boost::bind(
					&Connection::HandleRead,
					this,
					io::placeholders::error,
					io::placeholders::bytes_transferred));
		}

		void HandleResolve(
					const boost::system::error_code &error,
					ResolverIterator endpointIterator) {
			const Lock lock(m_mutex);
			Assert(!m_isActive);
			if (!error) {
				std::auto_ptr<Endpoint> endpoint(new Endpoint(*endpointIterator));
				m_socket->async_connect(
					*endpoint,
					boost::bind(
						&Connection::HandleConnect,
						this,
						io::placeholders::error,
						boost::ref(*endpoint),
						++endpointIterator));
				endpoint.release();
			} else {
				Log::Error(IQFEED_CLIENT_CONNECTION_NAME ": server resolve error.");
				m_condition.notify_all();
			}
		}

		void HandleConnect(
					const boost::system::error_code &error,
					Endpoint &endpoint,
					ResolverIterator endpointIterator) {
			std::auto_ptr<Endpoint> endpointHolder(&endpoint);
			const Lock lock(m_mutex);
			Assert(!m_isActive);
			if (!error) {
				Log::Info(IQFEED_CLIENT_CONNECTION_NAME ": CONNECTED.");
				m_isActive = true;
				m_condition.notify_all();
				StartRead();
			} else if (endpointIterator != ResolverIterator()) {
				m_socket->close();
				std::auto_ptr<Endpoint> endpoint(new Endpoint(*endpointIterator));
				m_socket->async_connect(
					*endpoint,
					boost::bind(
						&Connection::HandleConnect,
						this,
						io::placeholders::error,
						boost::ref(*endpoint),
						++endpointIterator));
				endpoint.release();
			} else {
				m_condition.notify_all();
			}
		}

	public:

		Mutex m_mutex;

	protected:
	
		Service &m_service;

	private:

		const std::string m_host;
		const unsigned short m_port;

		bool m_isActive;

		Condition m_condition;
		std::unique_ptr<boost::thread> m_thread;

		std::unique_ptr<io::io_service> m_ioService;
		std::unique_ptr<Socket> m_socket;
		SocketBuffer m_socketBuffer;

		MessagesBuffer m_messagesBuffer;

	};

	template<typename Service>
	class Level1Connection : public Connection<Service> {

	public:

		Level1Connection(Service &service)
				: Connection(service, 5009) {
			//...//
		}

		virtual ~Level1Connection() {
			//...//
		}

	protected:

		virtual void HandleMessage(MessageParser &message) {
			switch (*message.GetBegin()) {
				case 'Q':
					HandleUpdateMessage(message, false);
					break;
				case 'P':
					HandleUpdateMessage(message, true);
					break;
			}
		}

	private:

		void HandleUpdateMessage(const MessageParser &message, bool isSummary) {
			const pt::ptime now = boost::get_system_time();
			const std::string symbol = message.GetFieldAsString(2, true);
			const UpdatesSubscribers::const_iterator subscriber
				= m_service.m_marketDataLevel1Subscribers.find(symbol);
			if (subscriber == m_service.m_marketDataLevel1Subscribers.end()) {
				Log::Warn(
					IQFEED_CLIENT_CONNECTION_NAME " connection received Update Message for unknown symbol \"%1%\".",
					symbol);
				DumpReceived(message);
				return;
			} 
			
			//! @todo: to optimize!!!!
			const std::string error = message.GetFieldAsString(4, false);
			if (error == "Not Found") {
				Log::Error(
					IQFEED_CLIENT_CONNECTION_NAME " symbol \"%1%\" not found by IQLink.",
					symbol);
				DumpReceived(message);
				return;
			}

			const double last = message.GetFieldAsDouble(4, true);
			Assert(last > 0);

			const double ask = message.GetFieldAsDouble(11, true);
			const double bid = message.GetFieldAsDouble(12, true);
			if (isSummary && (Util::IsZero(ask) || Util::IsZero(bid))) {
				Assert(Util::IsZero(ask));
				Assert(Util::IsZero(bid));
				return;
			}
			Assert(ask > 0);
			Assert(bid > 0);
			Assert(ask * 0.75 < bid);
			Assert(ask * 1.25 > bid);

			subscriber->second->UpdateLevel1(now, last, ask, bid);

		}

	};

	template<typename Service>
	class Level2Connection : public Connection<Service> {

	public:

		Level2Connection(Service &service)
				: Connection(service, 9200),
				m_estTimeDiff(Util::GetEdtDiff()) {
			//...//
		}

		virtual ~Level2Connection() {
			//...//
		}

	protected:

		virtual void HandleMessage(MessageParser &message) {
			switch (*message.GetBegin()) {
				case 'U':
					HandleUpdateMessage(message);
					break;
				case 'n': // n,[SYMBOL]<CR><LF> Symbol not found message.
					HandleNoSymbolMessage(message);
					break;
				case 'E': // E,[error text]<CR><LF> An error message. 
					HandleErrorMessage(message);
					break;
			}
		}

	private:

		void HandleNoSymbolMessage(const MessageParser &message) {
			DumpReceived(message);
			Log::Error(
				IQFEED_CLIENT_CONNECTION_NAME ": Level 2 ERROR message received: Symbol \"%1%\" not found.",
				message.GetFieldAsString(2, true));
		}

		void HandleErrorMessage(const MessageParser &message) {
			DumpReceived(message);
			Log::Error(
				IQFEED_CLIENT_CONNECTION_NAME ": Level 2 ERROR message received: \"%1%\".",
				message.GetFieldAsString(2, true));
		}

		void HandleUpdateMessage(MessageParser &message) {

			const pt::ptime now = boost::get_system_time();
			
			const std::string symbol = message.GetFieldAsString(2, true);
			const UpdatesSubscribers::const_iterator subscriber
				= m_service.m_marketDataLevel2Subscribers.find(symbol);
			if (subscriber == m_service.m_marketDataLevel2Subscribers.end()) {
				Log::Warn(
					IQFEED_CLIENT_CONNECTION_NAME " Level II connection received Update Message for unknown symbol \"%1%\".",
					symbol);
				DumpReceived(message);
				return;
			}

			bool isBidValid = message.GetFieldAsBoolean(14, true);
			bool isAskValid = message.GetFieldAsBoolean(15, true);
			Assert(isBidValid || isAskValid);
			if (!isBidValid && !isAskValid) {
				return;
			}
			message.Reset();

			const double bid = isBidValid
				?	message.GetFieldAsDouble(4, true)
				:	.0;
			isBidValid = !Util::IsZero(bid);

			const double ask = isAskValid
				?	message.GetFieldAsDouble(5, true)
				:	.0;
			isAskValid = !Util::IsZero(ask);
		
			const size_t bidSize = isBidValid
				?	message.GetFieldAsUnsignedInt(6, true)
				:	0;
			isBidValid = bidSize > 0;

			const size_t askSize = isAskValid
				?	message.GetFieldAsUnsignedInt(7, true)
				:	0;
			isAskValid = askSize > 0;

// 			const pt::ptime bidTime = isBidValid
// 				?	(message.GetFieldAsTime(8, true) - m_estTimeDiff)
// 				:	pt::not_a_date_time;
// 
// 			const pt::ptime askTime = isAskValid
// 				?	(message.GetFieldAsTime(13, true) - m_estTimeDiff)
// 				:	pt::not_a_date_time;

			if (isBidValid) {
				Assert(!Util::IsZero(bid));
				Assert(bidSize > 0);
//				Assert(!bidTime.is_not_a_date_time());
				subscriber->second->UpdateBidLevel2(now, bid, bidSize);
			}
			if (isAskValid) {
				Assert(!Util::IsZero(ask));
				Assert(askSize > 0);
//				Assert(!askTime.is_not_a_date_time());
				subscriber->second->UpdateAskLevel2(now, ask, askSize);
			}

		}

	private:

		const pt::time_duration m_estTimeDiff;

	};

	//////////////////////////////////////////////////////////////////////////

	template<typename Service>
	class LookupConnection : public Connection<Service> {

	public:

		LookupConnection(Service &service)
				: Connection(service, 9100),
				m_estTimeDiff(Util::GetEdtDiff()) {
			//...//
		}

		virtual ~LookupConnection() {
			//...//
		}

	protected:

		virtual void HandleMessage(MessageParser &message) {
			switch (*message.GetBegin()) {
				case 'H':
					{
						const std::string magic = message.GetFieldAsString(1, true);
						if (!boost::starts_with(magic, "H:")) {
							break;
						}
						typedef boost::algorithm::split_iterator<std::string::const_iterator>
							SplitIterator;
						SplitIterator split = boost::make_split_iterator(
							magic,
							boost::first_finder(":", boost::is_equal()));
						if (split == SplitIterator() || ++split == SplitIterator()) {
							Log::Error(
								IQFEED_CLIENT_CONNECTION_NAME ": failed to parse History Message request ID \"%1%\".",
								magic);
							DumpReceived(message);
							break;
						}
						const std::string symbol = boost::copy_range<std::string>(*split);
						if (symbol.empty()) {
							Log::Error(
								IQFEED_CLIENT_CONNECTION_NAME ": failed to get symbol for History Message from request \"%1%\".",
								magic);
							DumpReceived(message);
							break;
						}
						HandleHistoryMessage(symbol, message);
					}
					break;
			}
		}

	private:

		void HandleHistoryMessage(const std::string &symbol, const MessageParser &message) {

			const HistorySubscribers::iterator subscriberIt
				= m_service.m_historySubscribers.find(symbol);
			if (subscriberIt == m_service.m_historySubscribers.end()) {
				Log::Warn(
					IQFEED_CLIENT_CONNECTION_NAME " connection received History Message for unknown symbol \"%1%\".",
					symbol);
				DumpReceived(message);
				return;
			}
			DynamicSecurity &subscriber = *subscriberIt->second;

			const std::string messageStr = message.GetFieldAsString(2, true);
			if (messageStr == "!ENDMSG!") {
				CompleteHistory(subscriberIt);
				return;
			} else if (messageStr == "E") {
				const std::string errorMessageStr = message.GetFieldAsString(3, false);
				if (errorMessageStr == "!NO_DATA!") {
					Log::Debug(
						IQFEED_CLIENT_CONNECTION_NAME ": no history data for \"%1%\".",
						subscriber.GetSymbol());
				} else {
					Log::Error(
						IQFEED_CLIENT_CONNECTION_NAME ": history data error for \"%1%\": \"%2%\".",
						subscriber.GetSymbol(),
						errorMessageStr);
				}
				subscriber.OnHistoryDataEnd();
				return;
			}

			const pt::ptime timeEdt = message.GetFieldAsTime(2, true);
			const pt::ptime time(timeEdt - m_estTimeDiff);

			const double last = message.GetFieldAsDouble(3, true);
			Assert(last > 0);

			const double ask = message.GetFieldAsDouble(6, true);
			Assert(ask > 0);
			const double bid = message.GetFieldAsDouble(7, true);
			Assert(bid > 0);

			Assert(ask * 0.75 < bid);
			Assert(ask * 1.25 > bid);

			subscriber.UpdateLevel1(time, last, ask, bid);

		}

		void CompleteHistory(const HistorySubscribers::const_iterator subscriberIt) {
			boost::shared_ptr<DynamicSecurity> subscriber = subscriberIt->second;
			Log::Debug(
				IQFEED_CLIENT_CONNECTION_NAME ": completed history data for \"%1%\".",
				subscriber->GetSymbol());
			m_service.m_historySubscribers.erase(subscriberIt);
			subscriber->OnHistoryDataEnd();
			{
				const auto i = m_service.m_marketDataLevel1Subscribers.find(subscriber->GetSymbol());
				if (i != m_service.m_marketDataLevel1Subscribers.end()) {
					m_service.SendSubscribeToMarketDataLevel1Request(*i->second);
				}
			}
			{
				const auto i = m_service.m_marketDataLevel2Subscribers.find(subscriber->GetSymbol());
				if (i != m_service.m_marketDataLevel1Subscribers.end()) {
					m_service.SendSubscribeToMarketDataLevel2Request(*i->second);
				}
			}
		}

	private:

		const pt::time_duration m_estTimeDiff;

	};

	//////////////////////////////////////////////////////////////////////////

}


namespace {

	std::ofstream * OpenFile(const fs::path &path) {
		fs::create_directories(path.branch_path());
		return new std::ofstream(path.string().c_str());
	}

}

class IqFeedClient::Implementation : private boost::noncopyable {

private:

	typedef Level1Connection<IqFeedClient::Implementation> Level1Connection;
	typedef Level2Connection<IqFeedClient::Implementation> Level2Connection;
	typedef LookupConnection<IqFeedClient::Implementation> LookupConnection;

public:

	Implementation()
			: m_log(OpenFile(Defaults::GetIqFeedLogFilePath())) {
		if (!*m_log) {
			Log::Error(
				IQFEED_CLIENT_CONNECTION_NAME ": failed to open log file %1%.",
				Defaults::GetIqFeedLogFilePath());
			throw IqFeedClient::Error(IQFEED_CLIENT_CONNECTION_NAME ": failed to open log file");
		} else {
			Log::Info("Logging IQFeed messages into %1%...", Defaults::GetIqFeedLogFilePath());
			WriteLogRecordHead(*m_log, 0);
			*m_log << "Started." << std::endl;
		}
		m_level1.reset(new Level1Connection(*this));
		m_level2.reset(new Level2Connection(*this));
		m_lookup.reset(new LookupConnection(*this));
	}

	~Implementation() {
		//...//
	}

public:

	void Connect() {
		m_level1->Connect();
		m_level2->Connect();
		m_lookup->Connect();
	}

	void SubscribeToMarketDataLevel1(boost::shared_ptr<DynamicSecurity> instrument) {
		SubscribeToMarketData(
			instrument,
			&Implementation::SendSubscribeToMarketDataLevel1Request,
			m_marketDataLevel1Subscribers);
	}

	void SubscribeToMarketDataLevel2(boost::shared_ptr<DynamicSecurity> instrument) {
		SubscribeToMarketData(
			instrument,
			&Implementation::SendSubscribeToMarketDataLevel2Request,
			m_marketDataLevel2Subscribers);
	}

	void RequestHistory(
				boost::shared_ptr<DynamicSecurity> instrument,
				const boost::posix_time::ptime &fromTime,
				const boost::posix_time::ptime &toTime) {

		Assert(fromTime != pt::not_a_date_time);
		Assert(toTime == pt::not_a_date_time || fromTime < toTime);

		const lt::local_date_time estFromTime(fromTime, Util::GetEdtTimeZone());
		Assert(estFromTime.utc_time() == fromTime);
		Assert(estFromTime.local_time() != fromTime);

		const LookupConnection::Lock lockLookup(m_lookup->m_mutex);
		const Level1Connection::Lock lockLevel1(m_level1->m_mutex);
		const Level1Connection::Lock lockLevel2(m_level2->m_mutex);

		CheckState();

		Assert(m_marketDataLevel1Subscribers.find(instrument->GetSymbol()) == m_marketDataLevel1Subscribers.end());
		if (m_marketDataLevel1Subscribers.find(instrument->GetSymbol()) != m_marketDataLevel1Subscribers.end()) {
			throw Error("Failed to subscribe symbol to history, already subscribed to market data");
		}

		HistorySubscribers historySubscribers(m_historySubscribers);
		Assert(historySubscribers.find(instrument->GetSymbol()) == historySubscribers.end());
		historySubscribers[instrument->GetSymbol()] = instrument;

		const size_t datapointsPerSend = m_lookup->GetSocketBufferSize() / 100;
		if (toTime != pt::not_a_date_time) {
			lt::local_date_time estToTime(toTime, Util::GetEdtTimeZone());
			Assert(estToTime.utc_time() == toTime);
			Assert(estToTime.local_time() != toTime);
			// HTT,[Symbol],[BeginDate BeginTime],[EndDate EndTime],[MaxDatapoints],[BeginFilterTime],[EndFilterTime],[DataDirection],[RequestID],[DatapointsPerSend]<CR><LF> 
			boost::format command("HTT,%1%,%2%%3%%4% %5%%6%%7%,%8%%9%%10% %11%%12%%13%,,,,1,H:%14%,%15%\r\n");
			command
				% instrument->GetSymbol()
				% estFromTime.local_time().date().year()
					% boost::io::group(std::setfill('0'), std::setw(2), int(estFromTime.local_time().date().month()))
					% boost::io::group(std::setfill('0'), std::setw(2), estFromTime.local_time().date().day())
				% boost::io::group(std::setfill('0'), std::setw(2), estFromTime.local_time().time_of_day().hours())
					% boost::io::group(std::setfill('0'), std::setw(2), estFromTime.local_time().time_of_day().minutes())
					% boost::io::group(std::setfill('0'), std::setw(2), estFromTime.local_time().time_of_day().seconds())
				% estToTime.local_time().date().year()
					% boost::io::group(std::setfill('0'), std::setw(2), int(estToTime.local_time().date().month()))
					% boost::io::group(std::setfill('0'), std::setw(2), estToTime.local_time().date().day())
				% boost::io::group(std::setfill('0'), std::setw(2), estToTime.local_time().time_of_day().hours())
					% boost::io::group(std::setfill('0'), std::setw(2), estToTime.local_time().time_of_day().minutes())
					% boost::io::group(std::setfill('0'), std::setw(2), estToTime.local_time().time_of_day().seconds())
				% instrument->GetSymbol()
				% datapointsPerSend;
			// DumpSend(command.str(), 999);
			m_lookup->Send(command.str());
			Log::Debug(
				"Sent " IQFEED_CLIENT_CONNECTION_NAME " history request:"
					" \"%1%\", period: %2% - %3% (EDT).",
				instrument->GetSymbol(),
				estFromTime.local_time(),
				estToTime.local_time());
		} else {
			Assert((boost::get_system_time().date() - toTime.date()).days() >= 0);
			// HTD,[Symbol],[Days],[MaxDatapoints],[BeginFilterTime],[EndFilterTime],[DataDirection],[RequestID],[DatapointsPerSend]<CR><LF> 
			boost::format command("HTD,%1%,%2%,,%3%%4%%5%,,1,H:%6%,%7%\r\n");
			command
				% instrument->GetSymbol()
				% ((boost::get_system_time().date() - fromTime.date()).days() + 1)
				% boost::io::group(std::setfill('0'), std::setw(2), estFromTime.local_time().time_of_day().hours())
					% boost::io::group(std::setfill('0'), std::setw(2), estFromTime.local_time().time_of_day().minutes())
					% boost::io::group(std::setfill('0'), std::setw(2), estFromTime.local_time().time_of_day().seconds())
				% instrument->GetSymbol()
				% datapointsPerSend;
			m_lookup->Send(command.str());
			Log::Debug(
				"Sent " IQFEED_CLIENT_CONNECTION_NAME " history request:"
					" for \"%1%\", from %2% (EDT).",
				instrument->GetSymbol(),
				estFromTime.local_time());
		}

		instrument->OnHistoryDataStart();
		historySubscribers.swap(m_historySubscribers);

	}

public:

	void SubscribeToMarketData(
				boost::shared_ptr<DynamicSecurity> instrument,
				void (Implementation::*sendSubscribeRequest)(const DynamicSecurity &),
				UpdatesSubscribers &list) {
		const LookupConnection::Lock lockLookup(m_lookup->m_mutex);
		const Level1Connection::Lock lockLevel1(m_level1->m_mutex);
		const Level2Connection::Lock lockLevel2(m_level2->m_mutex);
		if (list.find(instrument->GetSymbol()) != list.end()) {
			return;
		}
		CheckState();
		auto subscribers(list);
		subscribers[instrument->GetSymbol()] = instrument;
		if (m_historySubscribers.find(instrument->GetSymbol()) == m_historySubscribers.end()) {
			(this->*sendSubscribeRequest)(*instrument);
		}
		subscribers.swap(list);
	}

	void SendSubscribeToMarketDataLevel1Request(const DynamicSecurity &subscriber) {
		m_level1->Send((boost::format("w%1%\r\n") % subscriber.GetSymbol()).str());
		Log::Debug(
			"Sent " IQFEED_CLIENT_CONNECTION_NAME " Level I market data subscription request for \"%1%\".",
			subscriber.GetSymbol());
	}

	void SendSubscribeToMarketDataLevel2Request(const DynamicSecurity &subscriber) {
		m_level2->Send((boost::format("w%1%\r\n") % subscriber.GetSymbol()).str());
		Log::Debug(
			"Sent " IQFEED_CLIENT_CONNECTION_NAME " Level II market data subscription request for \"%1%\".",
			subscriber.GetSymbol());
	}

public:

	void DumpSend(const std::string &message, unsigned short port) const {
		const LogLock lock(m_logMutex);
		WriteLogRecordHead(*m_log, port);
		*m_log << "< ";
		std::for_each(message.begin(), message.end(), [&](char ch) {DumpCh(ch);});
		*m_log << std::endl;
	}

	void DumpReceived(const MessageParser &message, unsigned short port) const {
		const LogLock lock(m_logMutex);
		WriteLogRecordHead(*m_log, port);
		*m_log << "> ";
		std::for_each(
			message.GetBegin(),
			message.GetEnd(),
			[this](char ch) {DumpCh(ch);});
		*m_log << std::endl;
	}

private:

	void CheckState() {
		m_level1->CheckState();
		m_level2->CheckState();
		m_lookup->CheckState();
	}

	void DumpCh(char ch) const {
		switch (ch) {
			case '\n':
				*m_log << "<LF>";
				break;
			case '\r':
				*m_log << "<CF>";
				break;
			default:
				m_log->put(ch);
		}
	}

public:

	UpdatesSubscribers m_marketDataLevel1Subscribers;
	UpdatesSubscribers m_marketDataLevel2Subscribers;
	HistorySubscribers m_historySubscribers;

private:

	mutable std::unique_ptr<std::ofstream> m_log;
	mutable LogMutex m_logMutex;

	std::unique_ptr<Level1Connection> m_level1;
	std::unique_ptr<Level2Connection> m_level2;
	std::unique_ptr<LookupConnection> m_lookup;

};

//////////////////////////////////////////////////////////////////////////

IqFeedClient::IqFeedClient()
		: m_pimpl(new Implementation) {
	//...//
}

IqFeedClient::~IqFeedClient() {
	delete m_pimpl;
}

void IqFeedClient::Connect() {
	m_pimpl->Connect();
}

void IqFeedClient::SubscribeToMarketDataLevel1(
			boost::shared_ptr<DynamicSecurity> instrument) 
		const {
	m_pimpl->SubscribeToMarketDataLevel1(instrument);
}

void IqFeedClient::SubscribeToMarketDataLevel2(
			boost::shared_ptr<DynamicSecurity> instrument) 
		const {
	m_pimpl->SubscribeToMarketDataLevel2(instrument);
}

void IqFeedClient::RequestHistory(
			boost::shared_ptr<DynamicSecurity> instrument,
			const boost::posix_time::ptime &fromTime)
		const {
	m_pimpl->RequestHistory(instrument, fromTime, pt::not_a_date_time);
}

void IqFeedClient::RequestHistory(
			boost::shared_ptr<DynamicSecurity> instrument,
			const boost::posix_time::ptime &fromTime,
			const boost::posix_time::ptime &toTime)
		const {
	m_pimpl->RequestHistory(instrument, fromTime, toTime);
}
