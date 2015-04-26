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
				// +3 - to get required book size after adjusting.
				int(GetLevelsCount()) + 3);
			request.set(
				fix::FIX42::Tags::AggregatedBook,
				fix::FIX42::Values::AggregatedBook::one_book_entry_per_side_per_price);
			request.set(fix::FIX42::Tags::DeliverToCompID, "ALL");
			request.set(
				fix::FIX43::Tags::Product,
				fix::FIX43::Values::Product::CURRENCY);
		}

		void OnMarketDataSnapshot(
				const fix::Message &message,
				const boost::posix_time::ptime &dataTime,
				FixSecurity &security) {

			FixSecurity::BookSideSnapshot book;
			
			const fix::Group &entries
				= message.getGroup(fix::FIX42::Tags::NoMDEntries);
			for (size_t i = 0; i < entries.size(); ++i) {

				const auto &entry = entries[i];

				const auto &qty = ParseMdEntrySize(entry);
				if (IsZero(qty)) {
					GetLog().Warn(
						"Price level with zero-qty received for %1%: \"%2%\".",
						security,
						entry);
					continue;
				}

				const double price
					= entry.getDouble(fix::FIX42::Tags::MDEntryPx);
				Assert(book.find(security.ScalePrice(price)) == book.end());
				book[security.ScalePrice(price)]= std::make_pair(
					message.get(fix::FIX42::Tags::MDEntryType)
						== fix::FIX42::Values::MDEntryType::Bid,
					Security::Book::Level(dataTime, price, qty));

			}

			book.swap(security.m_book);

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
