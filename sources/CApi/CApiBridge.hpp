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

		typedef uintptr_t SecurityId;

	public:

		explicit Bridge(boost::shared_ptr<Context>);

	public:

		SecurityId ResolveFutOpt(
					const std::string &symbol,
					const std::string &exchange,
					const std::string &expirationDate,
					double strike,
					const std::string &right)
				const;
		Security & GetSecurity(const SecurityId &);
		const Security & GetSecurity(const SecurityId &) const;

		double GetCashBalance() const;

	private:

		boost::shared_ptr<Context> m_context;

	};

} }
