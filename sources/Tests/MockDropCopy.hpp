/**************************************************************************
 *   Created: 2016/09/10 16:22:24
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Core/DropCopy.hpp"
#include "Core/PriceBook.hpp"
#include "Core/Security.hpp"
#include "Core/Strategy.hpp"

namespace trdk { namespace Tests {

	class MockDropCopy : public trdk::DropCopy {

	public:

		MockDropCopy() {
			//...//
		}
		virtual ~MockDropCopy() {
			//...//
		}

	public:

		MOCK_METHOD0(Flush, void());
		MOCK_METHOD0(Dump, void());

		MOCK_METHOD1(
			RegisterStrategyInstance,
			StrategyInstanceId(const trdk::Strategy &));
		MOCK_METHOD2(
			ContinueStrategyInstance,
			StrategyInstanceId(
				const trdk::Strategy &,
				const boost::posix_time::ptime &));
		MOCK_METHOD3(
			RegisterDataSourceInstance,
			trdk::DropCopy::DataSourceInstanceId(
				const trdk::Strategy &,
				const boost::uuids::uuid &type,
				const boost::uuids::uuid &id));

		virtual void CopyOrder(
				const boost::uuids::uuid &,
				const std::string *,
				const boost::posix_time::ptime *,
				const boost::posix_time::ptime *,
				const trdk::OrderStatus &,
				const boost::uuids::uuid &,
				const int64_t *,
				const trdk::Security &,
				const trdk::TradingSystem &,
				const trdk::OrderSide &,
				const trdk::Qty &,
				const double *,
				const trdk::TimeInForce *,
				const trdk::Lib::Currency &,
				const trdk::Qty *,
				const trdk::Qty &,
				const double *,
				const trdk::Qty *,
				const double *,
				const trdk::Qty *) {
			//! @todo Make wrapper to struct to test this method.
			throw std::exception("Method is not mocked");
		}

		MOCK_METHOD9(
			CopyTrade,
			void(
				const boost::posix_time::ptime &,
				const std::string &tradingSystemTradeId,
				const boost::uuids::uuid &orderId,
				double price,
				const trdk::Qty &qty,
				double bestBidPrice,
				const trdk::Qty &bestBidQty,
				double bestAskPrice,
				const trdk::Qty &bestAskQty));

		MOCK_METHOD3(
			ReportOperationStart,
			void(
				const trdk::Strategy &,
				const boost::uuids::uuid &id,
				const boost::posix_time::ptime &));
		void ReportOperationEnd(
				const boost::uuids::uuid &id,
				const boost::posix_time::ptime &time,
				const trdk::OperationResult &result,
				double pnl,
				trdk::FinancialResult &&financialResult) {
			ReportOperationEnd(id, time, result, pnl, financialResult);
		}
		MOCK_METHOD5(
			ReportOperationEnd,
			void(
				const boost::uuids::uuid &id,
				const boost::posix_time::ptime &,
				const trdk::OperationResult &,
				double pnl,
				const trdk::FinancialResult &));

		MOCK_METHOD2(
			CopyBook,
			void(const trdk::Security &, const trdk::PriceBook &));
	
		MOCK_METHOD7(
			CopyBar,
			void(
				const trdk::DropCopy::DataSourceInstanceId &,
				size_t index,
				const boost::posix_time::ptime &,
				double,
				double,
				double,
				double));
		MOCK_METHOD4(
			CopyAbstractData,
			void(
				const trdk::DropCopy::DataSourceInstanceId &,
				size_t index,
				const boost::posix_time::ptime &,
				double value));

	};

} }
