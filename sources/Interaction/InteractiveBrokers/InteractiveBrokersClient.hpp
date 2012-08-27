/**************************************************************************
 *   Created: May 26, 2012 8:29:02 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#pragma once

#include "Core/TradeSystem.hpp"

class Security;

class InteractiveBrokersClient : protected EWrapper {

public:

	typedef std::list<boost::function<void()>> CallbackList;

	typedef void (OrderStatusSlotSignature)(
			Trader::TradeSystem::OrderId id,
			Trader::TradeSystem::OrderId parentId,
			Trader::TradeSystem::OrderStatus,
			long filled,
			long remaining,
			double avgFillPrice,
			double lastFillPrice,
			const std::string &whyHeld,
			CallbackList &);
	typedef boost::function<OrderStatusSlotSignature> OrderStatusSlot;

public:

	InteractiveBrokersClient(
			int clientId = 0,
			const std::string &host = "127.0.0.1",
			unsigned short port = 7496);
	virtual ~InteractiveBrokersClient();

public:

	void StartData();

public:

	Trader::TradeSystem::OrderId SendAsk(const Security &, Trader::TradeSystem::OrderQty);
	Trader::TradeSystem::OrderId SendAsk(
			const Security &,
			Trader::TradeSystem::OrderQty,
			double);
	Trader::TradeSystem::OrderId SendAskWithMarketPrice(
			const Security &,
			Trader::TradeSystem::OrderQty,
			double stopPrice);
	Trader::TradeSystem::OrderId SendIocAsk(
			const Security &,
			Trader::TradeSystem::OrderQty,
			double);

	Trader::TradeSystem::OrderId SendBid(const Security &, Trader::TradeSystem::OrderQty);
	Trader::TradeSystem::OrderId SendBid(
			const Security &,
			Trader::TradeSystem::OrderQty,
			double);
	Trader::TradeSystem::OrderId SendBidWithMarketPrice(
			const Security &,
			Trader::TradeSystem::OrderQty,
			double stopPrice);
	Trader::TradeSystem::OrderId SendIocBid(
			const Security &,
			Trader::TradeSystem::OrderQty,
			double);

	void CancelOrder(Trader::TradeSystem::OrderId);

public:

	void Subscribe(const OrderStatusSlot &) const;
	void SubscribeToMarketDataLevel2(boost::shared_ptr<Security>) const;

private:

	virtual void tickPrice(TickerId, TickType, double, int);
	virtual void tickSize(TickerId, TickType, int);
	virtual void tickOptionComputation(
			TickerId,
			TickType,
			double,
			double,
			double,
			double,
			double,
			double,
			double,
			double);
	virtual void tickGeneric(TickerId, TickType, double);
	virtual void tickString(TickerId, TickType, const IBString &);
	virtual void tickEFP(
			TickerId,
			TickType,
			double,
			const IBString &,
			double,
			int,
			const IBString &,
			double,
			double);
	virtual void orderStatus(
			OrderId,
			const IBString &,
			int,
			int,
			double,
			int,
			int,
			double,
			int,
			const IBString &);
	virtual void openOrder(OrderId orderId, const Contract &, const Order &, const OrderState &);
	virtual void openOrderEnd();
	virtual void winError(const IBString &, int);
	virtual void connectionClosed();
	virtual void updateAccountValue(const IBString &, const IBString &, const IBString &, const IBString &);
	virtual void updatePortfolio(
			const Contract &,
			int,
			double,
			double,
			double,
			double,
			double,
			const IBString &);
	virtual void updateAccountTime(const IBString &);
	virtual void accountDownloadEnd(const IBString &);
	virtual void nextValidId(OrderId);
	virtual void contractDetails(int, const ContractDetails &);
	virtual void bondContractDetails(int, const ContractDetails &);
	virtual void contractDetailsEnd(int);
	virtual void execDetails(int, const Contract &, const Execution &);
	virtual void execDetailsEnd(int);
	virtual void error(const int, const int, const IBString);
	virtual void updateMktDepth(TickerId, int, int, int, double, int);
	virtual void updateMktDepthL2(TickerId, int, IBString, int, int, double, int);
	virtual void updateNewsBulletin(int, int, const IBString &, const IBString &);
	virtual void managedAccounts(const IBString &);
	virtual void receiveFA(faDataType, const IBString &);
	virtual void historicalData(
			TickerId,
			const IBString &,
			double,
			double,
			double,
			double,
			int,
			int,
			double,
			int);
	virtual void scannerParameters(const IBString &);
	virtual void scannerData(
			int,
			int,
			const ContractDetails &,
			const IBString &,
			const IBString &,
			const IBString &,
			const IBString &);
	virtual void scannerDataEnd(int);
	virtual void realtimeBar(
			TickerId,
			long,
			double,
			double,
			double,
			double,
			long,
			double,
			int);
	virtual void currentTime(long);
	virtual void fundamentalData(TickerId, const IBString &);
	virtual void deltaNeutralValidation(int, const UnderComp &);
	virtual void tickSnapshotEnd(int);
	virtual void marketDataType(TickerId, int);

private:

	class Implementation;
	Implementation *m_pimpl;

};
