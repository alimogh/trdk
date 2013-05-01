/**************************************************************************
 *   Created: May 19, 2012 1:07:25 AM
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

namespace trdk {

	class TRDK_CORE_API Instrument : private boost::noncopyable {

	public:

		explicit Instrument(
					trdk::Context &,
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange);
		~Instrument();

	public:

		const std::string & GetFullSymbol() const throw();
		const std::string & GetSymbol() const throw();
		const std::string & GetPrimaryExchange() const;
		const std::string & GetExchange() const;

	public:

		const trdk::Context & GetContext() const;
		trdk::Context & GetContext();

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}
