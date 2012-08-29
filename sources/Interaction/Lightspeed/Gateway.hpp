/**************************************************************************
 *   Created: 2012/08/28 01:33:05
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "GatewayTsMessage.hpp"
#include "Core/TradeSystem.hpp"

namespace Trader {  namespace Interaction { namespace Lightspeed {

	class Gateway : public Trader::TradeSystem {

	public:

		typedef uint8_t Stage; 

		typedef boost::circular_buffer<char> MessagesBuffer;
		typedef GatewayTsMessage<MessagesBuffer> TsMessage;

		class Error : public TradeSystem::Error {
		public:
			explicit Error(const char *what) throw()
					: TradeSystem::Error(what)
					{
				//...//
			}
		};

		class ProtocolError : public Error {
		public:
			typedef std::vector<char> MessageBuffer;
		public:
			ProtocolError(
						const char *what,
						TsMessage::Iterator messageBegin,
						TsMessage::Iterator messageEnd,
						Stage stage)
					throw()
					: Error(what),
					m_stage(stage),
					m_messageBuffer(messageBegin, messageEnd) {
				//...//
			}
		public:
			Stage GetStage() const {
				return m_stage;
			}
			const MessageBuffer & GetMessage() const {
				return m_messageBuffer;
			}
		private:
			Stage m_stage;
			MessageBuffer m_messageBuffer;
		};

	private:

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;

		typedef boost::condition_variable Condition;

		typedef boost::asio::io_service IoService;
		typedef boost::asio::ip::tcp Proto;
		typedef Proto::resolver Resolver;
		typedef Proto::resolver::iterator ResolverIterator;
		typedef Proto::endpoint Endpoint;
		typedef Proto::socket Socket;

		typedef std::vector<char> SocketBuffer;

		typedef uint64_t Seqnumber;
		
		struct Connection {
		
			IoService ioService;
			Socket socket;
			boost::thread task;
			
			SocketBuffer socketBuffer;
			MessagesBuffer messagesBuffer;

			Stage stage;

			Seqnumber seqnumber;

			explicit Connection(size_t bufferSize);
			~Connection();

		};

	public:

		Gateway();
		virtual ~Gateway();

	public:

		virtual void Connect(const Settings &);

		virtual bool IsCompleted(const Security &) const;

	public:

		virtual OrderId SellAtMarketPrice(
				const Security &,
				OrderQty,
				const OrderStatusUpdateSlot &);
		virtual OrderId Sell(
				const Security &,
				OrderQty,
				OrderPrice,
				const OrderStatusUpdateSlot &);
		virtual OrderId SellAtMarketPriceWithStopPrice(
				const Security &,
				OrderQty,
				OrderPrice stopPrice,
				const OrderStatusUpdateSlot &);
		virtual OrderId SellOrCancel(
				const Security &,
				OrderQty,
				OrderPrice,
				const OrderStatusUpdateSlot &);

		virtual OrderId BuyAtMarketPrice(
				const Security &,
				OrderQty,
				const OrderStatusUpdateSlot &);
		virtual OrderId Buy(
				const Security &,
				OrderQty,
				OrderPrice,
				const OrderStatusUpdateSlot &);
		virtual OrderId BuyAtMarketPriceWithStopPrice(
				const Security &,
				OrderQty,
				OrderPrice stopPrice,
				const OrderStatusUpdateSlot &);
		virtual OrderId BuyOrCancel(
				const Security &,
				OrderQty,
				OrderPrice,
				const OrderStatusUpdateSlot &);

		virtual void CancelOrder(OrderId);
		virtual void CancelAllOrders(const Security &);

	private:

		void HandleResolve(
					Connection &,
					const boost::system::error_code &,
					ResolverIterator);
		void HandleConnect(
					Connection &,
					const boost::system::error_code &,
					Endpoint &,
					ResolverIterator);

		void HandleRead(const boost::system::error_code &, size_t size);
		void HandleRead(
				Connection &,
				const boost::system::error_code &,
				size_t size);
		void HandleRead(
				Connection &,
				const boost::system::error_code &,
				size_t size,
				const Lock &);
		
		void HandleMessage(const TsMessage &, Connection &);
		void HandleDebugMessage(const TsMessage &, Connection &);
		void HandleLoginAccepted(const TsMessage &, Connection &);
		void HandleLoginRejected(const TsMessage &, Connection &);

	private:

		void SendLoginRequest(
					Connection &,
					const std::string &login,
					const std::string &password)
				const;

	private:

		Mutex m_mutex;
		Condition m_condition;
		std::unique_ptr<Connection> m_connection;
		

	};

} } }
