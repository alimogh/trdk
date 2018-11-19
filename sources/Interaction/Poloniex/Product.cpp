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

using namespace trdk;
using namespace Lib;
using namespace Interaction;
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

const std::string &Poloniex::ResolveCurrency(const uintmax_t id) {
  switch (id) {
    default:
      throw Exception("Currency ID is unknown");
    case 1: {
      static const std::string result("1CR");
      return result;
    }
    case 2: {
      static const std::string result("ABY");
      return result;
    }
    case 3: {
      static const std::string result("AC");
      return result;
    }
    case 4: {
      static const std::string result("ACH");
      return result;
    }
    case 5: {
      static const std::string result("ADN");
      return result;
    }
    case 6: {
      static const std::string result("AEON");
      return result;
    }
    case 7: {
      static const std::string result("AERO");
      return result;
    }
    case 8: {
      static const std::string result("AIR");
      return result;
    }
    case 9: {
      static const std::string result("APH");
      return result;
    }
    case 10: {
      static const std::string result("AUR");
      return result;
    }
    case 11: {
      static const std::string result("AXIS");
      return result;
    }
    case 12: {
      static const std::string result("BALLS");
      return result;
    }
    case 13: {
      static const std::string result("BANK");
      return result;
    }
    case 14: {
      static const std::string result("BBL");
      return result;
    }
    case 15: {
      static const std::string result("BBR");
      return result;
    }
    case 16: {
      static const std::string result("BCC");
      return result;
    }
    case 17: {
      static const std::string result("BCN");
      return result;
    }
    case 18: {
      static const std::string result("BDC");
      return result;
    }
    case 19: {
      static const std::string result("BDG");
      return result;
    }
    case 20: {
      static const std::string result("BELA");
      return result;
    }
    case 21: {
      static const std::string result("BITS");
      return result;
    }
    case 22: {
      static const std::string result("BLK");
      return result;
    }
    case 23: {
      static const std::string result("BLOCK");
      return result;
    }
    case 24: {
      static const std::string result("BLU");
      return result;
    }
    case 25: {
      static const std::string result("BNS");
      return result;
    }
    case 26: {
      static const std::string result("BONES");
      return result;
    }
    case 27: {
      static const std::string result("BOST");
      return result;
    }
    case 28: {
      static const std::string result("BTC");
      return result;
    }
    case 29: {
      static const std::string result("BTCD");
      return result;
    }
    case 30: {
      static const std::string result("BTCS");
      return result;
    }
    case 31: {
      static const std::string result("BTM");
      return result;
    }
    case 32: {
      static const std::string result("BTS");
      return result;
    }
    case 33: {
      static const std::string result("BURN");
      return result;
    }
    case 34: {
      static const std::string result("BURST");
      return result;
    }
    case 35: {
      static const std::string result("C2");
      return result;
    }
    case 36: {
      static const std::string result("CACH");
      return result;
    }
    case 37: {
      static const std::string result("CAI");
      return result;
    }
    case 38: {
      static const std::string result("CC");
      return result;
    }
    case 39: {
      static const std::string result("CCN");
      return result;
    }
    case 40: {
      static const std::string result("CGA");
      return result;
    }
    case 41: {
      static const std::string result("CHA");
      return result;
    }
    case 42: {
      static const std::string result("CINNI");
      return result;
    }
    case 43: {
      static const std::string result("CLAM");
      return result;
    }
    case 44: {
      static const std::string result("CNL");
      return result;
    }
    case 45: {
      static const std::string result("CNMT");
      return result;
    }
    case 46: {
      static const std::string result("CNOTE");
      return result;
    }
    case 47: {
      static const std::string result("COMM");
      return result;
    }
    case 48: {
      static const std::string result("CON");
      return result;
    }
    case 49: {
      static const std::string result("CORG");
      return result;
    }
    case 50: {
      static const std::string result("CRYPT");
      return result;
    }
    case 51: {
      static const std::string result("CURE");
      return result;
    }
    case 52: {
      static const std::string result("CYC");
      return result;
    }
    case 53: {
      static const std::string result("DGB");
      return result;
    }
    case 54: {
      static const std::string result("DICE");
      return result;
    }
    case 55: {
      static const std::string result("DIEM");
      return result;
    }
    case 56: {
      static const std::string result("DIME");
      return result;
    }
    case 57: {
      static const std::string result("DIS");
      return result;
    }
    case 58: {
      static const std::string result("DNS");
      return result;
    }
    case 59: {
      static const std::string result("DOGE");
      return result;
    }
    case 60: {
      static const std::string result("DASH");
      return result;
    }
    case 61: {
      static const std::string result("DRKC");
      return result;
    }
    case 62: {
      static const std::string result("DRM");
      return result;
    }
    case 63: {
      static const std::string result("DSH");
      return result;
    }
    case 64: {
      static const std::string result("DVK");
      return result;
    }
    case 65: {
      static const std::string result("EAC");
      return result;
    }
    case 66: {
      static const std::string result("EBT");
      return result;
    }
    case 67: {
      static const std::string result("ECC");
      return result;
    }
    case 68: {
      static const std::string result("EFL");
      return result;
    }
    case 69: {
      static const std::string result("EMC2");
      return result;
    }
    case 70: {
      static const std::string result("EMO");
      return result;
    }
    case 71: {
      static const std::string result("ENC");
      return result;
    }
    case 72: {
      static const std::string result("eTOK");
      return result;
    }
    case 73: {
      static const std::string result("EXE");
      return result;
    }
    case 74: {
      static const std::string result("FAC");
      return result;
    }
    case 75: {
      static const std::string result("FCN");
      return result;
    }
    case 76: {
      static const std::string result("FIBRE");
      return result;
    }
    case 77: {
      static const std::string result("FLAP");
      return result;
    }
    case 78: {
      static const std::string result("FLDC");
      return result;
    }
    case 79: {
      static const std::string result("FLT");
      return result;
    }
    case 80: {
      static const std::string result("FOX");
      return result;
    }
    case 81: {
      static const std::string result("FRAC");
      return result;
    }
    case 82: {
      static const std::string result("FRK");
      return result;
    }
    case 83: {
      static const std::string result("FRQ");
      return result;
    }
    case 84: {
      static const std::string result("FVZ");
      return result;
    }
    case 85: {
      static const std::string result("FZ");
      return result;
    }
    case 86: {
      static const std::string result("FZN");
      return result;
    }
    case 87: {
      static const std::string result("GAP");
      return result;
    }
    case 88: {
      static const std::string result("GDN");
      return result;
    }
    case 89: {
      static const std::string result("GEMZ");
      return result;
    }
    case 90: {
      static const std::string result("GEO");
      return result;
    }
    case 91: {
      static const std::string result("GIAR");
      return result;
    }
    case 92: {
      static const std::string result("GLB");
      return result;
    }
    case 93: {
      static const std::string result("GAME");
      return result;
    }
    case 94: {
      static const std::string result("GML");
      return result;
    }
    case 95: {
      static const std::string result("GNS");
      return result;
    }
    case 96: {
      static const std::string result("GOLD");
      return result;
    }
    case 97: {
      static const std::string result("GPC");
      return result;
    }
    case 98: {
      static const std::string result("GPUC");
      return result;
    }
    case 99: {
      static const std::string result("GRCX");
      return result;
    }
    case 100: {
      static const std::string result("GRS");
      return result;
    }
    case 101: {
      static const std::string result("GUE");
      return result;
    }
    case 102: {
      static const std::string result("H2O");
      return result;
    }
    case 103: {
      static const std::string result("HIRO");
      return result;
    }
    case 104: {
      static const std::string result("HOT");
      return result;
    }
    case 105: {
      static const std::string result("HUC");
      return result;
    }
    case 106: {
      static const std::string result("HVC");
      return result;
    }
    case 107: {
      static const std::string result("HYP");
      return result;
    }
    case 108: {
      static const std::string result("HZ");
      return result;
    }
    case 109: {
      static const std::string result("IFC");
      return result;
    }
    case 110: {
      static const std::string result("ITC");
      return result;
    }
    case 111: {
      static const std::string result("IXC");
      return result;
    }
    case 112: {
      static const std::string result("JLH");
      return result;
    }
    case 113: {
      static const std::string result("JPC");
      return result;
    }
    case 114: {
      static const std::string result("JUG");
      return result;
    }
    case 115: {
      static const std::string result("KDC");
      return result;
    }
    case 116: {
      static const std::string result("KEY");
      return result;
    }
    case 117: {
      static const std::string result("LC");
      return result;
    }
    case 118: {
      static const std::string result("LCL");
      return result;
    }
    case 119: {
      static const std::string result("LEAF");
      return result;
    }
    case 120: {
      static const std::string result("LGC");
      return result;
    }
    case 121: {
      static const std::string result("LOL");
      return result;
    }
    case 122: {
      static const std::string result("LOVE");
      return result;
    }
    case 123: {
      static const std::string result("LQD");
      return result;
    }
    case 124: {
      static const std::string result("LTBC");
      return result;
    }
    case 125: {
      static const std::string result("LTC");
      return result;
    }
    case 126: {
      static const std::string result("LTCX");
      return result;
    }
    case 127: {
      static const std::string result("MAID");
      return result;
    }
    case 128: {
      static const std::string result("MAST");
      return result;
    }
    case 129: {
      static const std::string result("MAX");
      return result;
    }
    case 130: {
      static const std::string result("MCN");
      return result;
    }
    case 131: {
      static const std::string result("MEC");
      return result;
    }
    case 132: {
      static const std::string result("METH");
      return result;
    }
    case 133: {
      static const std::string result("MIL");
      return result;
    }
    case 134: {
      static const std::string result("MIN");
      return result;
    }
    case 135: {
      static const std::string result("MINT");
      return result;
    }
    case 136: {
      static const std::string result("MMC");
      return result;
    }
    case 137: {
      static const std::string result("MMNXT");
      return result;
    }
    case 138: {
      static const std::string result("MMXIV");
      return result;
    }
    case 139: {
      static const std::string result("MNTA");
      return result;
    }
    case 140: {
      static const std::string result("MON");
      return result;
    }
    case 141: {
      static const std::string result("MRC");
      return result;
    }
    case 142: {
      static const std::string result("MRS");
      return result;
    }
    case 143: {
      static const std::string result("OMNI");
      return result;
    }
    case 144: {
      static const std::string result("MTS");
      return result;
    }
    case 145: {
      static const std::string result("MUN");
      return result;
    }
    case 146: {
      static const std::string result("MYR");
      return result;
    }
    case 147: {
      static const std::string result("MZC");
      return result;
    }
    case 148: {
      static const std::string result("N5X");
      return result;
    }
    case 149: {
      static const std::string result("NAS");
      return result;
    }
    case 150: {
      static const std::string result("NAUT");
      return result;
    }
    case 151: {
      static const std::string result("NAV");
      return result;
    }
    case 152: {
      static const std::string result("NBT");
      return result;
    }
    case 153: {
      static const std::string result("NEOS");
      return result;
    }
    case 154: {
      static const std::string result("NL");
      return result;
    }
    case 155: {
      static const std::string result("NMC");
      return result;
    }
    case 156: {
      static const std::string result("NOBL");
      return result;
    }
    case 157: {
      static const std::string result("NOTE");
      return result;
    }
    case 158: {
      static const std::string result("NOXT");
      return result;
    }
    case 159: {
      static const std::string result("NRS");
      return result;
    }
    case 160: {
      static const std::string result("NSR");
      return result;
    }
    case 161: {
      static const std::string result("NTX");
      return result;
    }
    case 162: {
      static const std::string result("NXT");
      return result;
    }
    case 163: {
      static const std::string result("NXTI");
      return result;
    }
    case 164: {
      static const std::string result("OPAL");
      return result;
    }
    case 165: {
      static const std::string result("PAND");
      return result;
    }
    case 166: {
      static const std::string result("PAWN");
      return result;
    }
    case 167: {
      static const std::string result("PIGGY");
      return result;
    }
    case 168: {
      static const std::string result("PINK");
      return result;
    }
    case 169: {
      static const std::string result("PLX");
      return result;
    }
    case 170: {
      static const std::string result("PMC");
      return result;
    }
    case 171: {
      static const std::string result("POT");
      return result;
    }
    case 172: {
      static const std::string result("PPC");
      return result;
    }
    case 173: {
      static const std::string result("PRC");
      return result;
    }
    case 174: {
      static const std::string result("PRT");
      return result;
    }
    case 175: {
      static const std::string result("PTS");
      return result;
    }
    case 176: {
      static const std::string result("Q2C");
      return result;
    }
    case 177: {
      static const std::string result("QBK");
      return result;
    }
    case 178: {
      static const std::string result("QCN");
      return result;
    }
    case 179: {
      static const std::string result("QORA");
      return result;
    }
    case 180: {
      static const std::string result("QTL");
      return result;
    }
    case 181: {
      static const std::string result("RBY");
      return result;
    }
    case 182: {
      static const std::string result("RDD");
      return result;
    }
    case 183: {
      static const std::string result("RIC");
      return result;
    }
    case 184: {
      static const std::string result("RZR");
      return result;
    }
    case 185: {
      static const std::string result("SDC");
      return result;
    }
    case 186: {
      static const std::string result("SHIBE");
      return result;
    }
    case 187: {
      static const std::string result("SHOPX");
      return result;
    }
    case 188: {
      static const std::string result("SILK");
      return result;
    }
    case 189: {
      static const std::string result("SJCX");
      return result;
    }
    case 190: {
      static const std::string result("SLR");
      return result;
    }
    case 191: {
      static const std::string result("SMC");
      return result;
    }
    case 192: {
      static const std::string result("SOC");
      return result;
    }
    case 193: {
      static const std::string result("SPA");
      return result;
    }
    case 194: {
      static const std::string result("SQL");
      return result;
    }
    case 195: {
      static const std::string result("SRCC");
      return result;
    }
    case 196: {
      static const std::string result("SRG");
      return result;
    }
    case 197: {
      static const std::string result("SSD");
      return result;
    }
    case 198: {
      static const std::string result("STR");
      return result;
    }
    case 199: {
      static const std::string result("SUM");
      return result;
    }
    case 200: {
      static const std::string result("SUN");
      return result;
    }
    case 201: {
      static const std::string result("SWARM");
      return result;
    }
    case 202: {
      static const std::string result("SXC");
      return result;
    }
    case 203: {
      static const std::string result("SYNC");
      return result;
    }
    case 204: {
      static const std::string result("SYS");
      return result;
    }
    case 205: {
      static const std::string result("TAC");
      return result;
    }
    case 206: {
      static const std::string result("TOR");
      return result;
    }
    case 207: {
      static const std::string result("TRUST");
      return result;
    }
    case 208: {
      static const std::string result("TWE");
      return result;
    }
    case 209: {
      static const std::string result("UIS");
      return result;
    }
    case 210: {
      static const std::string result("ULTC");
      return result;
    }
    case 211: {
      static const std::string result("UNITY");
      return result;
    }
    case 212: {
      static const std::string result("URO");
      return result;
    }
    case 213: {
      static const std::string result("USDE");
      return result;
    }
    case 214: {
      static const std::string result("USDT");
      return result;
    }
    case 215: {
      static const std::string result("UTC");
      return result;
    }
    case 216: {
      static const std::string result("UTIL");
      return result;
    }
    case 217: {
      static const std::string result("UVC");
      return result;
    }
    case 218: {
      static const std::string result("VIA");
      return result;
    }
    case 219: {
      static const std::string result("VOOT");
      return result;
    }
    case 220: {
      static const std::string result("VRC");
      return result;
    }
    case 221: {
      static const std::string result("VTC");
      return result;
    }
    case 222: {
      static const std::string result("WC");
      return result;
    }
    case 223: {
      static const std::string result("WDC");
      return result;
    }
    case 224: {
      static const std::string result("WIKI");
      return result;
    }
    case 225: {
      static const std::string result("WOLF");
      return result;
    }
    case 226: {
      static const std::string result("X13");
      return result;
    }
    case 227: {
      static const std::string result("XAI");
      return result;
    }
    case 228: {
      static const std::string result("XAP");
      return result;
    }
    case 229: {
      static const std::string result("XBC");
      return result;
    }
    case 230: {
      static const std::string result("XC");
      return result;
    }
    case 231: {
      static const std::string result("XCH");
      return result;
    }
    case 232: {
      static const std::string result("XCN");
      return result;
    }
    case 233: {
      static const std::string result("XCP");
      return result;
    }
    case 234: {
      static const std::string result("XCR");
      return result;
    }
    case 235: {
      static const std::string result("XDN");
      return result;
    }
    case 236: {
      static const std::string result("XDP");
      return result;
    }
    case 237: {
      static const std::string result("XHC");
      return result;
    }
    case 238: {
      static const std::string result("XLB");
      return result;
    }
    case 239: {
      static const std::string result("XMG");
      return result;
    }
    case 240: {
      static const std::string result("XMR");
      return result;
    }
    case 241: {
      static const std::string result("XPB");
      return result;
    }
    case 242: {
      static const std::string result("XPM");
      return result;
    }
    case 243: {
      static const std::string result("XRP");
      return result;
    }
    case 244: {
      static const std::string result("XSI");
      return result;
    }
    case 245: {
      static const std::string result("XST");
      return result;
    }
    case 246: {
      static const std::string result("XSV");
      return result;
    }
    case 247: {
      static const std::string result("XUSD");
      return result;
    }
    case 248: {
      static const std::string result("XXC");
      return result;
    }
    case 249: {
      static const std::string result("YACC");
      return result;
    }
    case 250: {
      static const std::string result("YANG");
      return result;
    }
    case 251: {
      static const std::string result("YC");
      return result;
    }
    case 252: {
      static const std::string result("YIN");
      return result;
    }
    case 253: {
      static const std::string result("XVC");
      return result;
    }
    case 254: {
      static const std::string result("FLO");
      return result;
    }
    case 256: {
      static const std::string result("XEM");
      return result;
    }
    case 258: {
      static const std::string result("ARCH");
      return result;
    }
    case 260: {
      static const std::string result("HUGE");
      return result;
    }
    case 261: {
      static const std::string result("GRC");
      return result;
    }
    case 263: {
      static const std::string result("IOC");
      return result;
    }
    case 265: {
      static const std::string result("INDEX");
      return result;
    }
    case 267: {
      static const std::string result("ETH");
      return result;
    }
    case 268: {
      static const std::string result("SC");
      return result;
    }
    case 269: {
      static const std::string result("BCY");
      return result;
    }
    case 270: {
      static const std::string result("EXP");
      return result;
    }
    case 271: {
      static const std::string result("FCT");
      return result;
    }
    case 272: {
      static const std::string result("BITUSD");
      return result;
    }
    case 273: {
      static const std::string result("BITCNY");
      return result;
    }
    case 274: {
      static const std::string result("RADS");
      return result;
    }
    case 275: {
      static const std::string result("AMP");
      return result;
    }
    case 276: {
      static const std::string result("VOX");
      return result;
    }
    case 277: {
      static const std::string result("DCR");
      return result;
    }
    case 278: {
      static const std::string result("LSK");
      return result;
    }
    case 279: {
      static const std::string result("DAO");
      return result;
    }
    case 280: {
      static const std::string result("LBC");
      return result;
    }
    case 281: {
      static const std::string result("STEEM");
      return result;
    }
    case 282: {
      static const std::string result("SBD");
      return result;
    }
    case 283: {
      static const std::string result("ETC");
      return result;
    }
    case 284: {
      static const std::string result("REP");
      return result;
    }
    case 285: {
      static const std::string result("ARDR");
      return result;
    }
    case 286: {
      static const std::string result("ZEC");
      return result;
    }
    case 287: {
      static const std::string result("STRAT");
      return result;
    }
    case 288: {
      static const std::string result("NXC");
      return result;
    }
    case 289: {
      static const std::string result("PASC");
      return result;
    }
    case 290: {
      static const std::string result("GNT");
      return result;
    }
    case 291: {
      static const std::string result("GNO");
      return result;
    }
    case 292: {
      static const std::string result("BCH");
      return result;
    }
    case 293: {
      static const std::string result("ZRX");
      return result;
    }
    case 294: {
      static const std::string result("CVC");
      return result;
    }
    case 295: {
      static const std::string result("OMG");
      return result;
    }
    case 296: {
      static const std::string result("GAS");
      return result;
    }
    case 297: {
      static const std::string result("STORJ");
      return result;
    }
    case 298: {
      static const std::string result("EOS");
      return result;
    }
    case 299: {
      static const std::string result("USDC");
      return result;
    }
    case 300: {
      static const std::string result("SNT");
      return result;
    }
    case 301: {
      static const std::string result("KNC");
      return result;
    }
    case 302: {
      static const std::string result("BAT");
      return result;
    }
    case 303: {
      static const std::string result("LOOM");
      return result;
    }
    case 304: {
      static const std::string result("QTUM");
      return result;
    }
    case 305: {
      static const std::string result("BNT");
      return result;
    }
    case 306: {
      static const std::string result("MANA");
      return result;
    }
    case 308: {
      static const std::string result("BCHABC");
      return result;
    }
    case 309: {
      static const std::string result("BCHSV");
      return result;
    }
  }
}
