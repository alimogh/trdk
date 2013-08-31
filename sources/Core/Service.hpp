/**************************************************************************
 *   Created: 2012/11/03 00:19:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "ModuleVariant.hpp"
#include "Module.hpp"
#include "Api.h"

namespace trdk {

	class TRDK_CORE_API Service : public trdk::Module {

	public:

		typedef trdk::ModuleRefVariant Subscriber;
		typedef std::list<Subscriber> SubscriberList;

	public:

		explicit Service(
				trdk::Context &,
				const std::string &name,
				const std::string &tag);
		virtual ~Service();

	public:

		//! Notifies about new security start.
		/** @return Return desired security data start. Can be
		  * boost::posix_time::not_a_date_time.
		  */
		virtual boost::posix_time::ptime OnSecurityStart(
					const trdk::Security &);

		virtual bool OnLevel1Update(const trdk::Security &);

		virtual bool OnLevel1Tick(
					const trdk::Security &,
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &);

		virtual bool OnNewTrade(
					const trdk::Security &,
					const boost::posix_time::ptime &,
					trdk::ScaledPrice,
					trdk::Qty,
					trdk::OrderSide);

		virtual bool OnServiceDataUpdate(const trdk::Service &);
		
		//! Notifies about broker position update.
		/** @param security		Security.
		  * @param qty			Position size (may differ from current
		  *						trdk::Security::GetBrokerPosition).
		  * @param isInitial	true if it initial data at start.
		  */
		virtual bool OnBrokerPositionUpdate(
					trdk::Security &security,
					trdk::Qty qty,
					bool isInitial);

	public:

		void RegisterSource(trdk::Security &);
		const SecurityList & GetSecurities() const;

		void RegisterSubscriber(trdk::Strategy &);
		void RegisterSubscriber(trdk::Service &);
		void RegisterSubscriber(trdk::Observer &);
		const SubscriberList & GetSubscribers();

	public:

		bool RaiseLevel1UpdateEvent(const trdk::Security &);
		bool RaiseLevel1TickEvent(
					const trdk::Security &,
					const boost::posix_time::ptime &,
					const trdk::Level1TickValue &);
		bool RaiseNewTradeEvent(
					const trdk::Security &,
					const boost::posix_time::ptime &,
					trdk::ScaledPrice,
					trdk::Qty,
					trdk::OrderSide);
		bool RaiseServiceDataUpdateEvent(const trdk::Service &);
		bool RaiseBrokerPositionUpdateEvent(
					trdk::Security &security,
					trdk::Qty qty,
					bool isInitial);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}
