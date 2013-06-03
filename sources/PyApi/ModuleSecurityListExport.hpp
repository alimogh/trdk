/**************************************************************************
 *   Created: 2013/05/31 08:43:28
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Consumer.hpp"
#include "Core/Service.hpp"

namespace trdk { namespace PyApi {
	
	////////////////////////////////////////////////////////////////////////////////

	template<typename ModuleType, typename IteratorImplementation>
	class ModuleSecurityListExport {
	public:
		typedef ModuleType Module;
	public:
		class IteratorExport
			: public boost::iterator_adaptor<
				IteratorExport,
				IteratorImplementation,
				boost::python::object,
				boost::use_default,
				boost::python::object> {
		public:
			explicit IteratorExport(const IteratorImplementation &);
		public:
			boost::python::object dereference() const;
		};
		typedef IteratorExport iterator;
	public:
		explicit ModuleSecurityListExport(Module &);
	public:
		size_t GetSize() const;
	public:
		iterator begin();
		iterator end();
	public:
		boost::python::object FindBySymbol(const boost::python::str &symbol);
		boost::python::object FindBySymbolAndExchange(
					const boost::python::str &symbol,
					const boost::python::str &exchange);
		boost::python::object FindBySymbolAndPrimaryExchange(
					const boost::python::str &symbol,
					const boost::python::str &exchange,
					const boost::python::str &primaryExchange);
	private:
		Module *m_module;
	};

	////////////////////////////////////////////////////////////////////////////////

	class ConsumerSecurityListExport
		: public ModuleSecurityListExport<
			trdk::Consumer,
			trdk::Module::SecurityList::Iterator> {
	public:
		explicit ConsumerSecurityListExport(trdk::Consumer &);
	public:
		static void ExportClass(const char *className);
	};

	////////////////////////////////////////////////////////////////////////////////

	class ServiceSecurityListExport
		: public ModuleSecurityListExport<
			const trdk::Service,
			trdk::Module::SecurityList::ConstIterator> {
	public:
		explicit ServiceSecurityListExport(const trdk::Service &);
	public:
		static void ExportClass(const char *className);
	};

	////////////////////////////////////////////////////////////////////////////////

} }
