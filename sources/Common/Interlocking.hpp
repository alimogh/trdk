/**************************************************************************
 *   Created: May 20, 2012 6:17:41 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include <Windows.h>
#include <type_traits>

#pragma warning(push)
#pragma warning(disable: 4800)

namespace trdk { namespace Lib { namespace Interlocking {

	namespace Detail {

		template<typename T, size_t typeSize, bool isPointer>
		struct InterlockingImpl {
			//...//
		};

		template<typename T>
		struct InterlockingImpl<T, 4, false> {

			static_assert(
				!std::is_pointer<T>::value,
				"Type must be not a pointer.");

			typedef T ValueType;
			typedef LONG StorageType;

			static ValueType Increment(volatile T &destination) throw() {
				static_assert(
					!std::is_floating_point<T>::value,
					"This implementation doesn't support float increment.");
				const auto result = InterlockedIncrement(
					&reinterpret_cast<volatile StorageType &>(destination));
				return reinterpret_cast<const ValueType &>(result);
			}

			static ValueType Decrement(volatile T &destination) throw() {
				static_assert(
					!std::is_floating_point<T>::value,
					"This implementation doesn't support float decrement.");
				const auto result = InterlockedDecrement(
					&reinterpret_cast<volatile StorageType &>(destination));
				return reinterpret_cast<const ValueType &>(result);
			}

			static ValueType Exchange(
						volatile T &destination,
						const T &value)
					throw() {
				const auto result = InterlockedExchange(
					&reinterpret_cast<volatile StorageType &>(destination),
					reinterpret_cast<const StorageType &>(value));
				return reinterpret_cast<const ValueType &>(result);
			}

			static ValueType CompareExchange(
						volatile ValueType &destination,
						const ValueType &exchangeValue,
						const ValueType &compareValue)
					throw() {
				const auto result = InterlockedCompareExchange(
					&reinterpret_cast<volatile StorageType &>(destination),
					reinterpret_cast<const StorageType &>(exchangeValue),
					reinterpret_cast<const StorageType &>(compareValue));
				return reinterpret_cast<const ValueType &>(result);
			}

		};

		template<typename T>
		struct InterlockingImpl<T, 8, false> {

			static_assert(
				!std::is_pointer<T>::value,
				"Type must be not a pointer.");

			typedef T ValueType;
			typedef LONGLONG StorageType;

			static ValueType Increment(volatile T &destination) throw() {
				static_assert(
					!std::is_floating_point<T>::value,
					"This implementation doesn't support float increment.");
				const auto result = InterlockedIncrement64(
					&reinterpret_cast<volatile StorageType &>(destination));
				return reinterpret_cast<const ValueType &>(result);
			}

			static ValueType Decrement(volatile T &destination) throw() {
				static_assert(
					!std::is_floating_point<T>::value,
					"This implementation doesn't support float decrement.");
				const auto result = InterlockedDecrement64(
					&reinterpret_cast<volatile StorageType &>(destination));
				return reinterpret_cast<const ValueType &>(result);
			}

 			static ValueType Exchange(
 						volatile T &destination,
 						const T &value)
 					throw() {
 				const auto result = InterlockedExchange64(
 					&reinterpret_cast<volatile StorageType &>(destination),
					reinterpret_cast<const StorageType &>(value));
				return reinterpret_cast<const ValueType &>(result);
 			}

			static ValueType CompareExchange(
						volatile ValueType &destination,
						const ValueType &exchangeValue,
						const ValueType &compareValue)
					throw() {
				const auto result = InterlockedCompareExchange64(
					&reinterpret_cast<volatile StorageType &>(destination),
					reinterpret_cast<const StorageType &>(exchangeValue),
					reinterpret_cast<const StorageType &>(compareValue));
				return reinterpret_cast<const ValueType &>(result);
			}

		};

		template<typename T>
		struct InterlockingImpl<T, 4, true> {

			static_assert(std::is_pointer<T>::value, "Type must be a pointer.");

			typedef T ValueType;
			typedef LONG NotPointerStorageType;

			static ValueType Exchange(
						volatile T &destination,
						const T &value)
					throw() {
				const auto result = InterlockedExchangePointer(
					&reinterpret_cast<PVOID volatile &>(destination),
					value);
				return reinterpret_cast<const ValueType &>(result);
			}

			static ValueType CompareExchange(
						volatile ValueType &destination,
						const ValueType &exchangeValue,
						const ValueType &compareValue)
					throw() {
				const auto result = InterlockedCompareExchange(
					&reinterpret_cast<volatile NotPointerStorageType &>(
						destination),
					reinterpret_cast<const NotPointerStorageType &>(
						exchangeValue),
					reinterpret_cast<const NotPointerStorageType &>(
						compareValue));
				return reinterpret_cast<const ValueType &>(result);
			}

		};

		template<typename T>
		struct InterlockingImpl<T, 8, true> {

			static_assert(std::is_pointer<T>::value, "Type must be a pointer.");

			typedef T ValueType;
			typedef LONGLONG NotPointerStorageType;

			static ValueType Exchange(
						volatile T &destination,
						const T &value)
					throw() {
				const auto result = InterlockedExchangePointer(
					&reinterpret_cast<PVOID volatile &>(destination),
					value);
				return reinterpret_cast<const ValueType &>(result);
			}

			static ValueType CompareExchange(
						volatile ValueType &destination,
						const ValueType exchangeValue,
						const ValueType compareValue)
					throw() {
				const auto result = InterlockedCompareExchange64(
					&reinterpret_cast<volatile NotPointerStorageType &>(
						destination),
					reinterpret_cast<const NotPointerStorageType &>(
						exchangeValue),
					reinterpret_cast<const NotPointerStorageType &>(
						compareValue));
				return reinterpret_cast<const ValueType &>(result);
			}

		};

		template<typename T>
		struct TypeToSizedInterlocking {
			typedef InterlockingImpl<
					T,
					sizeof(T),
					std::is_pointer<T>::value>
				SizedInterlocking;
		};

		template<>
		struct TypeToSizedInterlocking<bool> {
			typedef InterlockingImpl<bool, 4, false> SizedInterlocking;
		};

	}

	///////////////////////////////////////////////////////////////////////////

	template<typename T>
	inline T Increment(volatile T &destination) throw() {
		return
			Detail
				::TypeToSizedInterlocking<T>
				::SizedInterlocking
				::Increment(destination);
	}

	template<typename T>
	inline T Decrement(volatile T &destination) throw() {
		return
			Detail
				::TypeToSizedInterlocking<T>
				::SizedInterlocking
				::Decrement(destination);
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

} } }

#pragma warning(pop)
