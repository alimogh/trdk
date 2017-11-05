/*******************************************************************************
 *   Created: 2017/09/11 08:47:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "MrigeshKejriwalPositionReport.hpp"
#include "Core/Position.hpp"
#include "MrigeshKejriwalPositionOperationContext.hpp"
#include "Settings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::MrigeshKejriwal;
namespace mk = trdk::Strategies::MrigeshKejriwal;

////////////////////////////////////////////////////////////////////////////////

namespace {
class PositionReportCalculator : private boost::noncopyable {
 public:
  explicit PositionReportCalculator(const Position &position,
                                    const mk::Settings &settings)
      : m_position(position),
        m_operationContext(
            *boost::polymorphic_downcast<const mk::PositionOperationContext *>(
                &m_position.GetOperationContext())),
        m_settings(settings) {}
  virtual ~PositionReportCalculator() = default;

 public:
  //!  Slippage is the difference between the signal price and the actual order
  // execution price multiplied by the quantity.
  virtual Double CalcOpenSlippage() const = 0;
  //!  Slippage is the difference between the signal price and the actual order
  // execution price multiplied by the quantity.
  virtual Double CalcCloseSlippage() const = 0;
  //!  (op_average_price / op_control_price - cl_average_price /
  //!  cl_control_price) for long and -(op_average_price / op_control_price -
  //!  cl_average_price / cl_control_price) for short positions
  /** https://trello.com/c/PU46S5P4
    */
  virtual Double CalcSlippagePercent() const = 0;

  //! SEBI Turnover fees(0.0002% of the trade value)
  /** https://trello.com/c/PU46S5P4
    */
  Double CalcSebiTurnoverFees() const {
    return (m_position.GetOpenedVolume() *
            m_settings.report.sebiTurnoverFeesRatio) +
           (m_position.GetClosedVolume() *
            m_settings.report.sebiTurnoverFeesRatio);
  }
  //! Stamp duty(0.002% of the trade value)
  /** https://trello.com/c/PU46S5P4
    */
  Double CalcStampDuty() const {
    return (m_position.GetOpenedVolume() * m_settings.report.stampDutyRatio) +
           (m_position.GetClosedVolume() * m_settings.report.stampDutyRatio);
  }
  //! Goods and Services Tax(18 % of broker commission)
  /** https://trello.com/c/PU46S5P4
    */
  Double CalcGoodsAndServicesTax() const {
    return (m_position.CalcCommission() + CaclExchangeTransactionCharges()) *
           m_settings.report.goodsAndServicesTaxRatio;
  }
  //! Securities Transaction Tax(0.1% of the trade value only for sale)
  /** https://trello.com/c/PU46S5P4
    */
  virtual Double CalcSecuritiesTransactionTax() const = 0;

  //! 1. Gross_PnL% = (cl_control_price/op_control_price-1) for long and
  //! -(cl_control_price/op_control_price-1) for short positions
  /** https://trello.com/c/PU46S5P4
    */
  virtual Double CalcGrossPnl() const = 0;
  //! Tran_costs% = (Commission + taxes) / op_volume
  /** https://trello.com/c/PU46S5P4
    */
  Double CalcTranCosts() const {
    AssertNe(0, m_position.GetOpenedVolume());
    if (m_position.GetOpenedVolume() == 0) {
      return 0;
    }
    return (m_position.CalcCommission() + CalcSebiTurnoverFees() +
            CalcStampDuty() + CalcGoodsAndServicesTax() +
            CalcSecuritiesTransactionTax()) /
           m_position.GetOpenedVolume();
  }
  //! Net_PnL% = GrossPnL%-Slippage%-Tran_costs%
  /** https://trello.com/c/PU46S5P4
    */
  Double CalcNetPnl() const {
    return CalcGrossPnl() - CalcSlippagePercent() - CalcTranCosts();
  }

  Double GetInitialMargin() const {
    // Will be customized in the custom branch.
    return m_settings.report.initialMargin;
  }

  //! Leverage = op_volume / initial_margin
  /** https://trello.com/c/PU46S5P4
    */
  Double CalcLeverage() const {
    const auto &initialMargin = GetInitialMargin();
    if (initialMargin == 0) {
      return 0;
    }
    return m_position.GetOpenedVolume() / initialMargin;
  }
  //! Leveraged_Net_PnL = Net_PnL%*Leverage
  /** https://trello.com/c/PU46S5P4
    */
  Double CalcLeveragedNetPnl() const { return CalcNetPnl() * CalcLeverage(); }

  //! Exchange Transaction Charges.
  /** https://www.interactivebrokers.co.in/en/index.php?f=1363
    */
  Double CaclExchangeTransactionCharges() const {
    return (m_position.GetOpenedVolume() *
            m_settings.report.exchangeTransactionChargesRatio) +
           (m_position.GetClosedVolume() *
            m_settings.report.exchangeTransactionChargesRatio);
  }

 protected:
  const Position &m_position;
  const mk::PositionOperationContext &m_operationContext;
  const mk::Settings &m_settings;
};
class LongPositionReportCalculator : public PositionReportCalculator {
 public:
  explicit LongPositionReportCalculator(const LongPosition &position,
                                        const mk::Settings &settings)
      : PositionReportCalculator(position, settings) {}
  virtual ~LongPositionReportCalculator() override = default;

