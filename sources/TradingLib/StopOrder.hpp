/**************************************************************************
 *   Created: 2016/12/15 04:46:00
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

	class StopOrder : public trdk::Position::Algo {

	public:

		explicit StopOrder(trdk::Position &);
		virtual ~StopOrder();

	public:

		trdk::ModuleTradingLog & GetTradingLog() const noexcept;

	protected:

		virtual const char * GetName() const = 0;

		virtual void OnHit();

		trdk::Position & GetPosition();
		const trdk::Position & GetPosition() const;

	private:

		trdk::Position & m_position;

	};

} }
