/**************************************************************************
 *   Created: 2014/11/03 13:53:42
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

	class AlpariStream : public FixStream {

	public:

		explicit AlpariStream(
				size_t index,
				Context &context,
				const std::string &tag,
				const IniSectionRef &conf)
			: FixStream(index, context, tag, conf) {
			//...//
		}

		virtual ~AlpariStream() {
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
			// @todo Fix me!
		}

		virtual Qty ParseMdEntrySize(const fix::GroupInstance &entry) const {
			return entry.getInt32(fix::FIX42::Tags::MDEntrySize);
		}

		virtual Qty ParseMdEntrySize(const fix::Message &message) const {
			return message.getInt32(fix::FIX42::Tags::MDEntrySize);
		}

		virtual void SetupBookRequest(
				fix::Message &request,
				const Security &) const {
			request.set(
				fix::FIX42::Tags::MarketDepth,
				fix::FIX42::Values::MarketDepth::Full_Book);
			request.set(
				fix::FIX42::Tags::AggregatedBook,
				fix::FIX42::Values::AggregatedBook::one_book_entry_per_side_per_price);
		}

		virtual void OnMarketDataSnapshot(
				const fix::Message &message,
				const boost::posix_time::ptime &dataTime,
				FixSecurity &security) {
		
			const fix::Group &entries
				= message.getGroup(fix::FIX42::Tags::NoMDEntries);
			if (entries.size() != 1) {
				GetLog().Error(
					"Wring Market Data Snapshot size for %1%: %2% (%3%).",
					security,
					entries.size(),
					message);
			}
			AssertEq(1, entries.size());
		
			const auto entryId = message.getInt64(fix::FIX42::Tags::MDEntryID);
			Assert(security.m_book.find(entryId) == security.m_book.end());
		
			security.m_book[entryId] = std::make_pair(
				message.get(fix::FIX42::Tags::MDEntryType)
					== fix::FIX42::Values::MDEntryType::Bid,
				Security::Book::Level(
					dataTime,
					message.getDouble(fix::FIX42::Tags::MDEntryPx),
					ParseMdEntrySize(message)));

		}

	};

} } }

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_ONIXSFIXCONNECTOR_API
boost::shared_ptr<MarketDataSource>
CreateAlpariStream(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration) {
	return boost::shared_ptr<MarketDataSource>(
		new AlpariStream(index, context, tag, configuration));
}

////////////////////////////////////////////////////////////////////////////////
