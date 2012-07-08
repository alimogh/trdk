/**************************************************************************
 *   Created: 2012/6/6/ 20:00
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#pragma once

#include "Core/TradeSystem.hpp"

class Security;

class IqFeedClient : private boost::noncopyable {

public:

	class Error : public Exception {
	public:
		explicit Error(const char *what) throw();
	};

	class ConnectError : public Error {
	public:
		ConnectError() throw();
	};


public:

	IqFeedClient();
	~IqFeedClient();

public:

	void Connect();

public:

	void SubscribeToMarketData(boost::shared_ptr<DynamicSecurity>) const;

	void RequestHistory(
			boost::shared_ptr<DynamicSecurity>,
			const boost::posix_time::ptime &fromTime)
		const;
	void RequestHistory(
			boost::shared_ptr<DynamicSecurity>,
			const boost::posix_time::ptime &fromTime,
			const boost::posix_time::ptime &toTime)
		const;

private:

	class Implementation;
	Implementation *m_pimpl;

};
