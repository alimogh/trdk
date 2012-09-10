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
		typedef double Price;

		enum BuySellIndicator {
			BUY_SELL_INDICATOR_BUY			= 'B',
			BUY_SELL_INDICATOR_SELL_LONG	= 'S',
			BUY_SELL_INDICATOR_SELL_SHORT	= 'T'
		};

	public:

		class Error : public Exception {
		public:
			explicit Error(const char *what) throw()
					: Exception(what) {
				//...//
			}
			virtual ~Error() throw() {
				//...//
			}
		public:
			virtual const std::string & GetSubject() const = 0;
		};

	};

} } }
