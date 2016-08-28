/**************************************************************************
 *   Created: 2016/08/25 18:06:18
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Security.hpp"

namespace trdk { namespace Interaction { namespace DdfPlus {

	class Security : public trdk::Security {

	public:

		typedef trdk::Security Base;
	
	public:

		explicit Security(
				Context &,
				const Lib::Symbol &,
				const MarketDataSource &);

	public:

		using Base::SetExpiration;
		using Base::IsLevel1Required;

	public:

		std::string GenerateDdfPlusCode() const;

	};

} } }
