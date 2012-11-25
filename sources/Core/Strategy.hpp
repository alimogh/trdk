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

namespace Trader {

	class TRADER_CORE_API Strategy
			: public Trader::SecurityAlgo,
			public boost::enable_shared_from_this<Trader::Strategy> {

	public:

		using boost::enable_shared_from_this<Trader::Strategy>::shared_from_this;

	public:

		explicit Strategy(
				const std::string &tag,
				boost::shared_ptr<Trader::Security>,
				boost::shared_ptr<const Settings>);
		virtual ~Strategy();

	public:

		virtual const std::string & GetTypeName() const;

	public:

		bool IsTimeToUpdate() const;
		bool IsBlocked() const;

		void CheckPositions(bool byTimeout);

		PositionReporter & GetPositionReporter();

	public:

		virtual boost::shared_ptr<PositionBandle> TryToOpenPositions() = 0;
		virtual void TryToClosePositions(PositionBandle &) = 0;

		virtual void ReportDecision(const Trader::Position &) const = 0;

	protected:

		virtual std::auto_ptr<PositionReporter> CreatePositionReporter()
				const
				= 0;

	private:

 		class Implementation;
 		Implementation *m_pimpl;

	};

}

