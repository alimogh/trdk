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
				const trdk::ScaledPrice &,
				const trdk::Qty &,
				const trdk::OrderSide &);
		bool RaiseServiceDataUpdateEvent(
				const trdk::Service &,
				const trdk::Lib::TimeMeasurement::Milestones &);
		bool RaiseBrokerPositionUpdateEvent(
				trdk::Security &security,
				const trdk::Qty &qty,
				bool isInitial);
		bool RaiseNewBarEvent(
				const trdk::Security &,
				const trdk::Security::Bar &);
		bool RaiseBookUpdateTickEvent(
				const trdk::Security &,
				const trdk::Security::Book &,
				const trdk::Lib::TimeMeasurement::Milestones &);

	protected:

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
				const trdk::ScaledPrice &,
				const trdk::Qty &,
				const trdk::OrderSide &);

		virtual bool OnServiceDataUpdate(
				const trdk::Service &,
				const trdk::Lib::TimeMeasurement::Milestones &);
		
		//! Notifies about broker position update.
		/** @param security		Security.
		  * @param qty			Position size (may differ from current
		  *						trdk::Security::GetBrokerPosition).
		  * @param isInitial	true if it initial data at start.
		  */
		virtual bool OnBrokerPositionUpdate(
				trdk::Security &,
				const trdk::Qty &,
				bool isInitial);

		virtual bool OnNewBar(
				const trdk::Security &,
				const trdk::Security::Bar &);

		virtual bool OnBookUpdateTick(
				const trdk::Security &,
				const trdk::Security::Book &,
				const trdk::Lib::TimeMeasurement::Milestones &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}
