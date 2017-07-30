/**************************************************************************
 *   Created: 2016/09/12 01:23:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "BarService.hpp"

namespace trdk {
namespace Tests {
namespace Mocks {

class BarService : public trdk::Services::BarService {
 public:
  BarService();
  virtual ~BarService() override = default;

 public:
  MOCK_CONST_METHOD0(GetLastDataTime, const boost::posix_time::ptime &());

  MOCK_CONST_METHOD0(GetSize, size_t());
  MOCK_CONST_METHOD0(IsEmpty, bool());

  MOCK_CONST_METHOD0(GetLastBar, Bar());
  MOCK_CONST_METHOD0(GetSecurity, const trdk::Security &());

  MOCK_CONST_METHOD1(GetBar, Bar(size_t));
  MOCK_CONST_METHOD1(GetBarByReversedIndex, Bar(size_t));

  MOCK_CONST_METHOD1(DropLastBarCopy,
                     void(const trdk::DropCopyDataSourceInstanceId &));
  MOCK_CONST_METHOD1(DropUncompletedBarCopy,
                     void(const trdk::DropCopyDataSourceInstanceId &));
};
}
}
}