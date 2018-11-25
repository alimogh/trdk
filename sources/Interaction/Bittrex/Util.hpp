/*******************************************************************************
 *   Created: 2017/11/19 18:24:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Interaction {
namespace Bittrex {

using ProductId = std::string;

struct Product {
  ProductId id;
  Qty minQty;
};

const boost::unordered_map<std::string, Product> &GetProductList(
    std::unique_ptr<Poco::Net::HTTPSClientSession> &,
    const Context &,
    ModuleEventsLog &);
}  // namespace Bittrex
}  // namespace Interaction
}  // namespace trdk
