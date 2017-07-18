/**************************************************************************
 *   Created: 2016/12/14 02:51:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "StopOrder.hpp"

namespace trdk {
namespace TradingLib {

////////////////////////////////////////////////////////////////////////////////

class StopLossOrder : public trdk::TradingLib::StopOrder {
 public:
  explicit StopLossOrder(trdk::Position &);
  virtual ~StopLossOrder();

 public:
  virtual void Run() override;

 protected:
  virtual bool Activate() = 0;
};

////////////////////////////////////////////////////////////////////////////////

class StopPrice : public trdk::TradingLib::StopLossOrder {
 public:
  class Params {
   public:
    explicit Params(const trdk::Price &price);

   public:
    const trdk::Price &GetPrice() const;

   private:
    trdk::Price m_price;
  };

 public:
  explicit StopPrice(const boost::shared_ptr<const Params> &, trdk::Position &);
  virtual ~StopPrice();

 protected:
  virtual const char *GetName() const override;

  virtual bool Activate() override;

 private:
  const boost::shared_ptr<const Params> m_params;
};

////////////////////////////////////////////////////////////////////////////////

class StopLoss : public trdk::TradingLib::StopLossOrder {
 public:
  class Params {
   public:
    explicit Params(const trdk::Volume &maxLossPerLot);

   public:
    const trdk::Volume &GetMaxLossPerLot() const;

   private:
    trdk::Volume m_maxLossPerLot;
  };

 public:
  explicit StopLoss(const boost::shared_ptr<const Params> &, trdk::Position &);
  virtual ~StopLoss();

 protected:
  virtual const char *GetName() const override;

  virtual bool Activate() override;

 private:
  const boost::shared_ptr<const Params> m_params;
};

////////////////////////////////////////////////////////////////////////////////
}
}
