/**************************************************************************
 *   Created: 2015/10/22 13:26:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include <boost/math/special_functions/round.hpp>

namespace trdk {
namespace Lib {

////////////////////////////////////////////////////////////////////////////////

namespace Detail {

template <uint8_t precision>
struct DoubleNumericPolicy {
  enum { PRECISION = precision };

  static constexpr double GetPrecisionPower() { return pow(10, precision); }
  static constexpr double GetEpsilon() { return 1 / GetPrecisionPower(); }

  template <typename T>
  static bool IsEq(const T &lhs, const T &rhs) {
    return std::abs(lhs - rhs) <= GetEpsilon();
  }
  template <typename T>
  static bool IsNe(const T &lhs, const T &rhs) {
    return !IsEq(lhs, rhs);
  }

  template <typename T>
  static bool IsLt(const T &lhs, const T &rhs) {
    return lhs < rhs && !IsEq(lhs, rhs);
  }
  template <typename T>
  static bool IsLe(const T &lhs, const T &rhs) {
    return lhs < rhs || IsEq(lhs, rhs);
  }

  template <typename T>
  static bool IsGt(const T &lhs, const T &rhs) {
    return lhs > rhs && !IsEq(lhs, rhs);
  }
  template <typename T>
  static bool IsGe(const T &lhs, const T &rhs) {
    return lhs > rhs || IsEq(lhs, rhs);
  }

  template <typename T>
  static bool IsNan(const T &val) {
    return isnan(val);
  }
  template <typename T>
  static bool IsNotNan(const T &val) {
    return !IsNan(val);
  }

  template <typename T>
  static void Normalize(T &) {}

  template <typename StreamElem, typename StreamTraits, typename Source>
  static void Dump(std::basic_ostream<StreamElem, StreamTraits> &os,
                   const Source &source) {
    os << std::fixed << std::setprecision(8) << source;
  }

  template <typename StreamElem, typename StreamTraits, typename Result>
  static void Load(std::basic_istream<StreamElem, StreamTraits> &is,
                   Result &result) {
    is >> result;
  }
};

template <uint8_t precision>
struct DoubleWithFixedPrecisionNumericPolicy : DoubleNumericPolicy<precision> {
  static_assert(precision > 0, "Must be greater than zero.");

  typedef DoubleNumericPolicy<precision> Base;

  using Base::GetPrecisionPower;

  template <typename T>
  static void Normalize(T &value) {
    if (IsNan(value)) {
      return;
    }
    value =
        boost::math::round(value * GetPrecisionPower()) / GetPrecisionPower();
  }
};

}  // namespace Detail

////////////////////////////////////////////////////////////////////////////////

template <typename T, typename Policy>
class Numeric {
 public:
  typedef T ValueType;

  static_assert(boost::is_pod<ValueType>::value, "Type should be POD.");

  enum { PRECISION = Policy::PRECISION };

  Numeric(const ValueType &value = 0) : m_value(value) { Normalize(); }

  void Swap(Numeric &rhs) noexcept { std::swap(m_value, rhs.m_value); }

  static uint8_t GetPrecision() { return PRECISION; }

  operator ValueType() const noexcept { return Get(); }

  const ValueType &Get() const noexcept { return m_value; }

  Numeric &operator=(const ValueType &rhs) noexcept {
    m_value = rhs;
    Normalize();
    return *this;
  }

  template <typename StreamElem, typename StreamTraits>
  friend std::basic_ostream<StreamElem, StreamTraits> &operator<<(
      std::basic_ostream<StreamElem, StreamTraits> &os,
      const Numeric<ValueType, Policy> &numeric) {
    numeric.Dump(os);
    return os;
  }

  template <typename StreamElem, typename StreamTraits>
  friend std::basic_istream<StreamElem, StreamTraits> &operator>>(
      std::basic_istream<StreamElem, StreamTraits> &is,
      Numeric<ValueType, Policy> &numeric) {
    numeric.Load(is);
    return is;
  }

  template <typename AnotherValueType>
  bool operator==(const AnotherValueType &rhs) const {
    return operator==(Numeric(rhs));
  }
  bool operator==(const Numeric &rhs) const {
    return Policy::IsEq(m_value, rhs.m_value);
  }

  template <typename AnotherValueType>
  bool operator!=(const AnotherValueType &rhs) const {
    return operator!=(Numeric(rhs));
  }
  bool operator!=(const Numeric &rhs) const {
    return Policy::IsNe(m_value, rhs.m_value);
  }

  template <typename AnotherValueType>
  bool operator<(const AnotherValueType &rhs) const {
    return operator<(Numeric(rhs));
  }
  bool operator<(const Numeric &rhs) const {
    return Policy::IsLt(m_value, rhs.m_value);
  }

  template <typename AnotherValueType>
  bool operator<=(const AnotherValueType &rhs) const {
    return operator<=(Numeric(rhs));
  }
  bool operator<=(const Numeric &rhs) const {
    return Policy::IsLe(m_value, rhs.m_value);
  }

  template <typename AnotherValueType>
  bool operator>(const AnotherValueType &rhs) const {
    return operator>(Numeric(rhs));
  }
  bool operator>(const Numeric &rhs) const {
    return Policy::IsGt(m_value, rhs.m_value);
  }

  template <typename AnotherValueType>
  bool operator>=(const AnotherValueType &rhs) const {
    return operator>=(Numeric(rhs));
  }
  bool operator>=(const Numeric &rhs) const {
    return Policy::IsGe(m_value, rhs.m_value);
  }

  bool IsNan() const { return Policy::IsNan(m_value); }
  bool IsNotNan() const { return Policy::IsNotNan(m_value); }

  template <typename AnotherValueType>
  Numeric &operator+=(const AnotherValueType &rhs) {
    m_value += rhs;
    Normalize();
    return *this;
  }
  Numeric &operator+=(const Numeric &rhs) {
    m_value += rhs.m_value;
    Normalize();
    return *this;
  }

  template <typename AnotherValueType>
  Numeric &operator-=(const AnotherValueType &rhs) {
    m_value -= rhs;
    Normalize();
    return *this;
  }
  Numeric &operator-=(const Numeric &rhs) {
    m_value -= rhs.m_value;
    Normalize();
    return *this;
  }

  template <typename AnotherValueType>
  Numeric &operator*=(const Numeric &rhs) {
    m_value *= rhs;
    Normalize();
    return *this;
  }
  Numeric &operator*=(const Numeric &rhs) {
    m_value *= rhs.m_value;
    Normalize();
    return *this;
  }

  template <typename AnotherValueType>
  Numeric &operator/=(const AnotherValueType &rhs) {
    m_value /= rhs;
    Normalize();
    return *this;
  }
  Numeric &operator/=(const Numeric &rhs) {
    m_value /= rhs.m_value;
    Normalize();
    return *this;
  }

  template <typename AnotherValueType>
  Numeric operator+(const AnotherValueType &rhs) const {
    return Numeric(m_value + rhs);
  }
  Numeric operator+(const Numeric &rhs) const {
    return Numeric(m_value + rhs.m_value);
  }

  template <typename AnotherValueType>
  Numeric operator-(const AnotherValueType &rhs) const {
    return Numeric(m_value - rhs);
  }
  Numeric operator-(const Numeric &rhs) const {
    return Numeric(m_value - rhs.m_value);
  }

  template <typename AnotherValueType>
  Numeric operator*(const AnotherValueType &rhs) const {
    return Numeric(m_value * rhs);
  }
  Numeric operator*(const Numeric &rhs) const {
    return Numeric(m_value * rhs.m_value);
  }

  template <typename AnotherValueType>
  Numeric operator/(const AnotherValueType &rhs) const {
    if (rhs == 0) {
      throw std::overflow_error("Division by zero");
    }
    return Numeric(m_value / rhs);
  }
  Numeric operator/(const Numeric &rhs) const {
    if (rhs == 0) {
      throw std::overflow_error("Division by zero");
    }
    return Numeric(m_value / rhs.m_value);
  }

 protected:
  template <typename StreamElem, typename StreamTraits>
  void Dump(std::basic_ostream<StreamElem, StreamTraits> &os) const {
    Policy::Dump(os, m_value);
  }

  template <typename StreamElem, typename StreamTraits>
  void Load(std::basic_istream<StreamElem, StreamTraits> &is) {
    Policy::Load(is, m_value);
    Normalize();
  }

 private:
  void Normalize() { Policy::Normalize(m_value); }

  ValueType m_value;
};

////////////////////////////////////////////////////////////////////////////////

typedef Numeric<double, Detail::DoubleNumericPolicy<15>> Double;

////////////////////////////////////////////////////////////////////////////////

template <typename T, typename Policy>
class BusinessNumeric : public Numeric<T, Policy> {
 public:
  typedef Numeric<T, Policy> Base;
  typedef typename Base::ValueType ValueType;

  using Base::Get;

  BusinessNumeric(const ValueType &value = 0) noexcept : Base(value) {}
  BusinessNumeric(const Double &value) : Base(value.Get()) {}

  operator Double() const { return Get(); }

  template <typename StreamElem, typename StreamTraits>
  friend std::basic_ostream<StreamElem, StreamTraits> &operator<<(
      std::basic_ostream<StreamElem, StreamTraits> &os,
      const BusinessNumeric<ValueType, Policy> &numeric) {
    numeric.Dump(os);
    return os;
  }

  template <typename StreamElem, typename StreamTraits>
  friend std::basic_istream<StreamElem, StreamTraits> &operator>>(
      std::basic_istream<StreamElem, StreamTraits> &is,
      BusinessNumeric<ValueType, Policy> &numeric) {
    numeric.Load(is);
    return is;
  }

  template <typename AnotherValueType>
  bool operator==(const AnotherValueType &rhs) const {
    return operator==(BusinessNumeric(rhs));
  }
  bool operator==(const BusinessNumeric &rhs) const {
    return Base::operator==(rhs);
  }

  template <typename AnotherValueType>
  bool operator!=(const AnotherValueType &rhs) const {
    return operator!=(BusinessNumeric(rhs));
  }
  bool operator!=(const BusinessNumeric &rhs) const {
    return Base::operator!=(rhs);
  }

  template <typename AnotherValueType>
  bool operator<(const AnotherValueType &rhs) const {
    return operator<(BusinessNumeric(rhs));
  }
  bool operator<(const BusinessNumeric &rhs) const {
    return Base::operator<(rhs);
  }

  template <typename AnotherValueType>
  bool operator<=(const AnotherValueType &rhs) const {
    return operator<=(BusinessNumeric(rhs));
  }
  bool operator<=(const BusinessNumeric &rhs) const {
    return Base::operator<=(rhs);
  }

  template <typename AnotherValueType>
  bool operator>(const AnotherValueType &rhs) const {
    return operator>(BusinessNumeric(rhs));
  }
  bool operator>(const BusinessNumeric &rhs) const {
    return Base::operator>(rhs);
  }

  template <typename AnotherValueType>
  bool operator>=(const AnotherValueType &rhs) const {
    return operator>=(BusinessNumeric(rhs));
  }
  bool operator>=(const BusinessNumeric &rhs) const {
    return Base::operator>=(rhs);
  }

  template <typename AnotherValueType>
  BusinessNumeric &operator+=(const AnotherValueType &rhs) {
    return operator+=(BusinessNumeric(rhs));
  }
  BusinessNumeric &operator+=(const BusinessNumeric &rhs) {
    Base::operator+=(rhs);
    return *this;
  }

  template <typename AnotherValueType>
  BusinessNumeric &operator-=(const AnotherValueType &rhs) {
    return operator-=(BusinessNumeric(rhs));
  }
  BusinessNumeric &operator-=(const BusinessNumeric &rhs) {
    Base::operator-=(rhs);
    return *this;
  }

  template <typename AnotherValueType>
  BusinessNumeric &operator*=(const AnotherValueType &rhs) {
    return operator*=(BusinessNumeric(rhs));
  }
  BusinessNumeric &operator*=(const BusinessNumeric &rhs) {
    Base::operator*=(rhs);
    return *this;
  }

  template <typename AnotherValueType>
  BusinessNumeric &operator/=(const AnotherValueType &rhs) {
    return operator/=(BusinessNumeric(rhs));
  }
  BusinessNumeric &operator/=(const BusinessNumeric &rhs) {
    Base::operator/=(rhs);
    return *this;
  }

  template <typename AnotherValueType>
  BusinessNumeric operator+(const AnotherValueType &rhs) const {
    return operator+(BusinessNumeric(rhs));
  }
  BusinessNumeric operator+(const BusinessNumeric &rhs) const {
    return BusinessNumeric(Get() + rhs.Get());
  }

  template <typename AnotherValueType>
  BusinessNumeric operator-(const AnotherValueType &rhs) const {
    return operator-(BusinessNumeric(rhs));
  }
  BusinessNumeric operator-(const BusinessNumeric &rhs) const {
    return BusinessNumeric(Get() - rhs.Get());
  }

  template <typename AnotherValueType>
  BusinessNumeric operator*(const AnotherValueType &rhs) const {
    return BusinessNumeric(Get() * rhs);
  }
  BusinessNumeric operator*(const BusinessNumeric &rhs) const {
    return BusinessNumeric(Get() * rhs.Get());
  }

  template <typename AnotherValueType>
  BusinessNumeric operator/(const AnotherValueType &rhs) const {
    if (rhs == 0) {
      throw std::overflow_error("Division by zero");
    }
    return BusinessNumeric(Get() / rhs);
  }
  BusinessNumeric operator/(const BusinessNumeric &rhs) const {
    if (rhs == 0) {
      throw std::overflow_error("Division by zero");
    }
    return BusinessNumeric(Get() / rhs.Get());
  }
};

////////////////////////////////////////////////////////////////////////////////

}  // namespace Lib
}  // namespace trdk
