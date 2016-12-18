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

namespace trdk { namespace Lib {

	////////////////////////////////////////////////////////////////////////////////

	template<typename T>
	class Numeric {

	public:

		typedef T ValueType;

		static_assert(boost::is_pod<ValueType>::value, "Type should be POD.");

	public:

		Numeric(const ValueType &value = 0)
			: m_value(value) {
			//...//
		}

		void Swap(Numeric &rhs) noexcept {
			std::swap(m_value, rhs.m_value);
		}

		operator ValueType() const noexcept {
			return Get();
		}

		const ValueType & Get() const noexcept {
			return m_value;
		}

		Numeric & operator =(const ValueType &rhs) noexcept {
			m_value = rhs;
			return *this;
		}

		template<typename StreamElem, typename StreamTraits>
		friend std::basic_ostream<StreamElem, StreamTraits> & operator <<(
				std::basic_ostream<StreamElem, StreamTraits> &os,
				const trdk::Lib::Numeric<ValueType> &numeric) {
			os << numeric.Get();
			return os;
		}

		template<typename StreamElem, typename StreamTraits>
		friend std::basic_istream<StreamElem, StreamTraits> & operator >>(
				std::basic_istream<StreamElem, StreamTraits> &is,
				trdk::Lib::Numeric<ValueType> &numeric) {
			is >> numeric.m_value;
			return is;
		}

	public:

		template<typename AnotherValueType>
		bool operator ==(const AnotherValueType &rhs) const {
			return operator ==(Numeric(rhs));
		}
		bool operator ==(const Numeric &rhs) const {
			return trdk::Lib::IsEqual(m_value, rhs.m_value);
		}

		template<typename AnotherValueType>
		bool operator !=(const AnotherValueType &rhs) const {
			return operator !=(Numeric(rhs));
		}
		bool operator !=(const Numeric &rhs) const {
			return !trdk::Lib::IsEqual(m_value, rhs.m_value);
		}

		template<typename AnotherValueType>
		bool operator <(const AnotherValueType &rhs) const {
			return operator <(Numeric(rhs));
		}
		bool operator <(const Numeric &rhs) const {
			return m_value < rhs.m_value && !trdk::Lib::IsEqual(m_value, rhs);
		}

		template<typename AnotherValueType>
		bool operator <=(const AnotherValueType &rhs) const {
			return operator <=(Numeric(rhs));
		}
		bool operator <=(const Numeric &rhs) const {
			return m_value < rhs.m_value || trdk::Lib::IsEqual(m_value, rhs);
		}

		template<typename AnotherValueType>
		bool operator >(const AnotherValueType &rhs) const {
			return operator >(Numeric(rhs));
		}
		bool operator >(const Numeric &rhs) const {
			return m_value > rhs.m_value && !trdk::Lib::IsEqual(m_value, rhs);
		}

		template<typename AnotherValueType>
		bool operator >=(const AnotherValueType &rhs) const {
			return operator >=(Numeric(rhs));
		}
		bool operator >=(const Numeric &rhs) const {
			return m_value > rhs.m_value || trdk::Lib::IsEqual(m_value, rhs);
		}

	public:

		template<typename AnotherValueType>
		Numeric & operator +=(const AnotherValueType &rhs) {
			m_value += rhs;
			return *this;
		}
		Numeric & operator +=(const Numeric &rhs) {
			m_value += rhs.m_value;
			return *this;
		}

		template<typename AnotherValueType>
		Numeric & operator -=(const AnotherValueType &rhs) {
			m_value -= rhs;
			return *this;
		}
		Numeric & operator -=(const Numeric &rhs) {
			m_value -= rhs.m_value;
			return *this;
		}

		template<typename AnotherValueType>
		Numeric & operator *=(const Numeric &rhs) {
			m_value *= rhs;
			return *this;
		}
		Numeric & operator *=(const Numeric &rhs) {
			m_value *= rhs.m_value;
			return *this;
		}

		template<typename AnotherValueType>
		Numeric & operator /=(const AnotherValueType &rhs) {
			m_value /= rhs;
			return *this;
		}
		Numeric & operator /=(const Numeric &rhs) {
			m_value /= rhs.m_value;
			return *this;
		}

		template<typename AnotherValueType>
		Numeric operator +(const AnotherValueType &rhs) const {
			return Numeric(m_value + rhs);
		}
		Numeric operator +(const Numeric &rhs) const {
			return Numeric(m_value + rhs.m_value);
		}

		template<typename AnotherValueType>
		Numeric operator -(const AnotherValueType &rhs) const {
			return Numeric(m_value - rhs);
		}
		Numeric operator -(const Numeric &rhs) const {
			return Numeric(m_value - rhs.m_value);
		}

		template<typename AnotherValueType>
		Numeric operator *(const AnotherValueType &rhs) const {
			return Numeric(m_value * rhs);
		}
		Numeric operator *(const Numeric &rhs) const {
			return Numeric(m_value * rhs.m_value);
		}

		template<typename AnotherValueType>
		Numeric operator /(const AnotherValueType &rhs) const {
			return Numeric(m_value / rhs);
		}
		Numeric operator /(const Numeric &rhs) const {
			return Numeric(m_value / rhs.m_value);
		}

	private:

		ValueType m_value;

	};

	////////////////////////////////////////////////////////////////////////////////

	template<typename T>
	bool IsEqual(const trdk::Lib::Numeric<T> &, const trdk::Lib::Numeric<T> &) {
		static_assert(false, "Deprecated call, use operator \"==\" instead.");
	}

	template<typename T>
	bool IsZero(const trdk::Lib::Numeric<T> &) {
		static_assert(false, "Deprecated call, use operator \"==\" instead.");
	}

	////////////////////////////////////////////////////////////////////////////////

	typedef trdk::Lib::Numeric<double> Double;

	////////////////////////////////////////////////////////////////////////////////

} }
