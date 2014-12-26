/**************************************************************************
 *   Created: 2013/05/14 07:19:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Module.hpp"
#include "Api.h"

namespace trdk {

	class TRDK_CORE_API Consumer : public trdk::Module {

	public:

		explicit Consumer(
				trdk::Context &,
				const std::string &typeName,
				const std::string &name,
				const std::string &tag);
		virtual ~Consumer();

	public:

		//! Notifies about new security start.
		/** @return	Return desired security data start. Can be
		  *			boost::posix_time::not_a_date_time.
		  */
		virtual boost::posix_time::ptime OnSecurityStart(trdk::Security &);

	public:
		
		virtual void OnLevel1Tick(
					trdk::Security &,
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &);
		
		virtual void OnNewTrade(
					trdk::Security &,
					const boost::posix_time::ptime &,
					trdk::ScaledPrice,
					trdk::Qty,
					trdk::OrderSide);
		
		virtual void OnServiceDataUpdate(const trdk::Service &);

		//! Notifies about broker position update.
		/** @sa trdk::Security::GetBrokerPosition
		  * @param security		Security.
		  * @param qty			Position size. Negative value means "short
		  *						position", positive - "long position". May
		  *						differ from current
		  *						trdk::Security::GetBrokerPosition.
		  * @param isInitial	true if it initial data at start.
		  */
		virtual void OnBrokerPositionUpdate(
					trdk::Security &security,
					trdk::Qty qty,
					bool isInitial);

		virtual void OnNewBar(trdk::Security &, const trdk::Security::Bar &);

	public:

		void RegisterSource(trdk::Security &);

		SecurityList & GetSecurities();
		const SecurityList & GetSecurities() const;

	public:

		void RaiseBrokerPositionUpdateEvent(
				trdk::Security &,
				trdk::Qty,
				bool isInitial);

		void RaiseNewBarEvent(trdk::Security &, const trdk::Security::Bar &);

		void RaiseBookUpdateTickEvent(
				trdk::Security &,
				const trdk::Security::Book &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}