 public:
  virtual Double CalcOpenSlippage() const override {
    return (m_position.GetOpenAvgPrice() -
            m_operationContext.GetOpenSignalPrice()) *
           m_position.GetOpenedQty();
  }
  virtual Double CalcCloseSlippage() const override {
    return (m_operationContext.GetCloseSignalPrice() -
            m_position.GetCloseAvgPrice()) *
           m_position.GetOpenedQty();
  }
  virtual Double CalcSlippagePercent() const override {
    AssertNe(0, m_operationContext.GetOpenSignalPrice());
    AssertNe(0, m_operationContext.GetCloseSignalPrice());
    if (m_operationContext.GetOpenSignalPrice() == 0 ||
        m_operationContext.GetCloseSignalPrice() == 0) {
      return 0;
    }
    return m_position.GetOpenAvgPrice() /
               m_operationContext.GetOpenSignalPrice() -
           m_position.GetCloseAvgPrice() /
               m_operationContext.GetCloseSignalPrice();
  }

  virtual Double CalcSecuritiesTransactionTax() const override {
    return m_position.GetClosedVolume() *
           m_settings.report.securitiesTransactionTaxRatio;
  }

  virtual Double CalcGrossPnl() const override {
    AssertNe(0, m_position.GetOpenAvgPrice());
    if (m_position.GetOpenAvgPrice() == 0) {
      return 0;
    }
    return m_operationContext.GetCloseSignalPrice() /
               m_position.GetOpenAvgPrice() -
           1;
  }
};
class ShortPositionReportCalculator : public PositionReportCalculator {
 public:
  explicit ShortPositionReportCalculator(const ShortPosition &position,
                                         const mk::Settings &settings)
      : PositionReportCalculator(position, settings) {}
  virtual ~ShortPositionReportCalculator() override = default;

 public:
  virtual Double CalcOpenSlippage() const override {
    return (m_operationContext.GetOpenSignalPrice() -
            m_position.GetOpenAvgPrice()) *
           m_position.GetOpenedQty();
  }
  virtual Double CalcCloseSlippage() const override {
    return (m_position.GetCloseAvgPrice() -
            m_operationContext.GetCloseSignalPrice()) *
           m_position.GetOpenedQty();
  }
  virtual Double CalcSlippagePercent() const override {
    AssertNe(0, m_operationContext.GetOpenSignalPrice());
    AssertNe(0, m_operationContext.GetCloseSignalPrice());
    if (m_operationContext.GetOpenSignalPrice() == 0 ||
        m_operationContext.GetCloseSignalPrice() == 0) {
      return 0;
    }
    return -(m_position.GetOpenAvgPrice() /
                 m_operationContext.GetOpenSignalPrice() -
             m_position.GetCloseAvgPrice() /
                 m_operationContext.GetCloseSignalPrice());
  }

  virtual Double CalcSecuritiesTransactionTax() const override {
    return m_position.GetOpenedVolume() *
           m_settings.report.securitiesTransactionTaxRatio;
  }

  virtual Double CalcGrossPnl() const override {
    AssertNe(0, m_position.GetOpenAvgPrice());
    if (m_position.GetOpenAvgPrice() == 0) {
      return false;
    }
    return -(m_operationContext.GetCloseSignalPrice() /
                 m_position.GetOpenAvgPrice() -
             1);
  }
};
}

////////////////////////////////////////////////////////////////////////////////

PositionReport::PositionReport(const Strategy &strategy,
                               const mk::Settings &settings)
    : Base(strategy), m_settings(settings) {}

void PositionReport::PrintHead(std::ostream &os) {
  os << "Date";                           // 1
  os << ",Open Start Time";               // 2
  os << ",Open Time";                     // 3
  os << ",Opening Duration";              // 4
  os << ",Close Start Time";              // 5
  os << ",Close Time";                    // 6
  os << ",Closing Duration";              // 7
  os << ",Position Duration";             // 8
  os << ",Type";                          // 9
  os << ",P&L Volume";                    // 10
  os << ",P&L %";                         // 11
  os << ",Open Slippage";                 // 12
  os << ",Close Slippage";                // 13
  os << ",Slippage %";                    // 14
  os << ",Exchange Transaction Charges";  // 15
  os << ",SEBI Turnover Fees";            // 16
  os << ",Stamp Duty";                    // 17
  os << ",Goods and Services Tax";        // 18
  os << ",Securities Transaction Tax";    // 19
  os << ",Gross P&L";                     // 20
  os << ",Tran Costs";                    // 21
  os << ",Initial margin";                // 22
  os << ",Net P&L";                       // 23
  os << ",Leverage";                      // 24
  os << ",Leveraged Net P&L";             // 25
  os << ",Is Profit";                     // 26
  os << ",Is Loss";                       // 27
  os << ",Open Signal Price";             // 28
  os << ",Close Signal Price";            // 29
  os << ",Commission";                    // 30
  os << ",Qty";                           // 31
  os << ",Open Price";                    // 32
  os << ",Open Orders";                   // 33
  os << ",Open Trades";                   // 34
  os << ",Close Reason";                  // 35
  os << ",Close Price";                   // 36
  os << ",Close Orders";                  // 37
  os << ",Close Trades";                  // 38
  os << ",ID";                            // 39
  os << std::endl;
}

