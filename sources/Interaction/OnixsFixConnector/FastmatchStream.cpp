/**************************************************************************
 *   Created: 2015/09/29 23:37:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "FixStream.hpp"
#include "Core/TradingLog.hpp"
#include "Core/PriceBook.hpp"

namespace fix = OnixS::FIX;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::OnixsFixConnector;

namespace trdk { namespace Interaction { namespace OnixsFixConnector {

	class FastmatchStream : public FixStream {

	public:

		explicit FastmatchStream(
				size_t index,
				Context &context,
				const std::string &tag,
				const IniSectionRef &conf)
			: FixStream(index, context, tag, conf) {
			//...//
		}

		virtual ~FastmatchStream() {
			try {
				GetSession().Disconnect();
			} catch (...) {
				AssertFailNoException();
				throw;
			}
		}

	protected:

		virtual void OnLogout() {
			//...//
		}

		virtual void OnReconnecting() {
			//...//
		}

		virtual void SetupBookRequest(
				fix::Message &request,
				const Security &)
				const {
			request.set(
				fix::FIX42::Tags::MarketDepth,
				int(PriceBook::GetSideMaxSize()));
			request.set(
				fix::FIX42::Tags::AggregatedBook,
				fix::FIX42::Values::AggregatedBook::one_book_entry_per_side_per_price);
		}

	};

} } }

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_ONIXSFIXCONNECTOR_API
boost::shared_ptr<MarketDataSource>
CreateFastmatchStream(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration) {
	return boost::shared_ptr<MarketDataSource>(
		new FastmatchStream(index, context, tag, configuration));
}

////////////////////////////////////////////////////////////////////////////////
