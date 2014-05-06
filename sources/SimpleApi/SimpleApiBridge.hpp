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

namespace trdk { namespace SimpleApi {

	class Bridge : private boost::noncopyable {

	public:

		typedef uintptr_t BarServiceHandle;

	public:

		explicit Bridge(boost::shared_ptr<trdk::Engine::Context>);

	public:

		BarServiceHandle ResolveFutOpt(
					const std::string &symbol,
					const std::string &exchange,
					const std::string &expirationDate,
					double strike,
					const std::string &right,
					const std::string &tradingClass,
					const boost::posix_time::ptime &dataStartTime,
					int32_t barIntervalType)
				const;
		Services::BarService & GetBarService(const BarServiceHandle &);
		const Services::BarService & GetBarService(const BarServiceHandle &) const;

		double GetCashBalance() const;

	private:

		boost::shared_ptr<trdk::Engine::Context> m_context;

	};

} }
