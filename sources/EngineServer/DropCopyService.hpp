/**************************************************************************
 *   Created: 2015/07/12 19:35:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/DropCopy.hpp"
#include "Core/EventsLog.hpp"
#include "Fwd.hpp"

namespace trdk { namespace EngineServer {

	template<Lib::Concurrency::Profile profile>
	struct DropCopyConcurrencyPolicyT {
		static_assert(
			profile == Lib::Concurrency::PROFILE_RELAX,
			"Wrong concurrency profile");
		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;
		typedef boost::condition_variable LazyCondition;
	};
	template<>
	struct DropCopyConcurrencyPolicyT<Lib::Concurrency::PROFILE_HFT> {
		typedef Lib::Concurrency::SpinMutex Mutex;
		typedef Mutex::ScopedLock Lock;
		typedef Lib::Concurrency::SpinCondition LazyCondition;
	};
	typedef DropCopyConcurrencyPolicyT<TRDK_CONCURRENCY_PROFILE>
		DropCopyConcurrencyPolicy;

	class DropCopyService : public trdk::DropCopy {

	public:

		typedef DropCopy Base;

	private:

		typedef boost::mutex ClientMutex;
		typedef ClientMutex::scoped_lock ClientLock;
		typedef boost::condition_variable ClientCondition;

		struct Io {
			boost::thread_group threads;
			boost::asio::io_service service;
			boost::shared_ptr<DropCopyClient> client;
		};

		struct OperationStart {
			boost::uuids::uuid id;
			boost::posix_time::ptime time;
			const trdk::Strategy *strategy;
			size_t updatesNumber;
		};

		struct OperationEnd {
			boost::uuids::uuid id;
			boost::posix_time::ptime time;
			double pnl;
			boost::shared_ptr<const trdk::FinancialResult> financialResult;
		};

		struct Order {
			boost::uuids::uuid id;
			boost::optional<std::string> tradeSystemId;
			boost::optional<boost::posix_time::ptime> orderTime;
			boost::optional<boost::posix_time::ptime> executionTime;
			trdk::TradeSystem::OrderStatus status;
			boost::uuids::uuid operationId;
			boost::optional<int64_t> subOperationId;
			const trdk::Security *security;
			trdk::OrderSide side;
			trdk::Qty qty;
			boost::optional<double> price;
			boost::optional<trdk::TimeInForce> timeInForce;
			trdk::Lib::Currency currency;
			boost::optional<trdk::Qty> minQty;
			boost::optional<std::string> user;
			trdk::Qty executedQty;
			boost::optional<double> bestBidPrice;
			boost::optional<trdk::Qty> bestBidQty;
			boost::optional<double> bestAskPrice;
			boost::optional<trdk::Qty> bestAskQty;
		};

		struct Trade {
			boost::posix_time::ptime time;
			std::string tradeSystemTradeid;
			boost::uuids::uuid orderId;
			double price;
			Qty qty;
			double bestBidPrice;
			Qty bestBidQty;
			double bestAskPrice;
			Qty bestAskQty;
		};

		class SendList : private boost::noncopyable {

		private:

			enum MessageType {
				MESSAGE_TYPE_OPERATION_START,
				MESSAGE_TYPE_OPERATION_END,
				MESSAGE_TYPE_ORDER,
				MESSAGE_TYPE_TRADE,
				numberOfMessageTypes
			};

			typedef DropCopyConcurrencyPolicy ConcurrencyPolicy;
			typedef ConcurrencyPolicy::Mutex Mutex;
			typedef ConcurrencyPolicy::Lock Lock;
			typedef ConcurrencyPolicy::LazyCondition Condition;

			struct Queue {

				typedef std::pair<MessageType, boost::any> Message;
				typedef std::vector<Message> Data;
			
				Data data;
				boost::atomic_size_t size;

				Queue()
					: size(0) {
					//...//
				}

				void Reset() {
					data.clear();
					size = 0;
				}
			
			};

		public:

			explicit SendList(DropCopyService &);
			~SendList();

		public:

			void Enqueue(OperationStart &&);
			void Enqueue(OperationEnd &&);
			void Enqueue(Order &&);
			void Enqueue(Trade &&);

			void Start();
			void Stop();
			
			void Flush();

			void OnConnectionRestored();

			size_t GetQueueSize() const;

			void LogQueue();

		private:

			void Enqueue(MessageType &&, boost::any &&);

			void SendTask();

		private:

/*
			trdk::EngineService::DropCopy::ServiceData CreateMessage(
					const Queue::Message &)
					const;
			trdk::EngineService::DropCopy::ServiceData CreateMessage(
					const OperationStart &)
					const;
			trdk::EngineService::DropCopy::ServiceData CreateMessage(
					const OperationEnd &)
					const;
			trdk::EngineService::DropCopy::ServiceData CreateMessage(
					const Order &)
					const;
			trdk::EngineService::DropCopy::ServiceData CreateMessage(
					const Trade &)
					const;
*/

		public:

			DropCopyService &m_service;
			
			Mutex m_mutex;
			Condition m_dataCondition;

			Condition m_connectionCondition;

			bool m_flushFlag;
			
			std::pair<Queue, Queue> m_queues;
			Queue *m_currentQueue;

			bool m_isStopped;

			boost::thread m_thread;

		};

		

	public:

		explicit DropCopyService(trdk::Context &, const Lib::IniSectionRef &);
		virtual ~DropCopyService();

	public:

		virtual void Start(const Lib::IniSectionRef &);

	public:

		virtual void CopyOrder(
				const boost::uuids::uuid &id,
				const std::string *tradeSystemId,
				const boost::posix_time::ptime *orderTime,
				const boost::posix_time::ptime *executionTime,
				const trdk::TradeSystem::OrderStatus &,
				const boost::uuids::uuid &operationId,
				const int64_t *subOperationId,
				const trdk::Security &,
				const trdk::OrderSide &,
				const trdk::Qty &qty,
				const double *price,
				const trdk::TimeInForce *,
				const trdk::Lib::Currency &,
				const trdk::Qty *minQty,
				const std::string *user,
				const trdk::Qty &executedQty,
				const double *bestBidPrice,
				const trdk::Qty *bestBidQty,
				const double *bestAskPrice,
				const trdk::Qty *bestAskQty);

		virtual void CopyTrade(
				const boost::posix_time::ptime &,
				const std::string &tradeSystemTradeid,
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
				const trdk::Strategy &,
				size_t updatesNumber);
		virtual void ReportOperationEnd(
				const boost::uuids::uuid &id,
				const boost::posix_time::ptime &,
				double pnl,
				const boost::shared_ptr<const trdk::FinancialResult> &);

	public:

		boost::asio::io_service & GetIoService() {
			return m_io->service;
		}

	public:

		void OnClientClose();

	private:

		void StartNewClient(const std::string &host, const std::string &port);

		void ReconnectClient(
				size_t attemptIndex,
				const std::string &host,
				const std::string &port);

//		void EnqueueToSend(const trdk::EngineService::DropCopy::ServiceData &);

	private:

//		bool SendSync(const trdk::EngineService::DropCopy::ServiceData &);

	private:

		SendList m_sendList;
		
		ClientMutex m_clientMutex;
		ClientCondition m_clientCondition;
		std::unique_ptr<Io> m_io;

//		trdk::EngineService::DropCopy::ServiceData m_dictonary;

	};

} }
