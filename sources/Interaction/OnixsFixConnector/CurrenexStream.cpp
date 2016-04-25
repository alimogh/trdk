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
namespace pt = boost::posix_time;

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
				terminate();
			}
		}

	protected:

		virtual void OnNewEntry(
				FixSecurity &security,
				const fix::Int64 &entryId,
				const pt::ptime &time,
				bool isBid,
				double price,
				const Qty &qty) {
			security.SetEntry(entryId, time, isBid, price, qty);
		}
		
		virtual void OnEntryReplace(
				FixSecurity &,
				const OnixS::FIX::Int64 &,
				const OnixS::FIX::Int64 &,
				const boost::posix_time::ptime &,
				bool,
				double,
				const Qty &) {
			GetLog().Error("Method \"replace entry\" is not implemented.");
			AssertFail("Method \"replace entry\" is not implemented");
			throw Error("Method \"replace entry\" is not implemented");
		}
		
		virtual void OnEntryUpdate(
				FixSecurity &,
				const fix::Int64 &,
				const pt::ptime &,
				bool,
				double,
				const Qty &) {
			GetLog().Error("Method \"update entry\" is not implemented.");
			AssertFail("Method \"update entry\" is not implemented");
			throw Error("Method \"update entry\" is not implemented");
		}
		
		virtual void OnEntryDelete(
				FixSecurity &security,
				const fix::Int64 &entryId) {
			security.OnEntryDelete(entryId);
		}

		virtual void OnLogout() {
			GetSession().ResetLocalSequenceNumbers(true, true);
		}

		virtual void OnReconnecting() {
			GetSession().ResetLocalSequenceNumbers(true, true);
		}

		virtual Qty ParseMdEntrySize(const fix::GroupInstance &entry) const {
			return entry.getDouble(fix::FIX42::Tags::MDEntrySize);
		}

		virtual Qty ParseMdEntrySize(const fix::Message &message) const {
			return message.getDouble(fix::FIX42::Tags::MDEntrySize);
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
				const pt::ptime &time,
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
			security.OnNewEntry(
				entryId,
				time,
				message.get(fix::FIX42::Tags::MDEntryType)
					== fix::FIX42::Values::MDEntryType::Bid,
				message.getDouble(fix::FIX42::Tags::MDEntryPx),
				ParseMdEntrySize(message));

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
