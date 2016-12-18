/**************************************************************************
 *   Created: 2016/12/18 17:10:13
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

	class TakeProfit : public trdk::TradingLib::StopOrder {

	public:

		explicit TakeProfit(
				double minProfitPerLotToActivate,
				double minProfitRatioToClose,
				trdk::Position &);
		virtual ~TakeProfit();

	public:

		virtual void Run() override;

	protected:

		virtual const char * GetName() const override;

	private:
		
		bool CheckSignal();
		bool Activate(const trdk::Volume &plannedPnl);

	private:

		const trdk::Lib::Double m_minProfitPerLotToActivate;
		const trdk::Lib::Double m_minProfitRatioToClose;

		bool m_isActivated;
		boost::optional<trdk::Lib::Double> m_minProfit;
		boost::optional<trdk::Lib::Double> m_maxProfit;

	};

} }
