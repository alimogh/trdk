/**************************************************************************
 *   Created: May 20, 2012 6:17:41 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#pragma once

#include <Windows.h>

namespace Interlocking {

	inline long Increment(volatile long &destination) throw() {
#		ifdef _WINDOWS
			return InterlockedIncrement(&destination);
#		else
			static_assert(false, "Not implemented.");
#		endif
	}

	inline long Exchange(volatile long &destination, long value) throw() {
#		ifdef _WINDOWS
			return InterlockedExchange(&destination, value);
#		else
			return __sync_val_compare_and_swap(&destination, destination, value);
#		endif
	}

	inline LONGLONG Exchange(volatile LONGLONG &destination, LONGLONG value) throw() {
#		ifdef _WINDOWS
			return InterlockedExchange64(&destination, value);
#		else
			return __sync_val_compare_and_swap(&destination, destination, value);
#		endif
	}


	inline long CompareExchange(
				volatile long &destination,
				long exchangeValue,
				long compareValue)
			throw() {
#		ifdef _WINDOWS
			return InterlockedCompareExchange(&destination, exchangeValue, compareValue);
#		else
			return __sync_val_compare_and_swap(&destination, compareValue, exchangeValue);
#		endif
	}

	inline LONGLONG CompareExchange(
				volatile LONGLONG &destination,
				LONGLONG exchangeValue,
				LONGLONG compareValue)
			throw() {
#		ifdef _WINDOWS
			return InterlockedCompareExchange64(&destination, exchangeValue, compareValue);
#		else
			return __sync_val_compare_and_swap(&destination, compareValue, exchangeValue);
#		endif
	}

}
