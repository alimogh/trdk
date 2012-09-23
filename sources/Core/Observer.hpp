/**************************************************************************
 *   Created: 2012/09/19 23:57:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Module.hpp"
#include "Api.h"

namespace Trader {
	class Security;
}

namespace Trader {

	class TRADER_CORE_API Observer
		: public Trader::Module,
		public boost::enable_shared_from_this<Trader::Observer> {

	public:

		typedef std::list<boost::shared_ptr<const Trader::Security>> NotifyList;

	public:

		Observer(const std::string &tag, const Observer::NotifyList &notifyList);
		virtual ~Observer();

	public:

		const NotifyList & GetNotifyList() const;

	public:

		virtual void OnUpdate(const Trader::Security &) = 0;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}
