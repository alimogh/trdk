/**************************************************************************
 *   Created: 2012/09/13 00:12:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

namespace Trader {  namespace Interaction { namespace Enyx {

	class FeedHandler: public NXFeedHandler {

	private:

		class SubscribedSecurity {
		public:
			explicit SubscribedSecurity(boost::shared_ptr<Security> security);
		public:
			const std::string & GetExchange() const;
			const std::string & GetSymbol() const;
		private:
			boost::shared_ptr<Security> m_security;
		};

		struct BySecurtiy {
			//...//
		};

		typedef boost::multi_index_container<
			SubscribedSecurity,
			boost::multi_index::indexed_by<
				boost::multi_index::hashed_unique<
					boost::multi_index::tag<BySecurtiy>,
					boost::multi_index::composite_key<
						SubscribedSecurity,
						boost::multi_index::const_mem_fun<
							SubscribedSecurity,
							const std::string &,
							&SubscribedSecurity::GetExchange>,
						boost::multi_index::const_mem_fun<
							SubscribedSecurity,
							const std::string &,
							&SubscribedSecurity::GetSymbol>>>>>
			Subscribtion;

		typedef Subscribtion::index<BySecurtiy>::type SubscribedBySecurity;

	public:

		virtual ~FeedHandler();

	public:

		void Subscribe(boost::shared_ptr<Security>) const throw();

	public:

		virtual void onOrderMessage(NXFeedOrder *);

	private:

		mutable Subscribtion m_subscribtion;

	};

} } }
