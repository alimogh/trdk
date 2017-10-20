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
  //!  Leakage is the difference between the signal price and the actual order
  // execution price multiplied by the quantity.
  virtual Double CalcOpenLeakage() const = 0;
  //!  Leakage is the difference between the signal price and the actual order
  // execution price multiplied by the quantity.
  virtual Double CalcCloseLeakage() const = 0;
  //!  (op_average_price / op_control_price - cl_average_price /
  //!  cl_control_price) for long and -(op_average_price / op_control_price -
  //!  cl_average_price / cl_control_price) for short positions
  /** https://trello.com/c/PU46S5P4
    */
  virtual Double CalcLeakagePercent() const = 0;

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
    return m_position.CalcCommission() *
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
  //! Net_PnL% = GrossPnL%-Leakage%-Tran_costs%
  /** https://trello.com/c/PU46S5P4
    */
  Double CalcNetPnl() const {
    return CalcGrossPnl() - CalcLeakagePercent() - CalcTranCosts();
  }
  //! Leverage = op_volume / initial_margin
  /** https://trello.com/c/PU46S5P4
    */
  Double CalcLeverage() const {
    if (m_settings.report.initialMargin == 0) {
      return 0;
    }
    return m_position.GetOpenedVolume() / m_settings.report.initialMargin;
  }
  //! Leveraged_Net_PnL = Net_PnL%*Leverage
  /** https://trello.com/c/PU46S5P4
    */
  Double CalcLeveragedNetPnl() const { return CalcNetPnl() * CalcLeverage(); }

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
  virtual Double CalcOpenLeakage() const override {
    return (m_position.GetOpenAvgPrice() -
            m_operationContext.GetOpenSignalPrice()) *
           m_position.GetOpenedQty();
  }
  virtual Double CalcCloseLeakage() const override {
    return (m_operationContext.GetCloseSignalPrice() -
            m_position.GetCloseAvgPrice()) *
           m_position.GetOpenedQty();
  }
  virtual Double CalcLeakagePercent() const override {
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
  virtual Double CalcOpenLeakage() const override {
    return (m_operationContext.GetOpenSignalPrice() -
            m_position.GetOpenAvgPrice()) *
           m_position.GetOpenedQty();
  }
  virtual Double CalcCloseLeakage() const override {
    return (m_position.GetCloseAvgPrice() -
            m_operationContext.GetCloseSignalPrice()) *
           m_position.GetOpenedQty();
  }
  virtual Double CalcLeakagePercent() const override {
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
  os << "Date";                         // 1
  os << ",Open Start Time";             // 2
  os << ",Open Time";                   // 3
  os << ",Opening Duration";            // 4
  os << ",Close Start Time";            // 5
  os << ",Close Time";                  // 6
  os << ",Closing Duration";            // 7
  os << ",Position Duration";           // 8
  os << ",Type";                        // 9
  os << ",P&L Volume";                  // 10
  os << ",P&L %";                       // 11
  os << ",Open Leakage";                // 12
  os << ",Close Leakage";               // 13
  os << ",Leakage %";                   // 14
  os << ",SEBI Turnover Fees";          // 15
  os << ",Stamp Duty";                  // 16
  os << ",Goods and Services Tax";      // 17
  os << ",Securities Transaction Tax";  // 18
  os << ",Gross P&L";                   // 19
  os << ",Tran Costs";                  // 20
  os << ",Net P&L";                     // 21
  os << ",Leverage";                    // 22
  os << ",Leveraged Net P&L";           // 23
  os << ",Is Profit";                   // 24
  os << ",Is Loss";                     // 25
  os << ",Open Signal Price";           // 26
  os << ",Close Signal Price";          // 27
  os << ",Commission";                  // 28
  os << ",Qty";                         // 29
  os << ",Open Price";                  // 30
  os << ",Open Orders";                 // 31
  os << ",Open Trades";                 // 32
  os << ",Close Reason";                // 33
  os << ",Close Price";                 // 34
  os << ",Close Orders";                // 35
  os << ",Close Trades";                // 36
  os << ",ID";                          // 37
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
  os << ',' << calculator->CalcOpenLeakage();                           // 12
  if (pos.GetCloseReason() == CLOSE_REASON_SIGNAL) {
    os << ',' << calculator->CalcCloseLeakage();    // 13
    os << ',' << calculator->CalcLeakagePercent();  // 14
  } else {
    AssertNe(CLOSE_REASON_NONE, pos.GetCloseReason());
    os << ",,";  // 13, 14
  }
  os << ',' << calculator->CalcSebiTurnoverFees();          // 15
  os << ',' << calculator->CalcStampDuty();                 // 16
  os << ',' << calculator->CalcGoodsAndServicesTax();       // 17
  os << ',' << calculator->CalcSecuritiesTransactionTax();  // 18
  if (pos.GetCloseReason() == CLOSE_REASON_SIGNAL) {
    os << ',' << calculator->CalcGrossPnl();  // 19
  } else {
    os << ',';  // 19
  }
  os << ',' << calculator->CalcTranCosts();  // 20
  if (pos.GetCloseReason() == CLOSE_REASON_SIGNAL) {
    os << ',' << calculator->CalcNetPnl();           // 21
    os << ',' << calculator->CalcLeverage();         // 22
    os << ',' << calculator->CalcLeveragedNetPnl();  // 23
  } else {
    os << ",,,";  // 21, 22, 23
  }
  os << (pos.IsProfit() ? ",1,0" : ",0,1");            // 24, 25
  os << ',' << operationContext.GetOpenSignalPrice();  // 26
  if (pos.GetCloseReason() == CLOSE_REASON_SIGNAL) {
    os << ',' << operationContext.GetCloseSignalPrice();  // 27
  } else {
    AssertNe(CLOSE_REASON_NONE, pos.GetCloseReason());
    os << ',';  // 27
  }
  os << ',' << pos.CalcCommission();          // 28
  os << ',' << pos.GetOpenedQty();            // 29
  os << ',' << pos.GetOpenAvgPrice();         // 30
  os << ',' << pos.GetNumberOfOpenOrders();   // 31
  os << ',' << pos.GetNumberOfOpenTrades();   // 32
  os << ',' << pos.GetCloseReason();          // 33
  os << ',' << pos.GetCloseAvgPrice();        // 34
  os << ',' << pos.GetNumberOfCloseOrders();  // 35
  os << ',' << pos.GetNumberOfCloseTrades();  // 36
  os << ',' << pos.GetId();                   // 37
  os << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
