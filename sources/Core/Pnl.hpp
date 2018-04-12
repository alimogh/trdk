/*******************************************************************************
 *   Created: 2018/01/23 11:02:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {

class Pnl : boost::noncopyable {
 public:
  enum Result {
    RESULT_NONE,
    RESULT_COMPLETED,
    RESULT_PROFIT,
    RESULT_LOSS,
    RESULT_ERROR,
    numberOfResults
  };

  struct SymbolData {
    Volume financialResult;
    Volume commission;
  };
  typedef boost::unordered_map<std::string, SymbolData> Data;

  virtual ~Pnl() = default;

  virtual Result GetResult() const = 0;
  virtual const Data& GetData() const = 0;
};
}  // namespace trdk
