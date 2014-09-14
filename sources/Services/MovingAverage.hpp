/**************************************************************************
 *   Created: 2014/09/14 22:19:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Types.hpp"

namespace trdk { namespace Services {

	class MovingAverage : private boost::noncopyable {

	public:

		//! General Moving Average error.
		class Error : public trdk::Lib::Exception {
		public:
			explicit Error(const char *) throw();
		};

		//! Throws when client code requests value which does not exist.
		class ValueDoesNotExistError : public Error {
		public:
			explicit ValueDoesNotExistError(const char *) throw();
		};

		//! Has not points history.
		class HasNotHistory : public Error {
		public:
			explicit HasNotHistory(const char *) throw();
		};

		enum Type {
			TYPE_SIMPLE,
			TYPE_EXPONENTIAL,
			TYPE_SMOOTHED,
			numberOfTypes
		};

		typedef MovingAveragePoint Point;

	public:

		explicit MovingAverage(const Type &, size_t period, bool isHistoryOn);
		~MovingAverage();

	public:

		bool IsEmpty() const;
		Point GetLastPoint() const;

		bool IsHistoryOn() const;
		size_t GetHistorySize() const;
		Point GetHistoryPoint(size_t index) const;
		Point GetHistoryPointByReversedIndex(size_t index) const;

	public:

		bool PushBack(const boost::posix_time::ptime &, const ScaledPrice &);

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

} }
