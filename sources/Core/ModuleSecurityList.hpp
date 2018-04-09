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

namespace trdk {

////////////////////////////////////////////////////////////////////////////////

typedef boost::unordered_map<
    boost::tuple<size_t /*trdk::Lib::Symbol::Hash*/, const MarketDataSource *>,
    Security *>
    ModuleSecurityListStorage;

////////////////////////////////////////////////////////////////////////////////

class Module::SecurityList::Iterator::Implementation {
 public:
  ModuleSecurityListStorage::iterator iterator;

  explicit Implementation(const ModuleSecurityListStorage::iterator &iterator)
      : iterator(iterator) {}
};

class Module::SecurityList::ConstIterator::Implementation {
 public:
  ModuleSecurityListStorage::const_iterator iterator;

  explicit Implementation(
      const ModuleSecurityListStorage::const_iterator &iterator)
      : iterator(iterator) {}
};

////////////////////////////////////////////////////////////////////////////////

class ModuleSecurityList : public Module::SecurityList {
 public:
  ~ModuleSecurityList() override = default;

  bool Insert(Security &security) {
    const auto &result =
        m_list.emplace(boost::make_tuple(security.GetSymbol().GetHash(),
                                         &security.GetSource()),
                       &security);
    // Detects symbol hash collisions:
    Assert(result.second || &security == result.first->second);
    return result.second;
  }

  size_t GetSize() const override { return m_list.size(); }

  bool IsEmpty() const override { return m_list.empty(); }

  Iterator GetBegin() override {
    return Iterator(
        boost::make_unique<Iterator::Implementation>(m_list.begin()));
  }
  ConstIterator GetBegin() const override {
    return ConstIterator(
        boost::make_unique<ConstIterator::Implementation>(m_list.begin()));
  }
  Iterator GetEnd() override {
    return Iterator(boost::make_unique<Iterator::Implementation>(m_list.end()));
  }
  ConstIterator GetEnd() const override {
    return ConstIterator(
        boost::make_unique<ConstIterator::Implementation>(m_list.end()));
  }

 private:
  ModuleSecurityListStorage m_list;
};

////////////////////////////////////////////////////////////////////////////////
}  // namespace trdk
