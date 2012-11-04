/**************************************************************************
 *   Created: 2012/07/09 17:27:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "SecurityAlgo.hpp"
#include "Api.h"

class PositionBandle;
class PositionReporter;

namespace Trader {

	class TRADER_CORE_API Strategy
			: public Trader::SecurityAlgo,
			public boost::enable_shared_from_this<Trader::Strategy> {

	public:

		explicit Strategy(
				const std::string &tag,
				boost::shared_ptr<Trader::Security>);
		virtual ~Strategy();

	public:

		PositionReporter & GetPositionReporter();

	public:

		virtual void NotifyServiceStart(const Trader::Service &);

		virtual boost::shared_ptr<PositionBandle> TryToOpenPositions() = 0;
		virtual void TryToClosePositions(PositionBandle &) = 0;

		virtual void ReportDecision(const Trader::Position &) const = 0;

	protected:

		virtual std::auto_ptr<PositionReporter> CreatePositionReporter()
				const
				= 0;

	private:

		PositionReporter *m_positionReporter;

	};

}

