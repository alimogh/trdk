/**************************************************************************
 *   Created: 2014/09/15 20:21:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Foreach.hpp"

namespace trdk { namespace Lib { namespace Detail {

	template<typename Str, typename Stream>
	void DumpMultiLineString(const Str &str, Stream &stream) {
		foreach (char ch, str) {
			if (ch == 0) {
				break;
			}
			stream << ch;
			if (ch == stream.widen('\n')) {
				stream << "\t\t\t\t\t\t";
			}
		}
		stream << std::endl;
	}

} } }
