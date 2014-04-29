/**************************************************************************
 *   Created: 2014/04/29 23:28:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace CApi {

	class Bridge : private boost::noncopyable {

	public:

		explicit Bridge(
				Context &,
				const std::string &account,
				const std::string &expirationDate,
				double strike);

	public:

		double GetCashBalance() const;
		Qty GetPosition(const std::string &symbol) const;

	protected:

		Security & GetSecurity(const std::string &symbol) const;
		Lib::Symbol GetSymbol(std::string symbol) const;

	private:

		mutable Context &m_context;

		const std::string m_account;

		const std::string m_expirationDate;
		const double m_strike;


	};

} }
