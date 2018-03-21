/*******************************************************************************
 *   Created: 2017/10/16 16:04:03
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

namespace trdk {
namespace FrontEnd {

template <typename WidgetType>
class PriceAdapter {
 public:
  typedef WidgetType Widget;
  typedef trdk::Price Value;

 public:
  PriceAdapter() : m_widget(nullptr) {}
  explicit PriceAdapter(Widget &widget) : m_widget(&widget) {
    SetValue(std::numeric_limits<double>::quiet_NaN());
  }

 public:
  bool Set(const Value &value) {
    if (m_value == value) {
      return false;
    }
    SetValue(value);
    return true;
  }

  const Value &Get() const { return m_value; }

 protected:
  void SetValue(const Value &value) {
    Assert(m_widget);
    if (!m_widget) {
      return;
    }
    m_widget->setText(FrontEnd::ConvertPriceToText(value));
    m_value = value;
  }

 private:
  Value m_value;
  Widget *m_widget;
};

template <typename WidgetType>
class QtyAdapter {
 public:
  typedef WidgetType Widget;
  typedef trdk::Price Value;

 public:
  QtyAdapter() : m_widget(nullptr) {}
  explicit QtyAdapter(Widget &widget) : m_widget(&widget) {
    SetValue(std::numeric_limits<double>::quiet_NaN());
  }

 public:
  bool Set(const Value &value) {
    if (m_value == value || (m_value.IsNan() && value.IsNan())) {
      return false;
    }
    SetValue(value);
    return true;
  }

  const Value &Get() const { return m_value; }

 protected:
  void SetValue(const Value &value) {
    Assert(m_widget);
    if (!m_widget) {
      return;
    }
    m_widget->setText(FrontEnd::ConvertQtyToText(value));
    m_value = value;
  }

 private:
  Value m_value;
  Widget *m_widget;
};

template <typename WidgetType>
class SideAdapter {
 public:
  SideAdapter() {}
  explicit SideAdapter(QLabel &price, QLabel &qty)
      : m_price(price), m_qty(qty) {}

 public:
  void Set(const trdk::Price &price, const trdk::Qty &qty) {
    m_price.Set(price);
    m_qty.Set(qty);
  }

  PriceAdapter<WidgetType> &GetPrice() { return m_price; }
  const PriceAdapter<WidgetType> &GetPrice() const {
    return const_cast<SideAdapter *>(this)->GetPrice();
  }

  QtyAdapter<WidgetType> &GetQty() { return m_price; }
  const QtyAdapter<WidgetType> &GetQty() const {
    return const_cast<SideAdapter *>(this)->GetQty();
  }

 private:
  PriceAdapter<WidgetType> m_price;
  QtyAdapter<WidgetType> m_qty;
};

template <typename WidgetType>
class TimeAdapter {
 public:
  typedef WidgetType Widget;
  typedef boost::posix_time::ptime Value;

 public:
  TimeAdapter() : m_widget(nullptr) {}
  explicit TimeAdapter(Widget &widget) : m_widget(&widget) {
    SetValue(boost::posix_time::not_a_date_time);
  }

 public:
  bool Set(const Value &value) {
    if (m_value == value) {
      return false;
    }
    Assert(!value.is_not_a_date_time() || !m_value.is_not_a_date_time());
    SetValue(value);
    return true;
  }

  const Value &Get() const { return m_value; }

 protected:
  void SetValue(const Value &value) {
    Assert(m_widget);
    if (!m_widget) {
      return;
    }
    m_widget->setText(FrontEnd::ConvertTimeToText(value.time_of_day()));
    m_value = value;
  }

 private:
  Value m_value;
  Widget *m_widget;
};
}  // namespace FrontEnd
}  // namespace trdk