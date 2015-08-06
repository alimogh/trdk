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
#include "EngineService/DropCopy.h"

namespace trdk { namespace EngineServer {

	class DropCopyService : public trdk::DropCopy {

	public:

		typedef DropCopy Base;

	private:

		typedef boost::shared_mutex ClientsMutex;
		typedef boost::shared_lock<ClientsMutex> ClientsReadLock;
		typedef boost::unique_lock<ClientsMutex> ClientsWriteLock;

		typedef boost::mutex NoClientsConditionMutex;
		typedef NoClientsConditionMutex::scoped_lock NoClientsConditionLock;
		typedef boost::condition_variable NoClientsCondition;

	public:

		explicit DropCopyService(trdk::Context &, const Lib::IniSectionRef &);
		virtual ~DropCopyService();

	public:

		virtual void Start(const Lib::IniSectionRef &);

		virtual void CopyOrder(
				const boost::uuids::uuid &uuid,
				const boost::posix_time::ptime *orderTime,
				const std::string *tradeSystemId,
				const trdk::Strategy &,
				const uint64_t *operationId,
				const uint64_t *subOperationId,
				const trdk::Security &,
				const trdk::OrderSide &,
				const trdk::Qty &orderQty,
				const double *orderPrice,
				const trdk::TimeInForce *,
				const trdk::Lib::Currency &,
				const trdk::Qty *minQty,
				const std::string *user,
				const trdk::TradeSystem::OrderStatus &,
				const boost::posix_time::ptime *executionTime,
				const std::string *lastTradeId,
				size_t numberOfTrades,
				double avgTradePrice,
				const trdk::Qty &executedQty,
				const double *counterAmount,
				const double *bestBidPrice,
				const trdk::Qty *bestBidQty,
				const double *bestAskPrice,
				const trdk::Qty *bestAskQty);

		virtual void CopyTrade(
				const boost::posix_time::ptime &,
				const std::string &id,
				const trdk::Strategy &,
				const uint64_t *operationId,
				const uint64_t *subOperationId,
				bool isMaker,
				double tradePrice,
				const trdk::Qty &tradeQty,
				double counterAmount,
				const boost::uuids::uuid &orderUuid,
				const std::string &orderId,
				const trdk::Security &,
				const trdk::OrderSide &,
				const trdk::Qty &orderQty,
				double orderPrice,
				const trdk::TimeInForce &,
				const trdk::Lib::Currency &,
				const trdk::Qty &minQty,
				const std::string &user,
				double bestBidPrice,
				const trdk::Qty &bestBidQty,
				double bestAskPrice,
				const trdk::Qty &bestAskQty);

		virtual void GenerateDebugEvents();

	public:

		boost::asio::io_service & GetIoService() {
			return m_ioService;
		}

	public:

		void OnClientClose(const DropCopyClient &);

	protected:

		void StartNewClient(const std::string &host, const std::string &port);

		void ReconnectClient(
				size_t attemptIndex,
				const std::string &host,
				const std::string &port);
		
		void Send(const trdk::EngineService::DropCopy::ServiceData &);

	private:

		boost::asio::io_service m_ioService;
		boost::thread_group m_ioServiceThreads;

		ClientsMutex m_clientsMutex;
		NoClientsConditionMutex m_noClientsConditionMutex;
		NoClientsCondition m_noClientsCondition;
		std::vector<DropCopyClient *> m_clients;

	};

} }
