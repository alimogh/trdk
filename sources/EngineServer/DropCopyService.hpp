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

		struct Io {

			std::vector<DropCopyClient *> clients;

			boost::thread_group threads;
			boost::asio::io_service service;

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

		void OnClientClose(const DropCopyClient &);

	protected:

		void StartNewClient(const std::string &host, const std::string &port);

		void ReconnectClient(
				size_t attemptIndex,
				const std::string &host,
				const std::string &port);
		
		void Send(const trdk::EngineService::DropCopy::ServiceData &);

	private:

		ClientsMutex m_clientsMutex;

		std::unique_ptr<Io> m_io;

	};

} }
