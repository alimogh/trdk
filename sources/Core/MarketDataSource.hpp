/**************************************************************************
 *   Created: 2012/07/15 13:20:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

class Security;

//////////////////////////////////////////////////////////////////////////

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

};

//////////////////////////////////////////////////////////////////////////

class LiveMarketDataSource : public MarketDataSource {

public:

	LiveMarketDataSource();
	virtual ~LiveMarketDataSource();

public:

	virtual void SubscribeToMarketDataLevel1(boost::shared_ptr<Security>) const = 0;
	virtual void SubscribeToMarketDataLevel2(boost::shared_ptr<Security>) const = 0;

};

//////////////////////////////////////////////////////////////////////////

class HistoryMarketDataSource : public MarketDataSource {

public:

	HistoryMarketDataSource();
	virtual ~HistoryMarketDataSource();

public:

	virtual void RequestHistory(
			boost::shared_ptr<Security>,
			const boost::posix_time::ptime &fromTime)
		const
		= 0;
	virtual void RequestHistory(
			boost::shared_ptr<Security>,
			const boost::posix_time::ptime &fromTime,
			const boost::posix_time::ptime &toTime)
		const
		= 0;

};

//////////////////////////////////////////////////////////////////////////
