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

		explicit Security(
					Context &,
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange,
					bool logMarketData);

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

		void SetLevel1(
					const boost::posix_time::ptime &time,
					const Level1TickValue &tick) {
			return Base::SetLevel1(time, tick);
		}

		void AddLevel1Tick(
					const boost::posix_time::ptime &time,
					const Level1TickValue &tick) {
			return Base::AddLevel1Tick(time, tick);
		}

	};

} } }
