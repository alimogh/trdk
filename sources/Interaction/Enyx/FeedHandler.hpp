/**************************************************************************
 *   Created: 2012/09/13 00:12:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

namespace Trader {  namespace Interaction { namespace Enyx {

	class Security;

} } }

namespace Trader {  namespace Interaction { namespace Enyx {

	class FeedHandler: public NXFeedHandler {

	private:

		class SecuritySubscribtion {

		private:

			typedef void (OrderAddSlotSignature)();
			typedef boost::function<OrderAddSlotSignature> OrderAddSlot;
			typedef boost::signal<OrderAddSlotSignature> OrderAddSignal;

			typedef void (OrderDelSlotSignature)();
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

			void OnOrderAdd() {
				(*m_orderAddSignal)();
			}

			void OnOrderDel() {
				(*m_orderDelSignal)();
			}

		private:

			std::string m_exchange;
			std::string m_symbol;

			boost::shared_ptr<OrderAddSignal> m_orderAddSignal;
			boost::shared_ptr<OrderDelSignal> m_orderDelSignal;

		};

		struct BySecurtiy {
			//...//
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

	public:

		virtual ~FeedHandler();

	public:

		void Subscribe(const boost::shared_ptr<Security> &) const throw();

	public:

		virtual void onOrderMessage(NXFeedOrder *);

	private:

		mutable Subscribtion m_subscribtion;

	};

} } }
