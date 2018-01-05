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
  explicit StopLossOrder(trdk::Position &,
                         trdk::TradingLib::PositionController &);
  explicit StopLossOrder(const boost::posix_time::time_duration &delay,
                         trdk::Position &,
                         trdk::TradingLib::PositionController &);
  virtual ~StopLossOrder() override = default;

 public:
  virtual void Run() override;

  virtual bool IsWatching() const;

 protected:
  virtual const boost::posix_time::ptime &GetStartTime() const;
  const boost::posix_time::time_duration &GetDelay() const { return m_delay; }
  virtual bool Activate() = 0;

 private:
  const boost::posix_time::time_duration m_delay;
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
  explicit StopPrice(const boost::shared_ptr<const Params> &,
                     trdk::Position &,
                     trdk::TradingLib::PositionController &);
  virtual ~StopPrice() override = default;

  virtual void Report(const char *action) const override;

 protected:
  virtual bool Activate() override;
  virtual trdk::Price GetActualPrice() const = 0;

 private:
  const boost::shared_ptr<const Params> m_params;
};

class StopLastPrice : public trdk::TradingLib::StopPrice {
 public:
  explicit StopLastPrice(const boost::shared_ptr<const Params> &,
                         trdk::Position &,
                         trdk::TradingLib::PositionController &);
  virtual ~StopLastPrice() override = default;

 protected:
  virtual const char *GetName() const override;
  virtual trdk::Price GetActualPrice() const override;
};

class StopBidAskPrice : public trdk::TradingLib::StopPrice {
 public:
  explicit StopBidAskPrice(const boost::shared_ptr<const Params> &,
                           trdk::Position &,
                           trdk::TradingLib::PositionController &);
  virtual ~StopBidAskPrice() override = default;

 protected:
  virtual const char *GetName() const override;
  virtual trdk::Price GetActualPrice() const override;
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
  explicit StopLoss(const boost::shared_ptr<const Params> &,
                    trdk::Position &,
                    trdk::TradingLib::PositionController &);
  explicit StopLoss(const boost::shared_ptr<const Params> &,
                    const boost::posix_time::time_duration &delay,
                    trdk::Position &,
                    trdk::TradingLib::PositionController &);
  virtual ~StopLoss() override = default;

  virtual void Report(const char *action) const override;

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
  explicit StopLossShare(const boost::shared_ptr<const Params> &,
                         trdk::Position &,
                         trdk::TradingLib::PositionController &);
  explicit StopLossShare(const trdk::Lib::Double &maxLossShare,
                         trdk::Position &,
                         trdk::TradingLib::PositionController &);

  virtual void Report(const char *action) const override;

 protected:
  virtual const char *GetName() const override;

  virtual bool Activate() override;

 private:
  const boost::shared_ptr<const Params> m_params;
};

////////////////////////////////////////////////////////////////////////////////
}
}
