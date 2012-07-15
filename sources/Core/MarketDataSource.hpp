/**************************************************************************
 *   Created: 2012/07/15 13:20:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

class DynamicSecurity;

class MarketDataSource : private boost::noncopyable {

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

	MarketDataSource();
	virtual ~MarketDataSource();

public:

	virtual void Connect() = 0;

public:

	virtual void SubscribeToMarketData(boost::shared_ptr<DynamicSecurity>) const = 0;

	virtual void RequestHistory(
			boost::shared_ptr<DynamicSecurity>,
			const boost::posix_time::ptime &fromTime)
		const
		= 0;
	virtual void RequestHistory(
			boost::shared_ptr<DynamicSecurity>,
			const boost::posix_time::ptime &fromTime,
			const boost::posix_time::ptime &toTime)
		const
		= 0;

};
