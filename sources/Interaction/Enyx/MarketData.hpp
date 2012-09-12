/**************************************************************************
 *   Created: 2012/08/28 20:38:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/MarketDataSource.hpp"

class EnyxMDInterface;

namespace Trader {  namespace Interaction { namespace Enyx {

	class MarketData : public ::LiveMarketDataSource {

	public:

		MarketData();
		virtual ~MarketData();

	public:

		virtual void Connect(const IniFile &ini, const std::string &section);

	public:

		virtual void SubscribeToMarketDataLevel1(boost::shared_ptr<Security>) const;
		virtual void SubscribeToMarketDataLevel2(boost::shared_ptr<Security>) const;

	protected:

		bool IsSupported(const Security &) const;

	private:

		std::unique_ptr<EnyxMDInterface> m_enyx;

	};

} } }
