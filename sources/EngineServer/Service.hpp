/**************************************************************************
 *   Created: 2015/03/17 01:13:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Engine.hpp"
#include "QueueService.hpp"
#include "DropCopyRecord.hpp"
#include "Exception.hpp"

namespace trdk { namespace EngineServer {

	class Service : private boost::noncopyable {

	private:

		class Exception : public EngineServer::Exception {
		public:
			explicit Exception(const char *what) throw()
				: EngineServer::Exception(what) {
				//...//
			}
		};
		class ConnectError : public Exception {
		public:
			explicit ConnectError(const char *what) throw()
				: Exception(what) {
				//...//
			}
		};
		class TimeoutException : public Exception {
		public:
			explicit TimeoutException(const char *what) throw()
				: Exception(what) {
				//...//
			}
		};

		struct Config {

			boost::filesystem::path path;
			std::string name;
			std::string id;
			std::string twsHost;
			uint16_t twsPort;
			bool wampDebug;
			boost::posix_time::time_duration timeout;
			autobahn::wamp_call_options callOptions;

			explicit Config(const boost::filesystem::path &);

		};

		struct Topics {

			std::string registerEngine;
			std::string onEngineConnected;

			std::string time;
			std::string state;

			std::string storeOperationStart;
			std::string storeOperationEnd;
			std::string storeOrder;
			std::string storeTrade;
			std::string storeBook;
			std::string storeBar;

			std::string registerAbstractDataSource;
			std::string storeAbstractDataPoint;

			std::string startEngine;
			std::string stopEngine;

			std::string startDropCopy;
			std::string stopDropCopy;

			std::string closePositions;

			explicit Topics(const std::string &suffix);

		};

		typedef boost::recursive_mutex EngineMutex;
		typedef boost::unique_lock<EngineMutex> EngineLock;

		typedef boost::mutex ConnectionMutex;
		typedef boost::unique_lock<ConnectionMutex> ConnectionLock;
		typedef boost::condition_variable ConnectionCondition;

		class Connection
			: private boost::noncopyable,
			public boost::enable_shared_from_this<Connection> {

		public:

			typedef autobahn::wamp_session<
					boost::asio::ip::tcp::socket,
					boost::asio::ip::tcp::socket>
				Session;

			EventsLog &log;
			const Topics &topics;
			boost::asio::ip::tcp::socket socket;
			boost::asio::deadline_timer currentTimePublishTimer;
			boost::asio::deadline_timer ioTimeoutTimer;
			std::shared_ptr<Session> session;

			boost::shared_future<void> sessionStartFuture;
			boost::shared_future<void> sessionJoinFuture;
			boost::shared_future<void> engineRegistrationFuture;

			explicit Connection(
					boost::asio::io_service &io,
					const Topics &topics,
					EventsLog &log,
					bool isWampDebugOn);

			void ScheduleNextCurrentTimeNotification();
			void ScheduleIoTimeout(const boost::posix_time::time_duration &);
			void StopIoTimeout();

		};

		struct OrderCache {

			boost::uuids::uuid id;
			boost::optional<std::string> tradingSystemId;
			boost::optional<boost::posix_time::ptime> orderTime;
			boost::optional<boost::posix_time::ptime> executionTime;
			OrderStatus status;
			boost::uuids::uuid operationId;
			boost::optional<int64_t> subOperationId;
			const Security *security;
			const TradingSystem *tradingSystem;
			OrderSide side;
			Qty qty;
			boost::optional<double> price;
			boost::optional<TimeInForce> timeInForce;
			Lib::Currency currency;
			boost::optional<Qty> minQty;
			Qty executedQty;
			boost::optional<double> bestBidPrice;
			boost::optional<Qty> bestBidQty;
			boost::optional<double> bestAskPrice;
			boost::optional<Qty> bestAskQty;

			explicit OrderCache(
					const boost::uuids::uuid &id,
					const std::string *tradingSystemId,
					const boost::posix_time::ptime *orderTime,
					const boost::posix_time::ptime *executionTime,
					const OrderStatus &status,
					const boost::uuids::uuid &operationId,
					const int64_t *subOperationId,
					const Security &security,
					const TradingSystem &tradingSystem,
					const OrderSide &side,
					const Qty &qty,
					const double *price,
					const TimeInForce *timeInForce,
					const Lib::Currency &currency,
					const Qty *minQty,
					const Qty &executedQty,
					const double *bestBidPrice,
					const Qty *bestBidQty,
					const double *bestAskPrice,
					const Qty *bestAskQty);

		};

		struct BarCache {
			const Security *security;
			boost::posix_time::ptime time;
			int64_t size;
			ScaledPrice open;
			ScaledPrice close;
			ScaledPrice high;
			ScaledPrice low;
		};

		class DropCopy : public trdk::DropCopy {
		public:
			explicit DropCopy(Service &);
			virtual ~DropCopy();
		public:
			void OpenDataLog(const boost::filesystem::path &logsDir);
			void OnConnectionRestored();
			EventsLog & GetDataLog() {
				return m_dataLog;
			}
			void Start();
			void Stop(bool sync);
			bool IsStarted() const;
			size_t TakeRecordNumber();
		public:
			virtual void Flush();
			virtual void Dump();
		public:
			virtual void CopyOrder(
					const boost::uuids::uuid &id,
					const std::string *tradingSystemId,
					const boost::posix_time::ptime *orderTime,
					const boost::posix_time::ptime *executionTime,
					const trdk::OrderStatus &,
					const boost::uuids::uuid &operationId,
					const int64_t *subOperationId,
					const trdk::Security &,
					const trdk::TradingSystem &,
					const trdk::OrderSide &,
					const trdk::Qty &qty,
					const double *price,
					const trdk::TimeInForce *,
					const trdk::Lib::Currency &,
					const trdk::Qty *minQty,
					const trdk::Qty &executedQty,
					const double *bestBidPrice,
					const trdk::Qty *bestBidQty,
					const double *bestAskPrice,
					const trdk::Qty *bestAskQty);
			virtual void CopyTrade(
					const boost::posix_time::ptime &,
					const std::string &tradingSystemTradeId,
					const boost::uuids::uuid &orderId,
					double price,
					const trdk::Qty &qty,
					double bestBidPrice,
					const trdk::Qty &bestBidQty,
					double bestAskPrice,
					const trdk::Qty &bestAskQty);
			virtual void ReportOperationStart(
					const boost::uuids::uuid &id,
					const boost::posix_time::ptime &,
					const trdk::Strategy &);
			virtual void ReportOperationEnd(
					const boost::uuids::uuid &id,
					const boost::posix_time::ptime &,
					const trdk::OperationResult &,
					double pnl,
					const boost::shared_ptr<const trdk::FinancialResult> &);
			virtual void CopyBook(
					const trdk::Security &,
					const trdk::PriceBook &);
			virtual void CopyBar(
					const trdk::Security &,
					const boost::posix_time::ptime &,
					const boost::posix_time::time_duration &,
					const trdk::ScaledPrice &openTradePrice,
					const trdk::ScaledPrice &closeTradePrice,
					const trdk::ScaledPrice &highTradePrice,
					const trdk::ScaledPrice &lowTradePrice);
			virtual void CopyBar(
					const trdk::Security &,
					const boost::posix_time::ptime &,
					size_t numberOfTicksInBar,
					const trdk::ScaledPrice &openTradePrice,
					const trdk::ScaledPrice &closeTradePrice,
					const trdk::ScaledPrice &highTradePrice,
					const trdk::ScaledPrice &lowTradePrice);
			virtual trdk::DropCopy::AbstractDataSourceId RegisterAbstractDataSource(
					const boost::uuids::uuid &instance,
					const boost::uuids::uuid &type,
					const std::string &name);
			virtual void CopyAbstractDataPoint(
					const trdk::DropCopy::AbstractDataSourceId &,
					const boost::posix_time::ptime &,
					double value);
		private:
			Service &m_service;
			EventsLog &m_log;
			std::ofstream m_dataLogFile;
			EventsLog m_dataLog;
			QueueService m_queue;
			boost::atomic_bool m_isSecondaryDataDisabled;
		};

		//! @sa https://mbcm.robotdk.com:8443/display/API/Constants
		enum EngineState {
			ENGINE_STATE_STARTED = 1100,
			ENGINE_STATE_STARTING = 1200,
			ENGINE_STATE_STOPPED = 2100,
			ENGINE_STATE_STOPPING = 2200,
			numberOfEngineStates = 4
		};

		class Task : private boost::noncopyable {
		public:
			bool IsActive() const {
				return !m_future.is_ready() && m_future.valid();
			}
			template<typename TaskImpl>
			void Start(const TaskImpl &&taskImpl) {
				Assert(!IsActive());
				boost::packaged_task<void> task(std::move(taskImpl));
				auto future = task.get_future();
				boost::thread(std::move(task)).swap(m_thread);
				future.swap(m_future);
			}
			void Join() {
				m_thread.join();
			}
		private:
			boost::thread m_thread;
			boost::future<void> m_future;
		};

	public:

		explicit Service(
				const boost::filesystem::path &engineConfigFilePath,
				const boost::posix_time::time_duration &startDelay);
		~Service();

	private:

		void OnContextStateChanged(
				const trdk::Context::State &,
				const std::string *message = nullptr);

	private:

		bool StoreOperationStartReport(
				size_t recordNumber,
				size_t storeAttemptNo,
				bool dump,
				const boost::uuids::uuid &id,
				const boost::posix_time::ptime &,
				const trdk::Strategy &);
		bool StoreOperationEndReport(
				size_t recordNumber,
				size_t storeAttemptNo,
				bool dump,
				const boost::uuids::uuid &id,
				const boost::posix_time::ptime &,
				const trdk::OperationResult &,
				double pnl,
				const boost::shared_ptr<const FinancialResult> &);

		bool StoreOrder(
				size_t recordNumber,
				size_t storeAttemptNo,
				bool dump,
				const OrderCache &);

		bool StoreTrade(
				size_t recordNumber,
				size_t storeAttemptNo,
				bool dump,
				const boost::posix_time::ptime &,
				const std::string &tradingSystemTradeId,
				const boost::uuids::uuid &orderId,
				double price,
				const Qty &qty,
				double bestBidPrice,
				const Qty &bestBidQty,
				double bestAskPrice,
				const Qty &bestAskQty);

		bool StoreBook(
				size_t recordNumber,
				size_t storeAttemptNo,
				bool dump,
				const trdk::Security &,
				const trdk::PriceBook &);

		bool StoreBar(
				size_t recordNumber,
				size_t storeAttemptNo,
				bool dump,
				const BarCache &);

		bool StoreAbstractDataPoint(
				size_t recordNumber,
				size_t storeAttemptNo,
				bool dump,
				const trdk::DropCopy::AbstractDataSourceId &,
				const boost::posix_time::ptime &,
				double value);

		bool StoreRecord(
				const std::string Topics::*topic,
				size_t recordNumber,
				size_t storeAttemptNo,
				bool dump,
				const DropCopyRecord &&);
		void DumpRecord(
				const std::string Topics::*topic,
				size_t recordNumber,
				size_t storeAttemptNo,
				const DropCopyRecord &&);

		trdk::DropCopy::AbstractDataSourceId RegisterAbstractDataSource(
				const boost::uuids::uuid &instance,
				const boost::uuids::uuid &type,
				const std::string &name);

	private:

		void RunIoThread();

		boost::shared_ptr<Connection> Connect(
				const boost::posix_time::time_duration &startDelay
					= boost::posix_time::time_duration());
		void OnConnected(
				const boost::shared_ptr<Connection> &,
				const boost::system::error_code &,
				const boost::posix_time::time_duration &startDelay);
		void OnSessionStarted(
				boost::shared_ptr<Connection>,
				bool isStarted,
				const boost::posix_time::time_duration &startDelay);
		void OnSessionJoined(
				const boost::shared_ptr<Connection> &,
				const boost::optional<uint64_t> &sessionId,
				const boost::posix_time::time_duration &startDelay);
		void OnEngineRegistered(
				boost::shared_ptr<Connection>,
				uint64_t sessionId,
				const boost::optional<uint64_t> &instanceId,
				const std::tuple<std::string, std::string, std::string> &);

		void Reconnect();
		void RepeatReconnection(const Exception &prevReconnectError);

		void PublishState() const;

		void RegisterMethods(Connection &);

		void StartEngine();
		void StopEngine();

		void StartDropCopy();
		void StopDropCopy();

		void ClosePositions();

	private:

		const Config m_config;

		//! Log file.
		/** Should be first to be removed last.
		  */
		std::ofstream m_logFile;
		EventsLog m_log;

		DropCopy m_dropCopy;
		Task m_dropCopyTask;

		mutable EngineMutex m_engineMutex;
		boost::atomic<EngineState> m_engineState;
		boost::shared_ptr<Engine> m_engine;
		Task m_engineTask;

		const Topics m_topics;

		size_t m_numberOfReconnects;

		boost::thread m_thread;
		//! IO service.
		/** Should be last, to be removed first - all objects in service should
		  * be removed before other objects in "this".
		  */
		std::unique_ptr<boost::asio::io_service> m_io;

		mutable ConnectionMutex m_connectionMutex;
		ConnectionCondition m_connectionCondition;
		boost::shared_ptr<Connection> m_connection;
		//! Engine instance ID and "in initialized" flag.
		/** To change Instance ID engine should be restarted and updated.
		  * So it writes only one time at first connect, and if it will be
		  * changed at next reconnects - it will be treated as error.
		  */
		boost::optional<uint64_t> m_instanceId;

		boost::unordered_map<const void *, boost::posix_time::ptime>
			m_bookSentTime;

	};

} }
