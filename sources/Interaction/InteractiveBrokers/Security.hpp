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

		virtual ~Security() {
			//...//
		}

	public:

		virtual bool IsLevel1Required() const;

		bool IsTradesRequired() const  {
			return Base::IsTradesRequired();
		}

		//! Sets last trade price.
		/** @return	"true" if values change and all subscribers will be
		  *			notified.
		  */
		bool SetLastPrice(double val) {
			return Base::SetLastPrice(val);
		}
		//! Sets last trade size.
		/** @return	"true" if values change and all subscribers will be
		  *			notified.
		  */
		bool SetLastQty(trdk::Qty val) {
			return Base::SetLastQty(val);
		}

		//! Sets traded volume.
		/** @return	"true" if values change and all subscribers will be
		  *			notified.
		  */
		bool SetVolume(trdk::Qty val) {
			return Base::SetVolume(val);
		}

		//! Sets bid price only.
		/** @return	"true" if values change and all subscribers will be
		  *			notified.
		  */
		bool SetBidPrice(double val) {
			return Base::SetBidPrice(val);
		}
		//! Sets bid size only.
		/** @return	"true" if values change and all subscribers will be
		  *			notified.
		  */
		bool SetBidQty(trdk::Qty val) {
			return Base::SetBidQty(val);
		}
		//! Sets ask price only.
		/** @return	"true" if values change and all subscribers will be
		  *			notified.
		  */
		bool SetAskPrice(double val) {
			return Base::SetAskPrice(val);
		}
		//! Sets ask size only.
		/** @return	"true" if values change and all subscribers will be
		  *			notified.
		  */
		bool SetAskQty(trdk::Qty qty) {
			return Base::SetAskQty(qty);
		}

	};

} } }
