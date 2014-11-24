/**************************************************************************
 *   Created: 2013/05/30 22:43:03
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Module.hpp"
#include "Fwd.hpp"

namespace trdk {

	////////////////////////////////////////////////////////////////////////////////

	typedef std::map<
			boost::tuple<
				size_t/*trdk::Lib::Symbol::Hash*/,
				const trdk::MarketDataSource *>,
			Security *>
		ModuleSecurityListStorage;

	////////////////////////////////////////////////////////////////////////////////

	class Module::SecurityList::Iterator::Implementation {
	public:
		ModuleSecurityListStorage::iterator iterator;
	public:
		explicit Implementation(ModuleSecurityListStorage::iterator iterator)
				: iterator(iterator) {
			//...//
		}
	};
	
	class Module::SecurityList::ConstIterator::Implementation {
	public:
		ModuleSecurityListStorage::const_iterator iterator;
	public:
		explicit Implementation(ModuleSecurityListStorage::const_iterator iterator)
				: iterator(iterator) {
			//...//
		}
	};

	////////////////////////////////////////////////////////////////////////////////

	class ModuleSecurityList : public Module::SecurityList {

	public:

		 typedef trdk::Module::SecurityList::Iterator Iterator;
		 typedef trdk::Module::SecurityList::ConstIterator ConstIterator;

	public:
		
		virtual ~ModuleSecurityList() {
			//...//
		}

	public:

		bool Insert(Security &security) {
			const auto &key = boost::make_tuple(
				security.GetSymbol().GetHash(),
				&security.GetSource());
			if (m_list.find(key) != m_list.end()) {
				Assert(&security == m_list[key]);
				return false;
			}
			m_list[key] = &security;
			return true;
		}

	public:

		virtual size_t GetSize() const {
			return m_list.size();
		}
		
		virtual bool IsEmpty() const {
			return m_list.empty();
		}
		
		virtual Iterator GetBegin() {
			return Iterator(new Iterator::Implementation(m_list.begin()));
		}
		virtual ConstIterator GetBegin() const {
			return ConstIterator(
				new ConstIterator::Implementation(m_list.begin()));
		}
		virtual Iterator GetEnd() {
			return Iterator(new Iterator::Implementation(m_list.end()));
		}
		virtual ConstIterator GetEnd() const {
			return ConstIterator(
				new ConstIterator::Implementation(m_list.end()));
		}

		virtual Iterator Find(const Lib::Symbol &symbol) {
			return Iterator(
				new Iterator::Implementation(m_list.find(symbol.GetHash())));
		}
		virtual ConstIterator Find(const Lib::Symbol &symbol) const {
			return ConstIterator(
				new ConstIterator::Implementation(
					m_list.find(symbol.GetHash())));
		}

	private:

		ModuleSecurityListStorage m_list;

	};

	////////////////////////////////////////////////////////////////////////////////

}
