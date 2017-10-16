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
namespace Lib {

template <typename WidgetType>
class PriceAdapter {
 public:
  typedef WidgetType Widget;
  typedef trdk::Price Value;

 public:
  explicit PriceAdapter(Widget &widget, uint8_t precision)
      : m_widget(&widget), m_precision(precision) {
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
    m_widget->setText(
        trdk::FrontEnd::Lib::ConvertPriceToText(value, m_precision));
    m_value = value;
  }

 private:
  Value m_value;
  Widget *m_widget;
  uint8_t m_precision;
};

template <typename WidgetType>
class QtyAdapter {
 public:
  typedef WidgetType Widget;
  typedef trdk::Price Value;

 public:
  explicit QtyAdapter(Widget &widget, uint8_t precision)
      : m_widget(&widget), m_precision(precision) {
    SetValue(std::numeric_limits<double>::quiet_NaN());
  }

 public:
  bool Set(const Value &value) {
    if (m_value == value || (isnan(m_value.Get()) && isnan(value.Get()))) {
      return false;
    }
    SetValue(value);
    return true;
  }

  const Value &Get() const { return m_value; }

 protected:
  void SetValue(const Value &value) {
    m_widget->setText(
        trdk::FrontEnd::Lib::ConvertQtyToText(value, m_precision));
    m_value = value;
  }

 private:
  Value m_value;
  Widget *m_widget;
  uint8_t m_precision;
};

template <typename WidgetType>
class SideAdapter {
 public:
  explicit SideAdapter(QLabel &price, QLabel &qty, uint8_t precision)
      : m_price(price, precision), m_qty(qty, precision) {}

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
    m_widget->setText(trdk::FrontEnd::Lib::ConvertTimeToText(value));
    m_value = value;
  }

 private:
  Value m_value;
  Widget *m_widget;
};
}
}
}