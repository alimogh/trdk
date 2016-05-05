/**************************************************************************
 *   Created: 2016/05/02 11:37:50
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Symbol.hpp"
//#include "DisableBoostWarningsBegin.h"
// #	include <boost/filesystem/path.hpp>
//#include "DisableBoostWarningsEnd.h"

namespace trdk { namespace Lib {

	class ExpirationCalendar {

	public:

		//! Futures contract code.
		/** https://en.wikipedia.org/wiki/Futures_contract#Codes
		  */
		enum Code {
			CODE_JANUARY = 'F',
			CODE_FEBRUARY = 'G',
			CODE_MARCH = 'H',
			CODE_APRIL = 'J',
			CODE_MAY = 'K',
			CODE_JUNE = 'M',
			CODE_JULY = 'N',
			CODE_AUGUST = 'Q',
			CODE_SEPTEMBER = 'U',
			CODE_OCTOBER = 'V',
			CODE_NOVEMBER = 'X',
			CODE_DECEMBER = 'Z'
		};

		struct Contract {
			Code code;
			uint16_t year;
			boost::gregorian::date expirationDate;
		};

		class IteratorImplementation;

		class Iterator : public boost::iterator_facade<
				Iterator,
				const Contract,
				boost::incrementable_traversal_tag> {
		public:
			Iterator();
			explicit Iterator(std::unique_ptr<IteratorImplementation> &&);
			Iterator(const Iterator &);
			~Iterator();
			Iterator & operator =(const Iterator &);
			void Swap(Iterator &) noexcept;
		public:
			operator bool() const;
			const trdk::Lib::ExpirationCalendar::Contract & dereference() const;
			bool equal(const Iterator &) const;
			void increment();
			void advance(const difference_type &);
		private:
			std::unique_ptr<IteratorImplementation> m_pimpl;
		};

		struct Stat {
			size_t numberOfSymbols;
			size_t numberOfExpirations;
		};

	public:
	
		explicit ExpirationCalendar();
		ExpirationCalendar(const ExpirationCalendar &);
		ExpirationCalendar & operator =(const ExpirationCalendar &);
		~ExpirationCalendar();

		void Swap(ExpirationCalendar &) noexcept;

	public:

		//! Loads data from CSV-file.
		/** Resets current dictionary and loads new data from CSV-file.
		  * Raises exception if format will be wrong.
		  */
		void ReloadCsv(const boost::filesystem::path &);

		trdk::Lib::ExpirationCalendar::Stat CalcStat() const;

	public:

		//! Finds contract by start time.
		/** @pram symbol	Symbol to iterate. If symbol not in the dictionary
							- exception will be raised.
		  *	@param start	Start time.
		  */
		Iterator Find(
				const trdk::Lib::Symbol &symbol,
				const boost::posix_time::ptime &start)
				const;

	private:

		class Implementation;
		std::unique_ptr <Implementation> m_pimpl;

	};

} }
