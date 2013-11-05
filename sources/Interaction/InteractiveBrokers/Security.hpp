/**************************************************************************
 *   Created: 2013/05/01 16:43:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Security.hpp"

namespace trdk {  namespace Interaction { namespace InteractiveBrokers {


	class Security : public trdk::Security {

	public:

		typedef trdk::Security Base;

	public:

		explicit Security(Context &, const Lib::Symbol &);

	public:

		bool IsLevel1Required() const {
			return Base::IsLevel1Required();
		}
		bool IsLevel1UpdatesRequired() const {
			return Base::IsLevel1UpdatesRequired();
		}
		bool IsLevel1TicksRequired() const {
			return Base::IsLevel1TicksRequired();
		}
		bool IsTradesRequired() const {
			return Base::IsTradesRequired();
		}
		bool IsBrokerPositionRequired() const {
			return Base::IsBrokerPositionRequired();
		}
		bool IsBarsRequired() const {
			return Base::IsBarsRequired();
		}

		void AddLevel1Tick(
					const boost::posix_time::ptime &time,
					const Level1TickValue &tick) {
			Base::AddLevel1Tick(time, tick);
		}
		void AddLevel1Tick(
					const boost::posix_time::ptime &time,
					const Level1TickValue &tick1,
					const Level1TickValue &tick2) {
			Base::AddLevel1Tick(time, tick1, tick2);
		}

		void AddBar(const Bar &bar) {
			Base::AddBar(bar);
		}

		void SetBrokerPosition(Qty qty, bool isInitial) {
			Base::SetBrokerPosition(qty, isInitial);
		}

	};

} } }
