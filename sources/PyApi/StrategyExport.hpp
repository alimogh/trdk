/**************************************************************************
 *   Created: 2013/01/10 20:01:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "SecurityAlgoExport.hpp"
#include "Core/Strategy.hpp"


namespace Trader { namespace PyApi {

	class StrategyExport : public SecurityAlgoExport {

	public:

		class PositionListExport {

		public:

			class IteratorExport
				: public boost::iterator_adaptor<
					IteratorExport,
					Trader::Strategy::PositionList::Iterator,
					boost::python::object,
					boost::use_default,
					boost::python::object> {

			public:

				explicit IteratorExport(
							const Trader::Strategy::PositionList::Iterator &);

			public:
      
				boost::python::object dereference() const;

			};
			typedef IteratorExport iterator;

		public:

			explicit PositionListExport(Strategy::PositionList &);

		public:

			static void Export(const char *className);

		public:

			size_t GetSize() const;

		public:

			iterator begin();
			iterator end();

		private:

			Strategy::PositionList *m_list;

		};

	public:

		explicit StrategyExport(Strategy &);

	public:

		static void Export(const char *className);

	public:

		void CallOnLevel1UpdatePyMethod();
		void CallOnNewTradePyMethod(
					const boost::python::object &time,
					const boost::python::object &price,
					const boost::python::object &qty,
					const boost::python::object &side);
		void CallOnServiceDataUpdatePyMethod(
					const boost::python::object &service);
		void CallOnPositionUpdatePyMethod(
					const boost::python::object &position);

	public:

		PositionListExport GetPositions();

	public:

		Trader::Strategy & GetStrategy();
		const Trader::Strategy & GetStrategy() const;

	};

} }
