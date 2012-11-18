/**************************************************************************
 *   Created: 2012/09/23 14:31:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Security.hpp"
#include "Api.h"

namespace Trader {

	class TRADER_CORE_API Module : private boost::noncopyable {
		
	public:

		explicit Module(const std::string &tag);
		virtual ~Module();

	public:

		virtual const std::string & GetTypeName() const = 0;
		virtual const std::string & GetName() const = 0;
		const std::string & GetTag() const throw();

	public:

		virtual void NotifyServiceStart(const Trader::Service &);

	public:

		virtual void OnNewTrade(
					const Trader::Security &,
					const boost::posix_time::ptime &,
					Trader::Security::ScaledPrice price,
					Trader::Security::Qty qty,
					bool isBuy);

		virtual void OnServiceDataUpdate(const Trader::Service &);

	private:

		const std::string m_tag;

	};

}

namespace std {
	TRADER_CORE_API std::ostream & operator <<(
				std::ostream &,
				const Trader::Module &);
}
