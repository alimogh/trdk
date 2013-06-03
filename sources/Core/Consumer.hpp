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

		virtual void OnSecurityStart(trdk::Security &);

	public:

		virtual void OnLevel1Update(trdk::Security &);
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
		virtual void OnPositionUpdate(trdk::Position &);

	public:

		void RegisterSource(trdk::Security &);

		SecurityList & GetSecurities();
		const SecurityList & GetSecurities() const;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}
