/**************************************************************************
 *   Created: 2015/03/17 23:22:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Core/MarketDataSource.hpp"
#include "DataHandler.hpp"

namespace trdk { namespace Interaction { namespace Itch {

	class Stream
		: public trdk::MarketDataSource,
		public DataHandler {

	private:

		typedef boost::mutex ClientMutex;
		typedef ClientMutex::scoped_lock ClientLock;

	public:

		explicit Stream(
				size_t index,
				Context &,
				const std::string &tag,
				const Lib::IniSectionRef &);
		virtual ~Stream();

	public:

		virtual void Connect(const Lib::IniSectionRef &);

		virtual void SubscribeToSecurities();

	public:

		virtual void OnNewOrder(
				const boost::posix_time::ptime &,
				bool isBuy,
				const char *pair,
				const Itch::OrderId &,
				double price,
				double amount);
		virtual void OnOrderModify(
				const boost::posix_time::ptime &time,
				const char *pair,
				const Itch::OrderId &,
				double amount);
		virtual void OnOrderCancel(
				const boost::posix_time::ptime &time,
				const char *pair,
				const Itch::OrderId &);

		virtual void Flush(
				const boost::posix_time::ptime &,
				const Lib::TimeMeasurement::Milestones &);

		virtual void OnDebug(const std::string &);
		virtual void OnErrorFromServer(const std::string &);
		virtual void OnConnectionClosed(
				const std::string &reason,
				bool isError);

	protected:

		virtual trdk::Security & CreateNewSecurityObject(const Lib::Symbol &);

	private:

		Itch::Security & GetSecurity(const char *symbol);

		void ScheduleReconnect();

		void ConnectClient();
		void ReconnectClient(const boost::system::error_code &);

	private:

		std::string m_host;
		size_t m_port;
		std::string m_login;
		std::string m_password;

		boost::asio::io_service m_ioService;
		boost::thread_group m_serviceThreads;
		std::unique_ptr<boost::asio::deadline_timer> m_reconnectTimer;

		std::vector<boost::shared_ptr<Itch::Security>> m_securities;

		ClientMutex m_clientMutex;
		boost::shared_ptr<Client> m_client;

		bool m_hasNewData;

		boost::posix_time::ptime m_lastConnectionAttempTime;

	};

} } }
