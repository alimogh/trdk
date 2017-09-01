/*******************************************************************************
 *   Created: 2017/08/28 10:43:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Trend.hpp"

namespace tl = trdk::TradingLib;

using namespace testing;

namespace {
class Trend : public tl::Trend {
 public:
  using tl::Trend::SetIsRising;
};
}

TEST(TrendingLib_Trend, General) {
  Trend trend;

  ASSERT_FALSE(trend.IsExistent());
  ASSERT_FALSE(trend.IsRising());
  ASSERT_FALSE(trend.IsFalling());

  ASSERT_TRUE(trend.SetIsRising(false));
  ASSERT_TRUE(trend.IsExistent());
  ASSERT_FALSE(trend.IsRising());
  ASSERT_TRUE(trend.IsFalling());

  ASSERT_FALSE(trend.SetIsRising(false));
  ASSERT_TRUE(trend.IsExistent());
  ASSERT_FALSE(trend.IsRising());
  ASSERT_TRUE(trend.IsFalling());

  ASSERT_TRUE(trend.SetIsRising(true));
  ASSERT_TRUE(trend.IsExistent());
  ASSERT_TRUE(trend.IsRising());
  ASSERT_FALSE(trend.IsFalling());

  ASSERT_FALSE(trend.SetIsRising(true));
  ASSERT_TRUE(trend.IsExistent());
  ASSERT_TRUE(trend.IsRising());
  ASSERT_FALSE(trend.IsFalling());

  ASSERT_TRUE(trend.SetIsRising(false));
  ASSERT_TRUE(trend.IsExistent());
  ASSERT_FALSE(trend.IsRising());
  ASSERT_TRUE(trend.IsFalling());

  ASSERT_FALSE(trend.SetIsRising(false));
  ASSERT_TRUE(trend.IsExistent());
  ASSERT_FALSE(trend.IsRising());
  ASSERT_TRUE(trend.IsFalling());
}