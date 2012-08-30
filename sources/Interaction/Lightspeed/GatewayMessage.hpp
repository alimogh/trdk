/**************************************************************************
 *   Created: 2012/08/30 00:52:30
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

namespace Trader {  namespace Interaction { namespace Lightspeed {

	class GatewayMessage {

	public:

		typedef size_t Len;
		typedef size_t FieldStart;
		typedef int64_t Numeric;
		typedef char Char;

		class Error : public Exception {
		public:
			explicit Error(const char *what)
					: Exception(what) {
				//...//
			}
			virtual ~Error() {
				//...//
			}
		public:
			virtual const std::string & GetSubject() const = 0;
		};

	};

} } }
