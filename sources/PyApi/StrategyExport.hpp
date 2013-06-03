/**************************************************************************
 *   Created: 2013/01/10 20:01:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "ModuleExport.hpp"
#include "ModuleSecurityListExport.hpp"
#include "PythonToCoreTransit.hpp"
#include "Core/Strategy.hpp"

namespace trdk { namespace PyApi {

	//////////////////////////////////////////////////////////////////////////

	class StrategyInfoExport : public ModuleExport {

	public:

		explicit StrategyInfoExport(const boost::shared_ptr<PyApi::Strategy> &);

	public:

		PyApi::Strategy & GetStrategy();
		const PyApi::Strategy & GetStrategy() const;

		boost::shared_ptr<PyApi::Strategy> ReleaseRefHolder() throw();

	private:

		boost::shared_ptr<PyApi::Strategy> m_strategyRefHolder;

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
					trdk::Strategy::PositionList::Iterator,
					boost::python::object,
					boost::use_default,
					boost::python::object> {

			public:

				explicit IteratorExport(
							const trdk::Strategy::PositionList::Iterator &);

			public:
      
				boost::python::object dereference() const;

			};
			typedef IteratorExport iterator;

		public:

			explicit PositionListExport(trdk::Strategy::PositionList &);

		public:

			static void ExportClass(const char *className);

		public:

			size_t GetSize() const;

		public:

			iterator begin();
			iterator end();

		private:

			trdk::Strategy::PositionList *m_list;

		};

	public:

		//! Creates new strategy instance.
		explicit StrategyExport(PyObject *self, uintptr_t instanceParam);

	public:

		static void ExportClass(const char *className);

	private:

		PositionListExport GetPositions();
		ConsumerSecurityListExport GetSecurities();

		boost::python::object FindSecurityBySymbol(
					const boost::python::str &symbol);
		boost::python::object FindSecurityBySymbolAndExchange(
					const boost::python::str &symbol,
					const boost::python::str &exchange);
		boost::python::object FindSecurityBySymbolAndPrimaryExchange(
					const boost::python::str &symbol,
					const boost::python::str &exchange,
					const boost::python::str &primaryExchange);

	};

	//////////////////////////////////////////////////////////////////////////

} }
