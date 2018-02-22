/*******************************************************************************
 *   Created: 2018/02/11 18:10:35
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
namespace Rest {

typedef std::string ExcambiorexProductId;

struct ExcambiorexProduct {
  ExcambiorexProductId directId;
  std::string symbol;
  std::string buyCoinAlias;
  std::string sellCoinAlias;
  ExcambiorexProductId oppositeId;
  const ExcambiorexProduct *oppositeProduct;
};

struct BySymbol {};
struct ById {};

typedef boost::multi_index_container<
    ExcambiorexProduct,
    boost::multi_index::indexed_by<
        boost::multi_index::hashed_unique<
            boost::multi_index::tag<BySymbol>,
            boost::multi_index::member<ExcambiorexProduct,
                                       std::string,
                                       &ExcambiorexProduct::symbol>>,
        boost::multi_index::hashed_unique<
            boost::multi_index::tag<ById>,
            boost::multi_index::member<ExcambiorexProduct,
                                       ExcambiorexProductId,
                                       &ExcambiorexProduct::directId>>>>
    ExcambiorexProductList;

std::pair<ExcambiorexProductList,
          boost::unordered_map<std::string, std::string>>
RequestExcambiorexProductAndCurrencyList(
    std::unique_ptr<Poco::Net::HTTPSClientSession> &,
    const Context &,
    ModuleEventsLog &);

std::unique_ptr<Poco::Net::HTTPSClientSession> CreateExcambiorexSession(
    const Settings &, bool isTrading);
}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
