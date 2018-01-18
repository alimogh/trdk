/*******************************************************************************
 *   Created: 2018/01/18 00:34:06
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

typedef std::string CexioProductId;

struct CexioProduct {
  CexioProductId id;
  boost::optional<Qty> minSize;
  boost::optional<Qty> minSizeS2;
  boost::optional<Qty> maxSize;
  boost::optional<Price> minPrice;
  boost::optional<Price> maxPrice;
};

boost::unordered_map<std::string, CexioProduct> RequestCexioProductList(
    Poco::Net::HTTPClientSession &, const Context &, ModuleEventsLog &);

boost::posix_time::ptime ParseCexioTimeStamp(
    const boost::property_tree::ptree &source,
    const boost::posix_time::time_duration &serverTimeDiff);

std::unique_ptr<Poco::Net::HTTPClientSession> CreateCexioSession(
    const Settings &, bool isTrading);
}
}
}
