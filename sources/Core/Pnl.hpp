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

class Pnl {
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

  Pnl() = default;
  Pnl(Pnl &&) = default;
  Pnl(const Pnl &) = delete;
  Pnl &operator=(Pnl &&) = delete;
  Pnl &operator=(const Pnl &) = delete;
  virtual ~Pnl() = default;

  virtual Result GetResult() const = 0;
  virtual const Data &GetData() const = 0;
};
}  // namespace trdk
