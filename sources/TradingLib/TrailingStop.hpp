/**************************************************************************
 *   Created: 2016/12/15 04:15:18
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "StopOrder.hpp"

namespace trdk { namespace TradingLib {

	class TrailingStop : public trdk::TradingLib::StopOrder {

	public:

		explicit TrailingStop(
				double minProfitPerLotToActivate,
				double minProfitPerLotToClose,
				trdk::Position &);
		virtual ~TrailingStop();

	public:

		virtual void Run() override;

	protected:

		virtual const char * GetName() const override;

	private:
		
		bool CheckSignal();
		bool Activate(const trdk::Volume &plannedPnl);

	private:

		const trdk::Lib::Double m_minProfitPerLotToActivate;
		const trdk::Lib::Double m_minProfitPerLotToClose;

		bool m_isActivated;
		boost::optional<trdk::Lib::Double> m_maxProfit;
		boost::optional<trdk::Lib::Double> m_minProfit;

	};

} }