void PositionReport::PrintReport(const Position &pos, std::ostream &os) {
  const auto &operationContext =
      *boost::polymorphic_downcast<const PositionOperationContext *>(
          &pos.GetOperationContext());
  std::unique_ptr<PositionReportCalculator> calculator;
  if (pos.IsLong()) {
    calculator = boost::make_unique<LongPositionReportCalculator>(
        *boost::polymorphic_downcast<const LongPosition *>(&pos), m_settings);
  } else {
    calculator = boost::make_unique<ShortPositionReportCalculator>(
        *boost::polymorphic_downcast<const ShortPosition *>(&pos), m_settings);
  }

  os << pos.GetOpenStartTime().date();                                      // 1
  os << ',' << ExcelTextField(pos.GetOpenStartTime().time_of_day());        // 2
  os << ',' << ExcelTextField(pos.GetOpenTime().time_of_day());             // 3
  os << ',' << ExcelTextField(pos.GetOpenTime() - pos.GetOpenStartTime());  // 4
  os << ',' << ExcelTextField(pos.GetCloseStartTime().time_of_day());       // 5
  os << ',' << ExcelTextField(pos.GetCloseTime().time_of_day());            // 6
  os << ','
     << ExcelTextField(pos.GetCloseTime() - pos.GetCloseStartTime());   // 7
  os << ',' << ExcelTextField(pos.GetCloseTime() - pos.GetOpenTime());  // 8
  os << ',' << pos.GetType();                                           // 9
  os << ',' << pos.GetRealizedPnlVolume();                              // 10
  os << ',' << pos.GetRealizedPnlRatio();                               // 11
  os << ',' << calculator->CalcOpenSlippage();                          // 12
  if (pos.GetCloseReason() == CLOSE_REASON_SIGNAL) {
    os << ',' << calculator->CalcCloseSlippage();    // 13
    os << ',' << calculator->CalcSlippagePercent();  // 14
  } else {
    AssertNe(CLOSE_REASON_NONE, pos.GetCloseReason());
    os << ",,";  // 13, 14
  }
  os << ',' << calculator->CalcSecuritiesTransactionTax();  // 15
  os << ',' << calculator->CalcSebiTurnoverFees();          // 16
  os << ',' << calculator->CalcStampDuty();                 // 17
  os << ',' << calculator->CalcGoodsAndServicesTax();       // 18
  os << ',' << calculator->CalcSecuritiesTransactionTax();  // 19
  if (pos.GetCloseReason() == CLOSE_REASON_SIGNAL) {
    os << ',' << calculator->CalcGrossPnl();  // 20
  } else {
    os << ',';  // 20
  }
  os << ',' << calculator->CalcTranCosts();     // 21
  os << ',' << calculator->GetInitialMargin();  // 22
  if (pos.GetCloseReason() == CLOSE_REASON_SIGNAL) {
    os << ',' << calculator->CalcNetPnl();           // 23
    os << ',' << calculator->CalcLeverage();         // 24
    os << ',' << calculator->CalcLeveragedNetPnl();  // 25
  } else {
    os << ",,,";  // 23, 24, 25
  }
  os << (pos.IsProfit() ? ",1,0" : ",0,1");            // 26, 27
  os << ',' << operationContext.GetOpenSignalPrice();  // 28
  if (pos.GetCloseReason() == CLOSE_REASON_SIGNAL) {
    os << ',' << operationContext.GetCloseSignalPrice();  // 29
  } else {
    AssertNe(CLOSE_REASON_NONE, pos.GetCloseReason());
    os << ',';  // 29
  }
  os << ',' << pos.CalcCommission();          // 30
  os << ',' << pos.GetOpenedQty();            // 31
  os << ',' << pos.GetOpenAvgPrice();         // 32
  os << ',' << pos.GetNumberOfOpenOrders();   // 33
  os << ',' << pos.GetNumberOfOpenTrades();   // 34
  os << ',' << pos.GetCloseReason();          // 35
  os << ',' << pos.GetCloseAvgPrice();        // 36
  os << ',' << pos.GetNumberOfCloseOrders();  // 37
  os << ',' << pos.GetNumberOfCloseTrades();  // 38
  os << ',' << pos.GetId();                   // 39
  os << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
