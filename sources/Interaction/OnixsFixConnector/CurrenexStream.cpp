/**************************************************************************
 *   Created: 2014/10/27 20:28:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "FixStream.hpp"

namespace fix = OnixS::FIX;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::OnixsFixConnector;

namespace trdk { namespace Interaction { namespace OnixsFixConnector {

	class CurrenexStream : public FixStream {

	public:

		explicit CurrenexStream(
				size_t index,
				Context &context,
				const std::string &tag,
				const IniSectionRef &conf)
			: FixStream(index, context, tag, conf) {
			//...//
		}

		virtual ~CurrenexStream() {
			try {
				GetSession().Disconnect();
			} catch (...) {
				AssertFailNoException();
			}
		}

	protected:

		virtual void OnLogout() {
			GetSession().ResetLocalSequenceNumbers(true, true);
		}

		virtual void OnReconnecting() {
			GetSession().ResetLocalSequenceNumbers(true, true);
		}

		virtual void SetupBookRequest(fix::Message &request) const {
			request.set(
				fix::FIX42::Tags::MarketDepth,
				fix::FIX42::Values::MarketDepth::Full_Book);
			request.set(
				fix::FIX42::Tags::AggregatedBook,
				fix::FIX42::Values::AggregatedBook::one_book_entry_per_side_per_price);
		}

	};

} } }

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_ONIXSFIXCONNECTOR_API
boost::shared_ptr<MarketDataSource>
CreateCurrenexStream(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration) {
	return boost::shared_ptr<MarketDataSource>(
		new CurrenexStream(index, context, tag, configuration));
}

////////////////////////////////////////////////////////////////////////////////