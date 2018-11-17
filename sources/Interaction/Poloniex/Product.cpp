//
//    Created: 2018/11/14 17:05
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "Product.hpp"

using namespace trdk::Interaction;
using namespace Poloniex;

namespace {
boost::unordered_map<std::string, Product> RequestProductList() {
  std::vector<boost::tuple<ProductId, std::string, std::string>> symbols;
  symbols.emplace_back(7, "BTC", "BCN");
  symbols.emplace_back(14, "BTC", "BTS");
  symbols.emplace_back(15, "BTC", "BURST");
  symbols.emplace_back(20, "BTC", "CLAM");
  symbols.emplace_back(25, "BTC", "DGB");
  symbols.emplace_back(27, "BTC", "DOGE");
  symbols.emplace_back(24, "BTC", "DASH");
  symbols.emplace_back(38, "BTC", "GAME");
  symbols.emplace_back(43, "BTC", "HUC");
  symbols.emplace_back(50, "BTC", "LTC");
  symbols.emplace_back(51, "BTC", "MAID");
  symbols.emplace_back(58, "BTC", "OMNI");
  symbols.emplace_back(61, "BTC", "NAV");
  symbols.emplace_back(64, "BTC", "NMC");
  symbols.emplace_back(69, "BTC", "NXT");
  symbols.emplace_back(75, "BTC", "PPC");
  symbols.emplace_back(89, "BTC", "STR");
  symbols.emplace_back(92, "BTC", "SYS");
  symbols.emplace_back(97, "BTC", "VIA");
  symbols.emplace_back(100, "BTC", "VTC");
  symbols.emplace_back(108, "BTC", "XCP");
  symbols.emplace_back(114, "BTC", "XMR");
  symbols.emplace_back(116, "BTC", "XPM");
  symbols.emplace_back(117, "BTC", "XRP");
  symbols.emplace_back(112, "BTC", "XEM");
  symbols.emplace_back(148, "BTC", "ETH");
  symbols.emplace_back(150, "BTC", "SC");
  symbols.emplace_back(155, "BTC", "FCT");
  symbols.emplace_back(162, "BTC", "DCR");
  symbols.emplace_back(163, "BTC", "LSK");
  symbols.emplace_back(167, "BTC", "LBC");
  symbols.emplace_back(168, "BTC", "STEEM");
  symbols.emplace_back(170, "BTC", "SBD");
  symbols.emplace_back(171, "BTC", "ETC");
  symbols.emplace_back(174, "BTC", "REP");
  symbols.emplace_back(177, "BTC", "ARDR");
  symbols.emplace_back(178, "BTC", "ZEC");
  symbols.emplace_back(182, "BTC", "STRAT");
  symbols.emplace_back(184, "BTC", "PASC");
  symbols.emplace_back(185, "BTC", "GNT");
  symbols.emplace_back(189, "BTC", "BCH");
  symbols.emplace_back(192, "BTC", "ZRX");
  symbols.emplace_back(194, "BTC", "CVC");
  symbols.emplace_back(196, "BTC", "OMG");
  symbols.emplace_back(198, "BTC", "GAS");
  symbols.emplace_back(200, "BTC", "STORJ");
  symbols.emplace_back(201, "BTC", "EOS");
  symbols.emplace_back(204, "BTC", "SNT");
  symbols.emplace_back(207, "BTC", "KNC");
  symbols.emplace_back(210, "BTC", "BAT");
  symbols.emplace_back(213, "BTC", "LOOM");
  symbols.emplace_back(221, "BTC", "QTUM");
  symbols.emplace_back(232, "BTC", "BNT");
  symbols.emplace_back(229, "BTC", "MANA");
  symbols.emplace_back(236, "BTC", "BCHABC");
  symbols.emplace_back(238, "BTC", "BCHSV");
  symbols.emplace_back(121, "USDT", "BTC");
  symbols.emplace_back(216, "USDT", "DOGE");
  symbols.emplace_back(122, "USDT", "DASH");
  symbols.emplace_back(123, "USDT", "LTC");
  symbols.emplace_back(124, "USDT", "NXT");
  symbols.emplace_back(125, "USDT", "STR");
  symbols.emplace_back(126, "USDT", "XMR");
  symbols.emplace_back(127, "USDT", "XRP");
  symbols.emplace_back(149, "USDT", "ETH");
  symbols.emplace_back(219, "USDT", "SC");
  symbols.emplace_back(218, "USDT", "LSK");
  symbols.emplace_back(173, "USDT", "ETC");
  symbols.emplace_back(175, "USDT", "REP");
  symbols.emplace_back(180, "USDT", "ZEC");
  symbols.emplace_back(217, "USDT", "GNT");
  symbols.emplace_back(191, "USDT", "BCH");
  symbols.emplace_back(220, "USDT", "ZRX");
  symbols.emplace_back(203, "USDT", "EOS");
  symbols.emplace_back(206, "USDT", "SNT");
  symbols.emplace_back(209, "USDT", "KNC");
  symbols.emplace_back(212, "USDT", "BAT");
  symbols.emplace_back(215, "USDT", "LOOM");
  symbols.emplace_back(223, "USDT", "QTUM");
  symbols.emplace_back(234, "USDT", "BNT");
  symbols.emplace_back(231, "USDT", "MANA");
  symbols.emplace_back(129, "XMR", "BCN");
  symbols.emplace_back(132, "XMR", "DASH");
  symbols.emplace_back(137, "XMR", "LTC");
  symbols.emplace_back(138, "XMR", "MAID");
  symbols.emplace_back(140, "XMR", "NXT");
  symbols.emplace_back(181, "XMR", "ZEC");
  symbols.emplace_back(166, "ETH", "LSK");
  symbols.emplace_back(169, "ETH", "STEEM");
  symbols.emplace_back(172, "ETH", "ETC");
  symbols.emplace_back(176, "ETH", "REP");
  symbols.emplace_back(179, "ETH", "ZEC");
  symbols.emplace_back(186, "ETH", "GNT");
  symbols.emplace_back(190, "ETH", "BCH");
  symbols.emplace_back(193, "ETH", "ZRX");
  symbols.emplace_back(195, "ETH", "CVC");
  symbols.emplace_back(197, "ETH", "OMG");
  symbols.emplace_back(199, "ETH", "GAS");
  symbols.emplace_back(202, "ETH", "EOS");
  symbols.emplace_back(205, "ETH", "SNT");
  symbols.emplace_back(208, "ETH", "KNC");
  symbols.emplace_back(211, "ETH", "BAT");
  symbols.emplace_back(214, "ETH", "LOOM");
  symbols.emplace_back(222, "ETH", "QTUM");
  symbols.emplace_back(233, "ETH", "BNT");
  symbols.emplace_back(230, "ETH", "MANA");
  symbols.emplace_back(224, "USDC", "BTC");
  symbols.emplace_back(226, "USDC", "USDT");
  symbols.emplace_back(225, "USDC", "ETH");
  symbols.emplace_back(235, "USDC", "BCH");
  symbols.emplace_back(237, "USDC", "BCHABC");
  symbols.emplace_back(239, "USDC", "BCHSV");

  boost::unordered_map<std::string, Product> result;
  for (const auto &symbol : symbols) {
    Verify(result
               .emplace(boost::get<2>(symbol) + "_" + boost::get<1>(symbol),
                        Product{boost::get<0>(symbol)})
               .second);
  }
  return result;
}
}  // namespace

const boost::unordered_map<std::string, Product> &Poloniex::GetProductList() {
  static const auto result = RequestProductList();
  return result;
}
