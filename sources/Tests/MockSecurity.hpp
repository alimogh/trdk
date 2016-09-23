/**************************************************************************
 *   Created: 2016/09/12 01:39:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "DummyMarketDataSource.hpp"
#include "DummyContext.hpp"
#include "Core/Security.hpp"

namespace trdk { namespace Tests {

	class MockSecurity : public trdk::Security {

	public:

		MockSecurity()
			: Security(
				DummyContext::GetInstance(),
				trdk::Lib::Symbol("TEST_SCALE2*/USD::FUT"),
				DummyMarketDataSource::GetInstance(),
				SupportedLevel1Types()) {
			//...//
		}

	public:

		MOCK_CONST_METHOD0(IsOnline, bool());
		MOCK_CONST_METHOD0(GetExpiration, trdk::Lib::ContractExpiration());

	};

} }
