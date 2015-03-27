/**************************************************************************
 *   Created: 2015/03/17 23:59:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace Interaction { namespace Itch {


	class Client
		: private boost::noncopyable,
		public boost::enable_shared_from_this<Client> {

	public:

		class Exception : public trdk::Lib::ModuleError {
		public:
			explicit Exception(const char *what) throw()
				: ModuleError(what) {
				//...//
			}
		};

	private:

		typedef std::vector<char> Buffer;
		typedef boost::mutex BufferMutex;
		typedef BufferMutex::scoped_lock BufferLock;


		class Message;
		class SessionMessage;
		class SequencedMessage;
		class ClientMessage;

		class PingPacket;

	private:

		explicit Client(
				const Context &,
				DataHandler &,
				boost::asio::io_service &,
				const std::string &host,
				size_t port);

	public:

		static boost::shared_ptr<Client> Create(
				const Context &,
				DataHandler &,
				boost::asio::io_service &,
				const std::string &host,
				size_t port,
				const std::string &login,
				const std::string &password);


	public:

		void SendMarketDataSubscribeRequest(const std::string &pair);

	private:

		void Login(const std::string &login, const std::string &password);
		void StartRead(
			Buffer &activeBuffer,
			size_t bufferStartOffset,
			Buffer &nextBuffer);

	private:

		void OnNewData(
				Buffer &activeBuffer,
				size_t bufferStartOffset,
				Buffer &nextBuffer,
				const boost::system::error_code &,
				size_t transferredBytes);
		void OnConnectionError(const boost::system::error_code &);

		void OnHeartbeat();
		void OnHeartbeatSent(const boost::system::error_code &);

		void OnError(const SessionMessage &);

		void OnSequncedDataPacket(
				const boost::posix_time::ptime &,
				const SequencedMessage &);
		void OnMarketSnapshot(
				const boost::posix_time::ptime &,
				const SequencedMessage &);
		void OnNewOrder(
				const boost::posix_time::ptime &,
				const SequencedMessage &);
		void OnOrderModify(
				const boost::posix_time::ptime &,
				const SequencedMessage &);
		void OnOrderCancel(
				const boost::posix_time::ptime &,
				const SequencedMessage &);

	private:

		size_t OnMarketSnapshotPriceLevel(
				const boost::posix_time::ptime &,
				const SequencedMessage &,
				size_t offset,
				const char *pair,
				bool isBuy);

	private:

		const Context &m_context;
		DataHandler &m_dataHandler;

		boost::asio::io_service &m_ioService;
		boost::asio::ip::tcp::socket m_socket;

		std::pair<Buffer, Buffer> m_buffer;
		BufferMutex m_bufferMutex;

#		ifdef TRDK_INTERACTION_ITCH_CLIENT_PERF_SOURCE
			std::ofstream m_perfSourceFile;
#		endif

		static const PingPacket m_pingPacket;

	};


} } }
