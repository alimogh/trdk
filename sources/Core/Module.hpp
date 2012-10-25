/**************************************************************************
 *   Created: 2012/09/23 14:31:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once
#include "Api.h"

namespace Trader {

	class TRADER_CORE_API Module : private boost::noncopyable {
		
	public:

		explicit Module(const std::string &tag);
		virtual ~Module();

	public:

		virtual const std::string & GetName() const = 0;
		const std::string & GetTag() const throw();

	private:

		const std::string m_tag;

	};

}

