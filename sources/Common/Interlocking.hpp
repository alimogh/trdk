/**************************************************************************
 *   Created: May 20, 2012 6:17:41 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

namespace Trader { namespace Lib { namespace Interlocking {

#ifdef BOOST_WINDOWS

#	pragma warning(push)
#	pragma warning(disable: 4800)

#	include <Windows.h>

	namespace Detail {

		template<typename T, size_t typeSize>
		struct SizedInterlocking {
			//...//
		};

		template<typename T>
		struct SizedInterlocking<T, 4> {

			typedef T ValueType;
			typedef LONG StorageType;

			static ValueType Increment(volatile T &destination) throw() {
				return (ValueType)InterlockedIncrement(
					(volatile StorageType *)&destination);
			}

			static ValueType Exchange(
						volatile T &destination,
						const T &value)
					throw() {
				return (ValueType)InterlockedExchange(
					(volatile StorageType *)&destination,
					(StorageType)value);
			}

			static ValueType CompareExchange(
						volatile ValueType &destination,
						const ValueType &exchangeValue,
						const ValueType &compareValue)
					throw() {
				return InterlockedCompareExchange(
					(volatile StorageType *)&destination,
					(StorageType)exchangeValue,
					(StorageType)compareValue);
			}

		};

		template<typename T>
		struct SizedInterlocking<T, 8> {

			typedef T ValueType;
			typedef LONGLONG StorageType;

			static ValueType Increment(volatile T &destination) throw() {
				return (ValueType)InterlockedIncrement64(
					(volatile StorageType *)&destination);
			}

			static ValueType Exchange(volatile T &destination, const T &value) throw() {
				return (ValueType)InterlockedExchange64(
					(volatile StorageType *)&destination,
					(StorageType)value);
			}

			static ValueType CompareExchange(
						volatile ValueType &destination,
						const ValueType &exchangeValue,
						const ValueType &compareValue)
					throw() {
				return InterlockedCompareExchange64(
					(volatile StorageType *)&destination,
					(StorageType)exchangeValue,
					(StorageType)compareValue);
			}

		};

		template<typename T>
		struct TypeToSizedInterlocking {
			typedef SizedInterlocking<T, sizeof(T)> SizedInterlocking;
		};

		template<>
		struct TypeToSizedInterlocking<bool> {
			typedef SizedInterlocking<bool, 4> SizedInterlocking;
		};

	}

	template<typename T>
	inline T Increment(volatile T &destination) throw() {
		return
			Detail
				::TypeToSizedInterlocking<T>
				::SizedInterlocking
				::Increment(destination);
	}

	template<typename T, typename Y>
	inline T Exchange(volatile T &destination, const Y &value) throw() {
		return
			Detail
				::TypeToSizedInterlocking<T>
				::SizedInterlocking
				::Exchange(destination, value);
	}

	template<typename T, typename Y>
	inline T CompareExchange(
				volatile T &destination,
				const Y &exchangeValue,
				const Y &compareValue)
			throw() {
		return
			Detail
				::TypeToSizedInterlocking<T>
				::SizedInterlocking
				::CompareExchange(destination, exchangeValue, compareValue);
	}

#	pragma warning(pop)

#else

	inline long Increment(volatile long &destination) throw() {
		return ++destination;
	}

	template<typename T, typename Y>
	inline T Exchange(volatile T &destination, const Y &value) throw() {
		const auto prevVal = destination;
		destination = value;
		return prevVal;
	}

	template<typename T, typename Y>
	inline T CompareExchange(
				volatile T &destination,
				const Y &exchangeValue,
				const Y &compareValue)
			throw() {
		const auto prevVal = destination;
		if (prevVal == compareValue) {
			destination = exchangeValue;
		}
		return prevVal;
	}

#endif

} } }
