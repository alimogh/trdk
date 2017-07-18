/**************************************************************************
 *   Created: 2013/01/26 11:33:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 **************************************************************************/

#pragma once

#include "ModuleRef.hpp"

namespace trdk {

//////////////////////////////////////////////////////////////////////////

typedef boost::variant<trdk::ConstStrategyRefWrapper,
                       trdk::ConstServiceRefWrapper,
                       trdk::ConstObserverRefWrapper>
    ConstModuleRefVariant;
typedef boost::variant<trdk::StrategyRefWrapper,
                       trdk::ServiceRefWrapper,
                       trdk::ObserverRefWrapper>
    ModuleRefVariant;

//////////////////////////////////////////////////////////////////////////

namespace Visitors {

//////////////////////////////////////////////////////////////////////////

struct GetConstModule : public boost::static_visitor<const trdk::Module &> {
  const trdk::Strategy &operator()(const trdk::Strategy &module) const {
    return module;
  }
  const trdk::Service &operator()(const trdk::Service &module) const {
    return module;
  }
  const trdk::Observer &operator()(const trdk::Observer &module) const {
    return module;
  }
};

struct GetModule : public boost::static_visitor<trdk::Module &> {
  trdk::Strategy &operator()(trdk::Strategy &module) const { return module; }
  trdk::Service &operator()(trdk::Service &module) const { return module; }
  trdk::Observer &operator()(trdk::Observer &module) const { return module; }
};

//////////////////////////////////////////////////////////////////////////
}

//////////////////////////////////////////////////////////////////////////
}
