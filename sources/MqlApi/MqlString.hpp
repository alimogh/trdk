/**************************************************************************
 *   Created: 2014/01/19 05:22:25
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace MqlApi { namespace MqlTypes { 

	struct String {
		
		int32_t size;
		char *value;

		String & operator =(const std::string &source) {
			AssertEq(0, size);
			Assert(!value);
			value = new char[source.size() + 1];
			size = source.size();
			memcpy(value, source.c_str(), size + 1);
			return *this;
		}

	};

} } }
