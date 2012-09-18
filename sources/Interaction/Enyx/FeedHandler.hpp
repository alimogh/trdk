/**************************************************************************
 *   Created: 2012/09/13 00:12:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Security.hpp"

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

		class SecuritySubscribtion {

		private:

			typedef void (OrderAddSlotSignature)(Security::OrderId, Security::Qty, double);
			typedef boost::function<OrderAddSlotSignature> OrderAddSlot;
			typedef boost::signal<OrderAddSlotSignature> OrderAddSignal;

			typedef void (OrderExecSlotSignature)(Security::OrderId, Security::Qty, double);
			typedef boost::function<OrderExecSlotSignature> OrderExecSlot;
			typedef boost::signal<OrderExecSlotSignature> OrderExecSignal;

			typedef void (OrderChangeSlotSignature)(Security::OrderId, Security::Qty, double);
			typedef boost::function<OrderChangeSlotSignature> OrderChangeSlot;
			typedef boost::signal<OrderChangeSlotSignature> OrderChangeSignal;

			typedef void (OrderDelSlotSignature)(Security::OrderId, Security::Qty, double);
			typedef boost::function<OrderDelSlotSignature> OrderDelSlot;
			typedef boost::signal<OrderDelSlotSignature> OrderDelSignal;

		public:

			SecuritySubscribtion(const boost::shared_ptr<Security> &);

		public:

			const std::string & GetExchange() const;
			const std::string & GetSymbol() const;

		public:

			void Subscribe(const boost::shared_ptr<Security> &);

		public:

			void OnOrderAdd(
						bool isBuy,
						uint64_t id,
						Security::Qty qty,
						double price)
					const {
				isBuy
					?	(*m_buyOrderAddSignal)(id, qty, price)
					:	(*m_sellOrderAddSignal)(id, qty, price);
			}

			void OnOrderExec(bool isBuy, uint64_t id, Security::Qty qty, double price) const {
				isBuy
					?	(*m_buyOrderExecSignal)(id, qty, price)
					:	(*m_sellOrderExecSignal)(id, qty, price);
			}

			void OnOrderChange(bool isBuy, uint64_t id, Security::Qty newQty, double newPrice) const {
				isBuy
					?	(*m_buyOrderChangeSignal)(id, newQty, newPrice)
					:	(*m_sellOrderChangeSignal)(id, newQty, newPrice);
			}

			void OnOrderDel(bool isBuy, uint64_t id, Security::Qty qty, double price) const {
				isBuy
					?	(*m_buyOrderDelSignal)(id, qty, price)
					:	(*m_sellOrderDelSignal)(id, qty, price);
			}

		private:

			std::string m_exchange;
			std::string m_symbol;

			boost::shared_ptr<OrderAddSignal> m_buyOrderAddSignal;
			boost::shared_ptr<OrderAddSignal> m_sellOrderAddSignal;

			boost::shared_ptr<OrderExecSignal> m_buyOrderExecSignal;
			boost::shared_ptr<OrderExecSignal> m_sellOrderExecSignal;

			boost::shared_ptr<OrderExecSignal> m_buyOrderChangeSignal;
			boost::shared_ptr<OrderExecSignal> m_sellOrderChangeSignal;

			boost::shared_ptr<OrderDelSignal> m_buyOrderDelSignal;
			boost::shared_ptr<OrderDelSignal> m_sellOrderDelSignal;

		};

		typedef boost::multi_index_container<
			SecuritySubscribtion,
			boost::multi_index::indexed_by<
				boost::multi_index::hashed_unique<
					boost::multi_index::tag<BySecurtiy>,
					boost::multi_index::composite_key<
						SecuritySubscribtion,
						boost::multi_index::const_mem_fun<
							SecuritySubscribtion,
							const std::string &,
							&SecuritySubscribtion::GetExchange>,
						boost::multi_index::const_mem_fun<
							SecuritySubscribtion,
							const std::string &,
							&SecuritySubscribtion::GetSymbol>>>>>
			Subscribtion;

		typedef Subscribtion::index<BySecurtiy>::type SubscribtionBySecurity;

		struct SubscribtionUpdate {
			explicit SubscribtionUpdate(const boost::shared_ptr<Security> &);
			void operator ()(SecuritySubscribtion &);
		private:
			const boost::shared_ptr<Security> &m_security;
		};

		//////////////////////////////////////////////////////////////////////////

		class Order {

		public:

			explicit Order(
						Security::OrderId orderId,
						bool isBuy,
						Security::Qty qty,
						double price,
						const SecuritySubscribtion &subscribtion)
					: m_orderId(orderId),
					m_isBuy(isBuy),
					m_qty(qty),
					m_price(price),
					m_subscribtion(subscribtion) {
				//...//
			}

		public:

			Security::OrderId GetOrderId() const {
				return m_orderId;
			}

			bool IsBuy() const {
				return m_isBuy;
			}

			Security::Qty GetQty() const {
				return m_qty;
			}
			Security::Qty DecreaseQty(Security::Qty qty) const {
				AssertGe(m_qty, qty);
				m_qty -= qty;
				return m_qty;
			}

			double GetPrice() const {
				m_price;
			}

			const SecuritySubscribtion & GetSubscribtion() const {
				return m_subscribtion;
			}

		private:

			Security::OrderId m_orderId;
			bool m_isBuy;
			mutable Security::Qty m_qty;
			double m_price;
			boost::reference_wrapper<const SecuritySubscribtion> m_subscribtion;

		};

		typedef boost::multi_index_container<
			Order,
			boost::multi_index::indexed_by<
				boost::multi_index::hashed_unique<
					boost::multi_index::tag<ByOrder>,
					boost::multi_index::const_mem_fun<
						Order,
						Security::OrderId,
						&Order::GetOrderId>>>>
			Orders;
		typedef Orders::index<ByOrder>::type OrderById;

		//////////////////////////////////////////////////////////////////////////

		class OrderNotFoundError : public Exception {
		public:
			OrderNotFoundError()
					: Exception("Failed to find order") {
				//...//
			}
		};

		//////////////////////////////////////////////////////////////////////////

	public:

		virtual ~FeedHandler();

	public:

		void Subscribe(const boost::shared_ptr<Security> &) const throw();

	public:

		virtual void onOrderMessage(NXFeedOrder *);

	private:

		void HandleOrderAdd(const nasdaqustvitch41::NXFeedOrderAdd &);
		void HandleOrderExec(const NXFeedOrderExecute &);
		void HandleOrderExec(const nasdaqustvitch41::NXFeedOrderExeWithPrice &);
		void HandleOrderChange(const NXFeedOrderReduce &);
		void HandleOrderDel(const NXFeedOrderDelete &);

		OrderById::iterator FindOrderPos(Security::OrderId);
		const Order & FindOrder(Security::OrderId);

	private:

		mutable Subscribtion m_subscribtion;
		Orders m_orders;

	};

} } }
