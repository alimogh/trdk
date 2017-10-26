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
  explicit StopLossOrder(
      trdk::Position &,
      const boost::shared_ptr<const trdk::TradingLib::OrderPolicy> &);
  virtual ~StopLossOrder() override = default;

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
  explicit StopPrice(
      const boost::shared_ptr<const Params> &,
      trdk::Position &,
      const boost::shared_ptr<const trdk::TradingLib::OrderPolicy> &);
  virtual ~StopPrice() override = default;

  virtual void Report(const trdk::Position &,
                      trdk::ModuleTradingLog &) const override;

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
  explicit StopLoss(
      const boost::shared_ptr<const Params> &,
      trdk::Position &,
      const boost::shared_ptr<const trdk::TradingLib::OrderPolicy> &);
  virtual ~StopLoss() override = default;

  virtual void Report(const trdk::Position &,
                      trdk::ModuleTradingLog &) const override;

 protected:
  virtual const char *GetName() const override;

  virtual bool Activate() override;

 private:
  const boost::shared_ptr<const Params> m_params;
};

////////////////////////////////////////////////////////////////////////////////

class StopLossShare : public trdk::TradingLib::StopLossOrder {
 public:
  class Params {
   public:
    //! Constructor.
    /** @param[in] maxLossShare Maximum share of loss where 1.00 is 100% of
      *                         position volume at opening. Minus means profit.
      */
    explicit Params(const trdk::Lib::Double &maxLossShare);

   public:
    const trdk::Lib::Double &GetMaxLossShare() const;

   private:
    trdk::Lib::Double m_maxLossShare;
  };

 public:
  explicit StopLossShare(
      const boost::shared_ptr<const Params> &,
      trdk::Position &,
      const boost::shared_ptr<const trdk::TradingLib::OrderPolicy> &);
  explicit StopLossShare(
      const trdk::Lib::Double &maxLossShare,
      trdk::Position &,
      const boost::shared_ptr<const trdk::TradingLib::OrderPolicy> &);

  virtual void Report(const trdk::Position &,
                      trdk::ModuleTradingLog &) const override;

 protected:
  virtual const char *GetName() const override;

  virtual bool Activate() override;

 private:
  const boost::shared_ptr<const Params> m_params;
};

////////////////////////////////////////////////////////////////////////////////
}
}
