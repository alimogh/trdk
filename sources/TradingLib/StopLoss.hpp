/**************************************************************************
 *   Created: 2016/12/14 02:51:16
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

	class StopLoss : public trdk::TradingLib::StopOrder {

	public:

		explicit StopLoss(double maxLossPerLot, trdk::Position &);
		virtual ~StopLoss();

	public:

		virtual void Run() override;

	protected:

		virtual const char * GetName() const override;

	private:

		const trdk::Lib::Double m_maxLossPerLot;

	};

} }
