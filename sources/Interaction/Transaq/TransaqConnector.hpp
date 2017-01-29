/**************************************************************************
 *   Created: 2016/10/30 17:06:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Fwd.hpp"
#include "Api.h"
#include "Common/TimeMeasurement.hpp"

namespace trdk { namespace Interaction { namespace Transaq {

	////////////////////////////////////////////////////////////////////////////////
	
	class TRDK_INTERACTION_TRANSAQ_API Connector : private boost::noncopyable {

	public:

		class Exception : public trdk::Lib::Exception {
		public:
			explicit Exception(const char *what) noexcept
				: trdk::Lib::Exception(what) {
				//...//
			}
		};

		class ConnectException : public Exception {
		public:
			explicit ConnectException(const char *what) noexcept
				: Exception(what) {
				//...//
			}
		};

	protected:
	
		typedef boost::unordered_map<
				std::string,
				boost::function<
					void(
						const boost::property_tree::ptree &,
						const Lib::TimeMeasurement::Milestones &,
						const Lib::TimeMeasurement::Milestones::TimePoint &)>>
			DataHandlers;

	private:

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;

	public:

		explicit Connector(const Context &, ModuleEventsLog &);
		virtual ~Connector();

	public:

		void Init();

		bool IsConnected() const;
		void Connect(const Lib::IniSectionRef &);

		ModuleEventsLog & GetLog() {
			return m_log;
		}

		const Context & GetContext() const {
			return m_context;
		}

	protected:

		virtual ConnectorContext & GetConnectorContext() = 0;

		virtual DataHandlers GetDataHandlers() const;

		std::pair<boost::property_tree::ptree, std::string>
		SendCommand(
				const char *command,
				boost::property_tree::ptree &&,
				const Lib::TimeMeasurement::Milestones &);

		const boost::posix_time::time_duration & GetTimeZoneDiff() const {
			return m_timeZoneDiff;
		}

		virtual void OnReconnect() = 0;

	private:

		void Connect(const  boost::property_tree::ptree &, Lock &);

		void RunReconnectionTaks();
		bool ScheduleReconnect(const Lock &);

		void OnNewDataMessage(
				const boost::property_tree::ptree &,
				const Lib::TimeMeasurement::Milestones &,
				const Lib::TimeMeasurement::Milestones::TimePoint &);
		void OnErrorMessage(
				const boost::property_tree::ptree &,
				const Lib::TimeMeasurement::Milestones &,
				const Lib::TimeMeasurement::Milestones::TimePoint &);
		void OnMarketsInfoMessage(
				const boost::property_tree::ptree &,
				const Lib::TimeMeasurement::Milestones &,
				const Lib::TimeMeasurement::Milestones::TimePoint &);
		void OnBoardsInfoMessage(
				const boost::property_tree::ptree &,
				const Lib::TimeMeasurement::Milestones &,
				const Lib::TimeMeasurement::Milestones::TimePoint &);
		void OnClientsInfoMessage(
				const boost::property_tree::ptree &,
				const Lib::TimeMeasurement::Milestones &,
				const Lib::TimeMeasurement::Milestones::TimePoint &);
		void OnServerStatusMessage(
				const boost::property_tree::ptree &,
				const Lib::TimeMeasurement::Milestones &,
				const Lib::TimeMeasurement::Milestones::TimePoint &);

	private:

		const Context &m_context;
		ModuleEventsLog &m_log;

		DataHandlers m_dataHandlers;

		boost::signals2::scoped_connection m_subscribtion;

		mutable Mutex m_mutex;
		
		bool m_isConnected;
		size_t m_numberOfReconnectes;
		std::unique_ptr<boost::property_tree::ptree> m_connectCommand;
		boost::condition_variable m_connectCondition;

		const boost::posix_time::time_duration m_timeZoneDiff;

		bool m_isStopped;

		boost::thread m_reconnectThread;
		boost::condition_variable m_reconnectCondition;
		boost::optional<boost::posix_time::ptime> m_reconnectStartTime;

	};

	////////////////////////////////////////////////////////////////////////////////

	class TRDK_INTERACTION_TRANSAQ_API TradingConnector : public Connector {

	public:

		typedef Connector Base;

		typedef int64_t TradingSystemOrderId;

	public:

		explicit TradingConnector(
				const Context &,
				ModuleEventsLog &,
				const Lib::IniSectionRef &);
		virtual ~TradingConnector();

	public:

		OrderId SendSellOrder(
				const std::string &board,
				const std::string &symbol,
				double price,
				uint8_t pricePrecision,
				const Qty &,
				const Lib::TimeMeasurement::Milestones &);
		OrderId SendBuyOrder(
				const std::string &board,
				const std::string &symbol,
				double price,
				uint8_t pricePrecision,
				const Qty &,
				const Lib::TimeMeasurement::Milestones &);

		//! Send order cancel request.
		/** @return	true if order found and request was sent, false if order
		 *			is not found.
		 */
		bool SendCancelOrder(
				const OrderId &,
				const Lib::TimeMeasurement::Milestones &);

	protected:

		virtual DataHandlers GetDataHandlers() const override;

		virtual void OnOrderUpdate(
				const OrderId &,
				TradingSystemOrderId &&,
				OrderStatus &&,
				Qty &&remainingQty,
				std::string &&tradingSystemMessage,
				const Lib::TimeMeasurement::Milestones::TimePoint &)
			= 0;
		virtual void OnTrade(
				std::string &&id,
				TradingSystemOrderId  &&,
				double price,
				Qty &&)
			= 0;

	private:

		OrderId SendOrder(
				const boost::property_tree::ptree &,
				const std::string &board,
				const std::string &symbol,
				double price,
				uint8_t pricePrecision,
				const Qty &,
				const Lib::TimeMeasurement::Milestones &);

		void OnOrdersMessage(
				const boost::property_tree::ptree &,
				const Lib::TimeMeasurement::Milestones &,
				const Lib::TimeMeasurement::Milestones::TimePoint &);
		void OnTradeMessage(
				const boost::property_tree::ptree &,
				const Lib::TimeMeasurement::Milestones &,
				const Lib::TimeMeasurement::Milestones::TimePoint &);

	private:

		boost::property_tree::ptree m_defaultBuyOrder;
		boost::property_tree::ptree m_defaultSellOrder;

	};

	////////////////////////////////////////////////////////////////////////////////

	class MarketDataSourceConnector : public Connector {

	public:

		typedef Connector Base;

	public:

		explicit MarketDataSourceConnector(const Context &, ModuleEventsLog &);
		virtual ~MarketDataSourceConnector();

	public:

		//! Sends market data subscription request.
		/** @param list	List of symbols to subscribe. Each item: first item
		  *				is a board, second is a security symbol.
		  */
		void SendMarketDataSubscriptionRequest(
				const std::vector<std::pair<std::string, std::string>> &list);

	protected:

		virtual DataHandlers GetDataHandlers() const override;

		virtual void OnNewTick(
				const boost::posix_time::ptime &,
				const std::string &board,
				const std::string &symbol,
				double price,
				const Qty &,
				const Lib::TimeMeasurement::Milestones &)
			= 0;

		virtual void OnLevel1Update(
				const std::string &board,
				const std::string &symbol,
				boost::optional<double> &&bidPrice,
				boost::optional<double> &&bidQty,
				boost::optional<double> &&askPrice,
				boost::optional<double> &&askQty,
				const Lib::TimeMeasurement::Milestones &)
			= 0;

	private:

		void OnTicksMessage(
				const boost::property_tree::ptree &,
				const Lib::TimeMeasurement::Milestones &,
				const Lib::TimeMeasurement::Milestones::TimePoint &);
		void OnQuotationsMessage(
				const boost::property_tree::ptree &,
				const Lib::TimeMeasurement::Milestones &,
				const Lib::TimeMeasurement::Milestones::TimePoint &);

	};

	////////////////////////////////////////////////////////////////////////////////

} } }
