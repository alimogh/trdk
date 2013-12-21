/**************************************************************************
 *   Created: 2013/12/22 01:41:21
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Strategy.hpp"

namespace trdk { namespace MqlApi {

	class BridgeStrategy : public trdk::Strategy {

	public:

		explicit BridgeStrategy(trdk::Context &, const std::string &tag);
		virtual ~BridgeStrategy();

	public:

		OrderId OpenLongPosition(Qty qty, double price);
		OrderId OpenShortPosition(Qty qty, double price);
		void CloseAllPositions();

	public:

		virtual std::string GetRequiredSuppliers() const;

	public:
	
		virtual void ReportDecision(const trdk::Position &) const;

	protected:

		virtual std::auto_ptr<PositionReporter> CreatePositionReporter() const;

		virtual void UpdateAlogImplSettings(const trdk::Lib::IniSectionRef &);

	};

} }
