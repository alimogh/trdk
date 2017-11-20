/*******************************************************************************
 *   Created: 2017/11/19 11:44:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "TargetSideWidget.hpp"
#include "Advice.hpp"

using namespace trdk;
using namespace trdk::Strategies::ArbitrageAdvisor;

////////////////////////////////////////////////////////////////////////////////

TargetSideWidget::TargetSideWidget(QWidget *parent) : Base(parent) {
  m_ui.setupUi(this);
  m_side = FrontEnd::Lib::SideAdapter<QLabel>(*m_ui.price, *m_ui.qty, 8);
}

void TargetSideWidget::SetPrecision(uint8_t precision) {
  m_side.SetPrecision(precision);
}

void TargetSideWidget::Update(const Security &security) {
  m_side.Set(GetPrice(security), GetQty(security));
}

void TargetSideWidget::Update(const Advice &source) {
  const auto &advice = GetAdvice(source);
  m_side.Set(advice.price, advice.qty);
}

void TargetSideWidget::Highlight(bool isBest, bool isSignaled) {
  if (isBest) {
    m_ui.frame->setStyleSheet(isSignaled ? GetSignaledStyle()
                                         : GetHighlightedStyle());
  } else {
    m_ui.frame->setStyleSheet(styleSheet());
  }
}

////////////////////////////////////////////////////////////////////////////////

TargetBidWidget::TargetBidWidget(QWidget *parent) : Base(parent) {}

Price TargetBidWidget::GetPrice(const Security &source) const {
  return source.GetBidPriceValue();
}

Qty TargetBidWidget::GetQty(const Security &source) const {
  return source.GetBidQtyValue();
}

const AdviceSide &TargetBidWidget::GetAdvice(const Advice &source) const {
  return source.bid;
}

const QString &TargetBidWidget::GetSignaledStyle() const {
  static const QString result("background-color: rgb(230, 59, 1);");
  return result;
}

const QString &TargetBidWidget::GetHighlightedStyle() const {
  static const QString result("color: rgb(230, 59, 1);");
  return result;
}

////////////////////////////////////////////////////////////////////////////////

TargetAskWidget::TargetAskWidget(QWidget *parent) : Base(parent) {}

Price TargetAskWidget::GetPrice(const Security &source) const {
  return source.GetAskPriceValue();
}
Qty TargetAskWidget::GetQty(const Security &source) const {
  return source.GetAskQtyValue();
}
const AdviceSide &TargetAskWidget::GetAdvice(const Advice &source) const {
  return source.ask;
}

const QString &TargetAskWidget::GetSignaledStyle() const {
  static const QString result("background-color: rgb(0, 146, 68);");
  return result;
}

const QString &TargetAskWidget::GetHighlightedStyle() const {
  static const QString result("color: rgb(0, 195, 91);");
  return result;
}

////////////////////////////////////////////////////////////////////////////////
