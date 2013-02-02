/**************************************************************************
 *   Created: May 19, 2012 1:07:25 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Fwd.hpp"
#include "Api.h"

namespace Trader {

	class TRADER_CORE_API Instrument : private boost::noncopyable {

	public:

		explicit Instrument(
					Trader::Context &,
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

		const Trader::Context & GetContext() const;
		Trader::Context & GetContext();

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}
