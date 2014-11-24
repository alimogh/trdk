/**************************************************************************
 *   Created: 2014/10/24 01:08:13
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
#include "Core/AsyncLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::OnixsFixConnector;

namespace fix = OnixS::FIX;

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace Interaction { namespace OnixsFixConnector {

	class FxAllTrading : public FixTrading {

	public:

		typedef FixTrading Base;

	public:

		explicit FxAllTrading(
					Context &context,
					const std::string &tag,
					const Lib::IniSectionRef &conf)
				: FixTrading(context, tag, conf) {
			//...//
		}
		
		virtual ~FxAllTrading() {
			try {
				GetSession().Disconnect();
			} catch (...) {
				AssertFailNoException();
			}
		}

	public:

		virtual void onInboundApplicationMsg(
					fix::Message &message,
					fix::Session *session) {
			const auto &replyTime = TimeMeasurement::Milestones::GetNow();
			Assert(session == &GetSession().Get());
			UseUnused(session);
			if (message.type() != "8") {
				return;
			}
			OnExecutionReport(message, replyTime);
		}

		virtual void onInboundSessionMsg (
					fix::Message &message,
					fix::Session *session) {
			Assert(session == &GetSession().Get());
			UseUnused(session);
			if (message.type() != "3") {
				return;
			}
			OnReject(message);
		}

	protected:

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
			order.set(
				fix::FIX40::Tags::TimeInForce,
				fix::FIX40::Values::TimeInForce::Immediate_or_Cancel);
			return std::move(order);
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
			return std::move(order);
		}

	protected:

		virtual Qty ParseLastShares(const fix::Message &message) const {
			return message.contain(fix::FIX43::Tags::LastQty)
				?	Qty(message.getDouble(fix::FIX43::Tags::LastQty))
				:	0;
		}

		virtual Qty ParseLeavesQty(const fix::Message &message) const {
			return Qty(message.getDouble(fix::FIX41::Tags::LeavesQty));
		}

		virtual void OnOrderStateChanged(
					const fix::Message &message,
					const OrderStatus &status,
					const Order &order) {

			if (order.params.isManualOrder) {
				Base::OnOrderStateChanged(message, status, order);
				return;
			}
			
			if (
					order.type != ORDER_TYPE_DAY_MARKET
					|| status != ORDER_STATUS_CANCELLED) {
				if (
						status == ORDER_STATUS_SUBMITTED
						&& order.params.goodInSeconds) {
					GetTradingLog().Write(
						"Skipping emulated \"day-market\" order %1%"
							" \"new status\" for %2%...",
						[&](LogRecord &record) {
							record % order.id % order.security->GetSymbol();
						});
					return;
				}
				Base::OnOrderStateChanged(message, status, order);
				return;
			}

			OrderParams params(order.params);
			if (params.goodInSeconds) {
				++*params.goodInSeconds;
			} else {
				params.goodInSeconds.reset(1);
			}

			AssertLt(order.filledQty, order.qty);
			const Qty newQty = order.qty - order.filledQty;

			GetTradingLog().Write(
				"Emulating \"day-market\" order"
					" by resending order %1%"
					" for \"%2%\" with qty %3%->%4% (%5% times)...",
				[&](LogRecord &record) {
					record
						% order.id
						% order.security->GetSymbol()
						% order.qty
						% newQty
						% *params.goodInSeconds;
				});

			if (order.isSell) {
				SellAtMarketPrice(
					*order.security,
					order.currency,
					newQty,
					params,
					order.callback);
			} else {
				BuyAtMarketPrice(
					*order.security,
					order.currency,
					newQty,
					params,
					order.callback);
			}
		
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

			} else if (execType == fix::FIX41::Values::ExecType::Pending_New) {
			
				return;
		
			} else if (execType == fix::FIX42::Values::ExecType::Pending_Cancel) {
			
				return;
		
			} else if (execType == fix::FIX44::Values::ExecType::PendingReplace) {
			
				return;

			}

			GetLog().Error(
				"Unknown Execution Report received: \"%1%\".",
				boost::cref(message));

		}

		void OnReject(fix::Message &message) {
			GetLog().Error("Unknown Reject received: \"%1%\".", message);
		}


	private:

		const std::string m_account;

	};

} } }

////////////////////////////////////////////////////////////////////////////////

TRDK_INTERACTION_ONIXSFIXCONNECTOR_API
TradeSystemFactoryResult
CreateFxAllTrading(
			Context &context,
			const std::string &tag,
			const IniSectionRef &configuration) {
	TradeSystemFactoryResult result;
	boost::get<0>(result).reset(
		new FxAllTrading(context, tag, configuration));
	return result;
}

////////////////////////////////////////////////////////////////////////////////
