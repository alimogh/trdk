/**************************************************************************
 *   Created: 2015/04/16 01:38:24
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "FixStream.hpp"
#include "Core/PriceBook.hpp"
#include "Core/TradingLog.hpp"

namespace fix = OnixS::FIX;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::OnixsFixConnector;

namespace trdk { namespace Interaction { namespace OnixsFixConnector {

	class IntegralStream : public FixStream {

	public:

		explicit IntegralStream(
				size_t index,
				Context &context,
				const std::string &tag,
				const IniSectionRef &conf)
			: FixStream(index, context, tag, conf) {
			//...//
		}

		virtual ~IntegralStream() {
			try {
				GetSession().Disconnect();
			} catch (...) {
				AssertFailNoException();
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
				fix::FIX42::Tags::MDUpdateType,
				fix::FIX42::Values::MDUpdateType::Full_Refresh);
			request.set(
				fix::FIX42::Tags::MarketDepth,
				int(PriceBook::GetSideMaxSize()));
			request.set(
				fix::FIX42::Tags::AggregatedBook,
				fix::FIX42::Values::AggregatedBook::one_book_entry_per_side_per_price);
			request.set(fix::FIX42::Tags::DeliverToCompID, "ALL");
			request.set(
				fix::FIX43::Tags::Product,
				fix::FIX43::Values::Product::CURRENCY);
		}

	};

} } }

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_ONIXSFIXCONNECTOR_API
boost::shared_ptr<MarketDataSource>
CreateIntegralStream(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration) {
	return boost::shared_ptr<MarketDataSource>(
		new IntegralStream(index, context, tag, configuration));
}

////////////////////////////////////////////////////////////////////////////////
