/**************************************************************************
 *   Created: 2012/08/28 01:33:05
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "GatewayTsMessage.hpp"
#include "GatewayClientMessage.hpp"
#include "Core/TradeSystem.hpp"

namespace Trader {  namespace Interaction { namespace Lightspeed {

	class Gateway : public Trader::TradeSystem {

	public:

		enum Stage {
			STAGE_CLOSED_BY_LOGIN_REJECTED	= 101,
			STAGE_CLOSED_BY_WRITE_ERROR	= 201,
			STAGE_CLOSED_BY_READ_ERROR	= 202,
			STAGE_CLOSED_GRACEFULLY		= 301,
			STAGE_CONNECTING	= 1101,
			STAGE_CONNECTED		= 1102,
			STAGE_HANDSHAKED	= 1203,
			STAGE_LOGGED_ON		= 1204
		};

		typedef boost::circular_buffer<char> MessagesBuffer;
		typedef GatewayTsMessage<MessagesBuffer> TsMessage;

		typedef GatewayClientMessage<boost::asio::streambuf> ClientMessage;

		class Error : public TradeSystem::Error {
		public:
			explicit Error(const char *what) throw()
					: TradeSystem::Error(what) {
				//...//
			}
		};

		class MessageError : public Error {
		public:
			explicit MessageError(const char *what, const std::string &message)
						throw()
					: Error(what),
					m_messageBuffer(message) {
				//...//
			}
			explicit MessageError(const char *what, const TsMessage &message)
						throw()
					: Error(what),
					m_messageBuffer(message.GetAsString(false)) {
				//...//
			}
		public:
			const std::string & GetMessage() const {
				return m_messageBuffer;
			}
		private:
			std::string m_messageBuffer;
		};

		class ProtocolError : public MessageError {
		public:
			explicit ProtocolError(
						const char *what,
						TsMessage::Iterator messageBegin,
						TsMessage::Iterator messageEnd,
						Stage stage)
					throw()
					: MessageError(what, std::string(messageBegin, messageEnd)),
					m_stage(stage) {
				//...//
			}
			explicit ProtocolError(
							const char *what,
							const std::string &message,
							Stage stage)
						throw()
					: MessageError(what, message),
					m_stage(stage) {
				//...//
			}
		public:
			Stage GetStage() const {
				return m_stage;
			}
		private:
			Stage m_stage;
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

		typedef std::vector<char> SocketReceiveBuffer;
		typedef boost::asio::streambuf SocketSendBuffer;

		typedef int64_t Seqnumber;
		typedef long Token;

		//////////////////////////////////////////////////////////////////////////

		struct Order {

			struct Modifers {

				struct Execution {
					OrderQty qty;
					double price;
					Execution(OrderQty qty, double price)
							: qty(qty),
							price(price) {
						//...//
					}
					void operator ()(Order &order) {
						AssertLe(order.executedQty + qty, order.initialQty);
						order.avgExecutionPrice(price);
						order.executedQty += qty;
					}
				};

			};

			Token token;

			OrderStatusUpdateSlot callback;

			OrderQty initialQty;
			OrderQty executedQty;

			boost::accumulators::accumulator_set<
					double,
					boost::accumulators::stats<boost::accumulators::tag::mean>>
				avgExecutionPrice;

			Order() {
				//...//
			}

			explicit Order(
						const Token &token,
						const OrderStatusUpdateSlot &callback,
						const OrderQty &qty)
					: token(token),
					callback(callback),
					initialQty(qty),
					executedQty(0) {
				//...//
			}

		};

		struct ByToken {
			//...//
		};

		typedef boost::multi_index_container<
			Order,
			boost::multi_index::indexed_by<
				boost::multi_index::hashed_unique<
					boost::multi_index::tag<ByToken>,
					boost::multi_index::member<Order, Token, &Order::token>>>>
			Orders;

		typedef Orders::index<ByToken>::type OrdersByToken;

		//////////////////////////////////////////////////////////////////////////

		struct Connection {

			IoService ioService;
			Socket socket;
			boost::thread_group tasks;

			SocketReceiveBuffer socketBuffer;
			MessagesBuffer messagesBuffer;

			Stage stage;

			Seqnumber seqnumber;
			const boost::posix_time::time_duration timeout;

			volatile Token lastToken;
			Orders orders;
			size_t orderMessagesFromLastStatCount;

			explicit Connection(
					size_t socketBufferSize,
					size_t messagesBufferSize,
					const boost::posix_time::time_duration &timeout);
			~Connection();

		};

	public:

		Gateway();
		virtual ~Gateway();

	public:

		virtual void Connect(const IniFile &iniFile, const std::string &);

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

		void StartReading();
		void StartReading(Connection &);
		void StartInitialDataReading(Connection &);

		bool IsReadingClosed(
					Connection &connection,
					const boost::system::error_code &error,
					size_t size)
				const;

		void HandleResolve(
					Connection &,
					const boost::system::error_code &,
					ResolverIterator);
		void HandleConnect(
					Connection &,
					const boost::system::error_code &,
					Endpoint &,
					ResolverIterator);
		void HandleInitialDataRead(
					Connection &,
					const boost::system::error_code &,
					size_t size);

		void HandleRead(const boost::system::error_code &, size_t size);
		bool HandleRead(
				Connection &,
				const boost::system::error_code &,
				size_t size,
				const Lock &);

		void HandleMessage(const TsMessage &, Connection &);
		void HandleDebug(const TsMessage &, Connection &);

		void HandleLoginAccepted(const TsMessage &, Connection &);
		void HandleLoginRejected(const TsMessage &, Connection &);

		void HandleOrderAccepted(const TsMessage &, Connection &);
		void HandleOrderReject(const TsMessage &, Connection &);
		void HandleOrderCanceled(const TsMessage &, Connection &);
		void HandleOrderExecuted(const TsMessage &, Connection &);
		void StatOrders(Connection &) throw();

	private:

		void SendLoginRequest(
					Connection &,
					const std::string &login,
					const std::string &password);

		OrderId SendOrder(
				const Security &,
				ClientMessage::BuySellIndicator,
				OrderQty,
				OrderPrice price,
				ClientMessage::Numeric timeInForce,
				const OrderStatusUpdateSlot &);

		void SendHeartbeat(Connection &);

		void Send(boost::shared_ptr<ClientMessage>, Connection &);

		void HandleWrite(
				boost::shared_ptr<const ClientMessage>,
				const boost::system::error_code &,
				size_t size);

	private:

		Mutex m_mutex;
		Condition m_condition;
		std::unique_ptr<Connection> m_connection;


	};

} } }
