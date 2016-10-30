/*******************************************************************************
 *   Created: 2015/12/11 02:16:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

namespace trdk { namespace EngineServer {

	namespace Details {
		template<Lib::Concurrency::Profile profile>
		struct QueueServiceConcurrencyPolicyT {
			static_assert(
				profile == Lib::Concurrency::PROFILE_RELAX,
				"Wrong concurrency profile");
			typedef boost::mutex Mutex;
			typedef Mutex::scoped_lock Lock;
			typedef boost::condition_variable LazyCondition;
		};
		template<>
		struct QueueServiceConcurrencyPolicyT<Lib::Concurrency::PROFILE_HFT> {
			typedef Lib::Concurrency::SpinMutex Mutex;
			typedef Mutex::ScopedLock Lock;
			typedef Lib::Concurrency::SpinCondition LazyCondition;
		};
		typedef QueueServiceConcurrencyPolicyT<TRDK_CONCURRENCY_PROFILE>
			QueueServiceConcurrencyPolicy;
	}

	class QueueService : private boost::noncopyable {

	private:

		//! Returns false it process should be paused and wait for next
		//! signal.
		typedef boost::function<
				bool (size_t recordId, size_t attemptNo, bool dump)>
			Callback;

		class Item {

		public:

			explicit Item(size_t index, const Callback &&);

		public:

			bool Dequeue();
			void Dump();

		private:

			size_t m_index;
			size_t m_attemptNo;

#			ifdef BOOST_ENABLE_ASSERT_HANDLER
				mutable bool m_isDequeued;
#			endif

			Callback m_callback;

		};

		typedef Details::QueueServiceConcurrencyPolicy ConcurrencyPolicy;
		typedef ConcurrencyPolicy::Mutex StateMutex;
		typedef ConcurrencyPolicy::Lock StateLock;
		typedef ConcurrencyPolicy::LazyCondition DataCondition;

		typedef boost::mutex DequeueMutex;
		typedef DequeueMutex::scoped_lock DequeueLock;

		struct Queue {

			typedef std::vector<Item> Data;
			
			Data data;
			boost::atomic_size_t logicalSize;

			Queue();

			void Reset();
			Data::iterator GetLogicalBegin();
			
		};

	public:

		explicit QueueService(EventsLog &);
		~QueueService();

	public:

		void Start();
		void Stop(bool sync);
		bool IsStarted() const;

		void Enqueue(Callback &&);

		void Flush();
		void Dump();

		void OnConnectionRestored();

		size_t GetSize() const;
		bool IsEmpty() const;

		size_t TakeRecordNumber();

	private:

		void RunDequeue();

	private:

		EventsLog &m_log;
			
		mutable StateMutex m_stateMutex;
		DataCondition m_dataCondition;

		mutable DequeueMutex m_dequeueMutex;

		bool m_flushFlag;

		size_t m_nextRecordNumber;
		std::pair<Queue, Queue> m_queues;
		Queue *m_currentQueue;

		bool m_isStopped;

		boost::thread m_thread;

	};

 } }
