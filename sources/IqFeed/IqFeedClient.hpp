/**************************************************************************
 *   Created: 2012/6/6/ 20:00
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#pragma once

#include "Core/MarketDataSource.hpp"

class IqFeedClient
	: public LiveMarketDataSource,
	public HistoryMarketDataSource {

public:

	IqFeedClient();
	virtual ~IqFeedClient();

public:

	virtual void Connect(const Settings &);

public:

	virtual void SubscribeToMarketDataLevel1(boost::shared_ptr<Security>) const;
	virtual void SubscribeToMarketDataLevel2(boost::shared_ptr<Security>) const;

	virtual void RequestHistory(
			boost::shared_ptr<Security>,
			const boost::posix_time::ptime &fromTime)
		const;
	virtual void RequestHistory(
			boost::shared_ptr<Security>,
			const boost::posix_time::ptime &fromTime,
			const boost::posix_time::ptime &toTime)
		const;

private:

	class Implementation;
	Implementation *m_pimpl;

};
