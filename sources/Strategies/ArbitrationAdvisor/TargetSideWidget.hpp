/*******************************************************************************
 *   Created: 2017/11/19 11:23:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "ui_TargetSideWidget.h"

namespace trdk {
namespace Strategies {
namespace ArbitrageAdvisor {

////////////////////////////////////////////////////////////////////////////////

class TargetSideWidget : public QWidget {
  Q_OBJECT

 public:
  typedef QWidget Base;

 public:
  explicit TargetSideWidget(QWidget *parent);
  virtual ~TargetSideWidget() override = default;

 public:
  void Update(const Security &);
  void Update(const Advice &);

  void Highlight(bool isBest, bool isSignaled);

 signals:
  void Order(const OrderSide &);

 protected:
  virtual Price GetPrice(const Security &) const = 0;
  virtual Qty GetQty(const Security &) const = 0;
  virtual const AdviceSide &GetAdvice(const Advice &) const = 0;
  virtual const QString &GetSignaledStyle() const = 0;
  virtual const QString &GetHighlightedStyle() const = 0;

 private:
  Ui::TargetSideWidget m_ui;
  FrontEnd::Lib::SideAdapter<QLabel> m_side;
};

////////////////////////////////////////////////////////////////////////////////

class TargetBidWidget : public TargetSideWidget {
  Q_OBJECT

 public:
  typedef TargetSideWidget Base;

 public:
  explicit TargetBidWidget(QWidget *parent);
  virtual ~TargetBidWidget() override = default;

 protected:
  virtual Price GetPrice(const Security &) const override;
  virtual Qty GetQty(const Security &) const override;
  virtual const AdviceSide &GetAdvice(const Advice &) const override;
  const QString &GetSignaledStyle() const override;
  const QString &GetHighlightedStyle() const override;
};

////////////////////////////////////////////////////////////////////////////////

class TargetAskWidget : public TargetSideWidget {
  Q_OBJECT

 public:
  typedef TargetSideWidget Base;

 public:
  explicit TargetAskWidget(QWidget *parent);
  virtual ~TargetAskWidget() override = default;

 protected:
  virtual Price GetPrice(const Security &) const override;
  virtual Qty GetQty(const Security &) const override;
  virtual const AdviceSide &GetAdvice(const Advice &) const override;
  const QString &GetSignaledStyle() const override;
  const QString &GetHighlightedStyle() const override;
};

////////////////////////////////////////////////////////////////////////////////
}  // namespace ArbitrageAdvisor
}  // namespace Strategies
}  // namespace trdk