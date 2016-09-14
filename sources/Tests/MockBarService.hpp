/**************************************************************************
 *   Created: 2016/09/12 01:23:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Services/BarService.hpp"

namespace trdk { namespace Tests {

	class MockBarService : public trdk::Services::BarService {

	public:

		MockBarService()
			: BarService(
				*static_cast<Context *>(nullptr),
				"Mock",
				trdk::Lib::IniSectionRef(
					trdk::Lib::IniString(
						"[Section]\nsize = 10 ticks\nlog = none"),
					"Section")) {
			//...//
		}

	public:

		MOCK_CONST_METHOD0(GetLastBar, const Bar &());
		MOCK_CONST_METHOD0(GetSecurity, const trdk::Security &());

	};

} }
