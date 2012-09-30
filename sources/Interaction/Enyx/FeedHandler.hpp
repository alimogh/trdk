/**************************************************************************
 *   Created: 2012/09/13 00:12:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "MarketDataSnapshot.hpp"
#include "Security.hpp"
#include "EnyxTypes.h"

namespace Trader {  namespace Interaction { namespace Enyx {

	class FeedHandler: public NXFeedHandler {

	private:

		//////////////////////////////////////////////////////////////////////////

		struct BySecurtiy {
			//...//
		};

		struct ByOrder {
			//...//
		};

		//////////////////////////////////////////////////////////////////////////

		struct MarketDataSnapshotHolder {

			boost::shared_ptr<MarketDataSnapshot> ptr;

			MarketDataSnapshotHolder() {
				//...//
			}

			explicit MarketDataSnapshotHolder(const boost::shared_ptr<MarketDataSnapshot> &ptr)
					: ptr(ptr) {
				//...//
			}

			const std::string & GetSymbol() const {
				return ptr->GetSymbol();
			}

		};

		typedef boost::multi_index_container<
			MarketDataSnapshotHolder,
			boost::multi_index::indexed_by<
				boost::multi_index::hashed_unique<
					boost::multi_index::tag<BySecurtiy>,
					boost::multi_index::const_mem_fun<
						MarketDataSnapshotHolder,
						const std::string &,
						&MarketDataSnapshotHolder::GetSymbol>>>>
			MarketDataSnapshots;
		typedef MarketDataSnapshots::index<BySecurtiy>::type MarketDataSnapshotsBySecurity;

		//////////////////////////////////////////////////////////////////////////

		class Order {
		public:
			Order()
					: m_isBuy(false),
					m_qty(0),
					m_price(0) {
				//...//
			}
			explicit Order(
						bool isBuy,
						Security::Qty qty,
						double price,
						boost::shared_ptr<MarketDataSnapshot> marketDataSnapshot)
					: m_isBuy(isBuy),
					m_qty(qty),
					m_price(price),
					m_marketDataSnapshot(marketDataSnapshot) {
				//...//
			}
		public:
			bool IsBuy() const {
				return m_isBuy;
			}
			Security::Qty GetQty() const {
				return m_qty;
			}
			Security::Qty ReduceQty(Security::Qty qty) {
				AssertGe(m_qty, qty);
				m_qty -= qty;
				return m_qty;
			}
			double GetPrice() const {
				return m_price;
			}
			MarketDataSnapshot & GetMarketDataSnapshot() const {
				return *m_marketDataSnapshot;
			}
		private:
			bool m_isBuy;
			mutable Security::Qty m_qty;
			double m_price;
			boost::shared_ptr<MarketDataSnapshot> m_marketDataSnapshot;
		};

		typedef std::map<EnyxOrderId, Order> Orders;

		//////////////////////////////////////////////////////////////////////////

		class OrderNotFoundError : public Exception {
		public:
			OrderNotFoundError()
					: Exception("Failed to find order") {
				//...//
			}
		};

		class ServerTimeNotSetError : public Exception {
		public:
			ServerTimeNotSetError()
					: Exception("Server time not set") {
				//...//
			}
		};

		//////////////////////////////////////////////////////////////////////////

		class RawLog;

	public:

		explicit FeedHandler(bool handlFirstLimitUpdate);
		virtual ~FeedHandler();

	public:

		void Subscribe(const boost::shared_ptr<Security> &) const throw();

		void EnableRawLog();

	public:

		virtual void onOrderMessage(NXFeedOrder *);
		virtual void onMiscMessage(NXFeedMisc *);

	protected:

		boost::posix_time::ptime GetMessageTime(const NXFeedMessage &) const;

	private:

		void HandleMessage(const nasdaqustvitch41::NXFeedOrderAdd &);
		void HandleMessage(const NXFeedOrderExecute &);
		void HandleMessage(const nasdaqustvitch41::NXFeedOrderExeWithPrice &);
		void HandleMessage(const NXFeedOrderReduce &);
		void HandleMessage(const NXFeedOrderReplace &);
		void HandleMessage(const NXFeedOrderDelete &);
		void HandleMessage(const NXFeedMiscTime &);

		template<typename Message>
		void LogAndHandleMessage(const Message &);

		Orders::iterator FindOrderPos(EnyxOrderId);
		Order & FindOrder(EnyxOrderId);

	private:

		const bool m_handlFirstLimitUpdate;

		mutable MarketDataSnapshots m_marketDataSnapshots;
		Orders m_orders;

		boost::posix_time::ptime m_serverTime;
		boost::posix_time::ptime m_prevServerLogTime;

		RawLog *m_rawLog;

	};

} } }
