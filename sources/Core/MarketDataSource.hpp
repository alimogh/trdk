/**************************************************************************
 *   Created: 2012/07/15 13:20:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Api.h"

class Security;
class IniFile;

//////////////////////////////////////////////////////////////////////////

class TRADER_CORE_API MarketDataSource : private boost::noncopyable {

public:

	class TRADER_CORE_API Error : public Exception {
	public:
		explicit Error(const char *what) throw();
	};

	class TRADER_CORE_API ConnectError : public Error {
	public:
		ConnectError() throw();
	};

public:

	MarketDataSource();
	virtual ~MarketDataSource();

public:

	virtual void Connect(const IniFile &, const std::string &section) = 0;

};

//////////////////////////////////////////////////////////////////////////

class TRADER_CORE_API LiveMarketDataSource : public MarketDataSource {

public:

	LiveMarketDataSource();
	virtual ~LiveMarketDataSource();

public:

	virtual void SubscribeToMarketDataLevel1(boost::shared_ptr<Security>) const = 0;
	virtual void SubscribeToMarketDataLevel2(boost::shared_ptr<Security>) const = 0;

};

//////////////////////////////////////////////////////////////////////////

class TRADER_CORE_API HistoryMarketDataSource : public MarketDataSource {

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
