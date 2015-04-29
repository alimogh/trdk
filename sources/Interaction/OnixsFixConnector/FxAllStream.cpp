/**************************************************************************
 *   Created: 2014/10/27 20:28:26
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

	class FxAllStream : public FixStream {

	public:

		explicit FxAllStream(
				size_t index,
				Context &context,
				const std::string &tag,
				const IniSectionRef &conf)
			: FixStream(index, context, tag, conf) {
			//...//
		}

		virtual ~FxAllStream() {
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

		virtual void SetupBookRequest(
				fix::Message &request,
				const Security &) const {
			request.set(
				fix::FIX42::Tags::MarketDepth,
				// +3 - to get required book size after adjusting.
				int(GetLevelsCount()) + 3);
		}

		virtual void OnMarketDataSnapshot(
				const fix::Message &message,
				const boost::posix_time::ptime &dataTime,
				FixSecurity &security) {
		
			const fix::Group &entries
				= message.getGroup(fix::FIX42::Tags::NoMDEntries);
			if (entries.size() != 1) {
				GetLog().Error(
					"Warning Market Data Snapshot size for %1%: %2% (%3%).",
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
CreateFxAllStream(
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration) {
	return boost::shared_ptr<MarketDataSource>(
		new FxAllStream(index, context, tag, configuration));
}

////////////////////////////////////////////////////////////////////////////////
