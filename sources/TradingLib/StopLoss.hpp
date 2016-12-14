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

#include "Core/Position.hpp"

namespace trdk { namespace TradingLib {

	class StopLoss : public trdk::Position::Algo {

	public:

		explicit StopLoss(double maxLossPerQty, trdk::Position &);
		virtual ~StopLoss();

	public:

		virtual void Run() override;

	protected:

		//! Closes cancels all active orders, closes by actual market price (not
		//! by market order) if there are no active orders.
		virtual void OnHit();

		trdk::Position & GetPosition();
		const trdk::Position & GetPosition() const;

	private:

		const double m_maxLossPerQty;
		trdk::Position & m_position;

	};

} }
