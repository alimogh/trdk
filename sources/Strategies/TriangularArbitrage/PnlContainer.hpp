/*******************************************************************************
 *   Created: 2018/03/10 20:12:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Strategies {
namespace TriangularArbitrage {

class PnlContainer : public trdk::PnlContainer {
 public:
  typedef trdk::PnlContainer Base;

 public:
  explicit PnlContainer(const Security &leg1,
                        const Security &leg2,
                        const Volume &leg2Commission);
  virtual ~PnlContainer() override = default;

 public:
  virtual bool Update(const trdk::Security &,
                      const trdk::OrderSide &,
                      const trdk::Qty &,
                      const trdk::Price &,
                      const trdk::Volume &commission) override;
  virtual Result GetResult() const override;
  virtual const Data &GetData() const override;

 private:
  boost::optional<Volume> UpdateSymbol(const std::string &symbol,
                                       const Lib::Double &delta);

 private:
  const Security &m_leg1;
  const Security &m_leg2;
  const Volume m_leg2MaxLoss;

  Data m_data;
  Volume m_aBalance;
  Volume m_bBalance;
  Volume m_cBalance;
  bool m_hasUnknownBalance;
};

}  // namespace TriangularArbitrage
}  // namespace Strategies
}  // namespace trdk