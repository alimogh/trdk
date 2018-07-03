//
//    Created: 2018/04/07 3:47 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

namespace trdk {

struct Bar {
  //! Bar start time.
  boost::posix_time::ptime time;
  //! Bar size (if bar-by-time). If period is not set - this is bar-by-points.
  /** @sa numberOfPoints
   */
  boost::optional<boost::posix_time::time_duration> period;
  //! Number of points (if existent).
  /** @sa period
   */
  boost::optional<size_t> numberOfPoints;

  //! The bar opening price.
  boost::optional<Price> openPrice;
  //! The high price during the time covered by the bar.
  boost::optional<Price> highPrice;

  //! The low price during the time covered by the bar.
  boost::optional<Price> lowPrice;
  //! The bar closing price.
  boost::optional<Price> closePrice;

  //! The volume during the time covered by the bar.
  boost::optional<Qty> volume;
};

}  // namespace trdk
