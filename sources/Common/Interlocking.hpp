/**************************************************************************
 *   Created: May 20, 2012 6:17:41 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#pragma once

#ifdef BOOST_WINDOWS

#	include <Windows.h>

	namespace Interlocking {

		inline long Increment(volatile long &destination) throw() {
			return InterlockedIncrement(&destination);
		}

		inline long Exchange(volatile long &destination, long value) throw() {
			return InterlockedExchange(&destination, value);
		}

		inline LONGLONG Exchange(volatile LONGLONG &destination, LONGLONG value) throw() {
			return InterlockedExchange64(&destination, value);
		}


		inline long CompareExchange(
					volatile long &destination,
					long exchangeValue,
					long compareValue)
				throw() {
			return InterlockedCompareExchange(&destination, exchangeValue, compareValue);
		}

		inline LONGLONG CompareExchange(
					volatile LONGLONG &destination,
					LONGLONG exchangeValue,
					LONGLONG compareValue)
				throw() {
			return InterlockedCompareExchange64(&destination, exchangeValue, compareValue);
		}

	}

#else

	namespace Interlocking {

		inline long Increment(volatile long &destination) throw() {
			return ++destination;
		}

		inline long Exchange(volatile long &destination, long value) throw() {
			const auto prevVal = destination;
			destination = value;
			return prevVal;
		}

		inline long long Exchange(volatile long long &destination, long long value) throw() {
			const auto prevVal = destination;
			destination = value;
			return prevVal;
		}


		inline long CompareExchange(
					volatile long &destination,
					long exchangeValue,
					long compareValue)
				throw() {
			if (exchangeValue == compareValue) {
				destination = exchangeValue;
			}
			return compareValue;
		}

		inline long long CompareExchange(
					volatile long long &destination,
					long long exchangeValue,
					long long compareValue)
				throw() {
			if (exchangeValue == compareValue) {
				destination = exchangeValue;
			}
			return compareValue;
		}

	}

#endif
