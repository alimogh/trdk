/**************************************************************************
 *   Created: 2015/04/20 23:40:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "FixTrading.hpp"
#include "Core/Security.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::OnixsFixConnector;

namespace fix = OnixS::FIX;

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Interaction { namespace OnixsFixConnector {

	class IntegralTrading : public FixTrading {

	public:

		typedef FixTrading Base;

	public:

		explicit IntegralTrading(
				const TradingMode &mode,
				size_t index,
				Context &context,
				const std::string &tag,
				const Lib::IniSectionRef &conf)
			: FixTrading(mode, index, context, tag, conf) {
			//...//
		}
		
		virtual ~IntegralTrading() {
			try {
				GetSession().Disconnect();
			} catch (...) {
				AssertFailNoException();
				throw;
			}
		}

	public:

		virtual void onInboundApplicationMsg(
				fix::Message &message,
				fix::Session *session) {
			const auto &replyTime = TimeMeasurement::Milestones::GetNow();
			Assert(session == &GetSession().Get());
			UseUnused(session);
			try {
				if (message.type() != "8") {
					return;
				}
				OnExecutionReport(message, replyTime);
			} catch (const std::exception &ex) {
				GetLog().Error(
					"Fatal error"
						" in the processing of incoming application messages"
						": \"%1%\".",
					ex.what());
				throw;
			} catch (...) {
				AssertFailNoException();
				throw;
			}
		}

		virtual void onInboundSessionMsg(
				fix::Message &message,
				fix::Session *session) {
			Assert(session == &GetSession().Get());
			UseUnused(session);
			try {
				if (message.type() != "3") {
					return;
				}
				OnReject(message);
			} catch (const std::exception &ex) {
				GetLog().Error(
					"Fatal error"
						" in the processing of incoming session messages"
						": \"%1%\".",
					ex.what());
				throw;
			} catch (...) {
				AssertFailNoException();
				throw;
			}
		}

	protected:

		virtual fix::Message CreateOrderMessage(
				const std::string &clOrderId,
				const Security &security,
				const Currency &currency,
				const Qty &qty,
				const trdk::OrderParams &params) {
			auto result = Base::CreateOrderMessage(
				clOrderId,
				security,
				currency,
				qty,
				params);
			result.set(
				fix::FIX43::Tags::Product,
				fix::FIX43::Values::Product::CURRENCY);
			//! @sa TRDK-226:
 			result.set(fix::FIX40::Tags::ExecInst, "B");
			result.set(
				fix::FIX41::Tags::SecurityType,
				fix::FIX41::Values::SecurityType::Foreign_Exchange_Contract);
			return result;
		}

		virtual fix::Message CreateMarketOrderMessage(
				const std::string &clOrderId,
				const Security &security,
				const Currency &currency,
				const Qty &qty,
				const trdk::OrderParams &params) {
			fix::Message order = CreateOrderMessage(
				clOrderId,
				security,
				currency,
				qty,
				params);
			order.set(
				fix::FIX40::Tags::OrdType,
				fix::FIX40::Values::OrdType::Market);
			return order;
		}

		virtual fix::Message CreateLimitOrderMessage(
				const std::string &clOrderId,
				const Security &security,
				const Currency &currency,
				const Qty &qty,
				const ScaledPrice &price,
				const trdk::OrderParams &params) {
			fix::Message order = CreateOrderMessage(
				clOrderId,
				security,
				currency,
				qty,
				params);
			order.set(
				fix::FIX40::Tags::OrdType,
				fix::FIX41::Values::OrdType::Limit);
			order.set(
				fix::FIX40::Tags::Price,
				security.DescalePrice(price),
				security.GetPricePrecision());
			return order;
		}

	protected:

		virtual Qty ParseLastShares(const fix::Message &message) const {
			return message.contain(fix::FIX43::Tags::LastQty)
				?	Qty(message.getDouble(fix::FIX43::Tags::LastQty))
				:	Qty(0);
		}

		virtual Qty ParseLeavesQty(const fix::Message &message) const {
			return Qty(message.getDouble(fix::FIX41::Tags::LeavesQty));
		}

		virtual void OnLogout() {
			//...//
		}

	private:

		void OnExecutionReport(
				fix::Message &message,
				const Lib::TimeMeasurement::Milestones::TimePoint &replyTime) {

			Assert(message.type() == "8");

			const auto &execType = message.get(fix::FIX41::Tags::ExecType);
			const auto &ordStatus = message.get(fix::FIX40::Tags::OrdStatus);
		
			if (
					execType == fix::FIX41::Values::ExecType::New
					|| execType == fix::FIX41::Values::ExecType::Replace) {
			
				if (ordStatus == fix::FIX40::Values::OrdStatus::New) {
					OnOrderNew(message, replyTime);
					return;
				}

			} else if (execType == fix::FIX41::Values::ExecType::Cancelled) {

				if (ordStatus == fix::FIX40::Values::OrdStatus::Canceled) {
					OnOrderCanceled(
						message,
						GetMessageOrigClOrderId(message),
						replyTime);
					return;
				}

			} else if (execType == fix::FIX43::Values::ExecType::Trade) {

				if (ordStatus == fix::FIX40::Values::OrdStatus::Partially_filled) {
					OnOrderPartialFill(message, replyTime);
					return;
				} else if (ordStatus == fix::FIX40::Values::OrdStatus::Filled) {
					OnOrderFill(message, replyTime);
					return;
				}

			}

			GetLog().Error(
				"Unknown Execution Report received: \"%1%\".",
				boost::cref(message));

		}

		void OnReject(fix::Message &message) {
			GetLog().Error("Unknown Reject received: \"%1%\".", message);
		}


	};

} } }

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_ONIXSFIXCONNECTOR_API
TradeSystemFactoryResult
CreateIntegralTrading(
		const TradingMode &mode,
		size_t index,
		Context &context,
		const std::string &tag,
		const IniSectionRef &configuration) {
	TradeSystemFactoryResult result;
	boost::get<0>(result).reset(
		new IntegralTrading(mode, index, context, tag, configuration));
	return result;
}

////////////////////////////////////////////////////////////////////////////////
