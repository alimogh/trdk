/**************************************************************************
 *   Created: 2012/09/23 14:31:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Fwd.hpp"
#include "Api.h"

namespace Trader {

	class TRADER_CORE_API Module : private boost::noncopyable {

	protected:

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;
		
	public:

		explicit Module(
					const std::string &tag,
					boost::shared_ptr<const Trader::Settings>);
		virtual ~Module();

	public:

		virtual const std::string & GetTypeName() const = 0;
		virtual const std::string & GetName() const = 0;
		const std::string & GetTag() const throw();

	public:

		const Trader::Settings & GetSettings() const;

	public:

		virtual void OnServiceStart(const Trader::Service &);

	protected:

		Mutex & GetMutex() const;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}

namespace std {
	TRADER_CORE_API std::ostream & operator <<(
				std::ostream &,
				const Trader::Module &)
			throw();
}
