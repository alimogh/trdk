/**************************************************************************
 *   Created: 2013/01/10 20:01:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "SecurityAlgoExport.hpp"
#include "PythonToCoreTransit.hpp"
#include "Core/Strategy.hpp"

namespace Trader { namespace PyApi {

	//////////////////////////////////////////////////////////////////////////

	class StrategyInfoExport : public SecurityAlgoExport {

	public:

		explicit StrategyInfoExport(const boost::shared_ptr<PyApi::Strategy> &);

	public:

		PyApi::Strategy & GetStrategy();
		const PyApi::Strategy & GetStrategy() const;

		boost::shared_ptr<PyApi::Strategy> ReleaseRefHolder() throw();

	private:

		boost::shared_ptr<PyApi::Strategy> m_strategyRefHolder;
		PyApi::Strategy *m_strategy;

	};

	//////////////////////////////////////////////////////////////////////////

	class StrategyExport : public StrategyInfoExport,
		public Detail::PythonToCoreTransit<StrategyExport>,
		public boost::enable_shared_from_this<StrategyExport> {

	private:

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

			explicit PositionListExport(Trader::Strategy::PositionList &);

		public:

			static void Export(const char *className);

		public:

			size_t GetSize() const;

		public:

			iterator begin();
			iterator end();

		private:

			Trader::Strategy::PositionList *m_list;

		};

	public:

		//! Creates new strategy instance.
		explicit StrategyExport(PyObject *self, uintptr_t instanceParam);

	public:

		static void Export(const char *className);

	private:

		PositionListExport GetPositions();

	private:

		SecurityExport m_securityExport;

	};

	//////////////////////////////////////////////////////////////////////////

} }
