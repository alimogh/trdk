/**************************************************************************
 *   Created: 2012/09/23 10:02:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Security.hpp"
#include "EnyxTypes.h"

namespace Trader {  namespace Interaction { namespace Enyx {

	class FirstLimitUpdateHandler : private boost::noncopyable {

	private:

		struct BySymbol {
			//...//
		};

		struct SecurityHolder {
			
			boost::shared_ptr<Security> security;

			const std::string & GetSymbol() const {
				return security->GetSymbol();
			}

		};

		typedef boost::multi_index_container<
			SecurityHolder,
			boost::multi_index::indexed_by<
				boost::multi_index::hashed_unique<
					boost::multi_index::tag<BySymbol>,
					boost::multi_index::const_mem_fun<
						SecurityHolder,
						const std::string &,
						&SecurityHolder::GetSymbol>>>>
			SecurityList;
		typedef SecurityList::index<BySymbol>::type SecurityBySymbol;

	public:

		FirstLimitUpdateHandler();
		~FirstLimitUpdateHandler();

	public:

		void Register(const boost::shared_ptr<Security> &) throw();

	public:

		void HandleUpdate(
				const std::string &symbol,
				Security::ScaledPrice price,
				Security::Qty qty,
				bool isBuy);

	private:

		SecurityList m_securities;

	};

} } }
